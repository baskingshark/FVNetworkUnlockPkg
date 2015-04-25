/**
 * Copyright (c) 2015, baskingshark
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <Uefi.h>
#include <Library/UefiBootServicesTableLib.h>

/**
  Library routines.
  Replacements for and additions to string routines in MdePkg.
 */

/**
  Return the length of a NULL-terminated ASCII string.

  @param  String The pointer to the NULL-terminated string.

  @return The length of the string.
 */
STATIC
UINTN
EFIAPI
AsciiStrLen(IN CHAR8 CONST *String)
{
  UINTN Length;
  for(Length = 0; *String++; Length++)
    ;
  return Length;
}

/**
  Return the length of a NULL-terminated ASCII string with a maximum length.

  @param  String    The pointer to the NULL-terminated string.
  @param  MaxLength The maximum allowed length of the string.

  @return The length of the string, or MaxLength if a NULL was not found.
 */
STATIC
UINTN
EFIAPI
AsciiStrnLen(IN CHAR8 CONST *String,
             IN UINTN        MaxLength)
{
  UINTN Length;
  for(Length = 0; Length < MaxLength && *String++; Length++)
    ;
  return Length;
}

/**
  Compares two Null-terminated ASCII strings with maximum lengths.

  @param  String1   The pointer to the NULL-terminated string.
  @param  String2   The pointer to the NULL-terminated string.
  @param  Length    The maximum number of characters to compare.

  @return An integer greater than, equal to or less than 0 depending on whether
          String1 is greater than, equal to or less than String2.
 */
STATIC
UINTN
AsciiStrnCmp(IN CHAR8 CONST *String1,
             IN CHAR8 CONST *String2,
             IN UINTN        Length)
{
  if(!Length)
    return 0;

  while(*String1 && *String1 == *String2 && Length > 1) {
    String1++;
    String2++;
    Length--;
  }
  return *String1 - *String2;
}

/**
  Returns the first occurrence of a NULL-terminated ASCII string in a NULL-
  terminated ASCII string with a limit on the number of characters searched.

  @param  String        The pointer to a NULL-terminated string.
  @param  SearchString  The pointer to the NULL-terminated string to search for.
  @param  MaxLength     The maximum number of characters to search

  @return A pointer to the first character of the first occurrence of
          SearchString is returned, or NULL if SearchString is not found.
 */
STATIC
CHAR8*
EFIAPI
AsciiStrnStr(IN CHAR8 CONST *String,
             IN CHAR8 CONST *SearchString,
             IN UINTN        MaxLength)
{
  UINTN SearchStringLen;
  UINTN Idx;

  if(!SearchString || !(SearchStringLen = AsciiStrLen(SearchString)))
    return (char *)String;

  MaxLength = AsciiStrnLen(String, MaxLength);
  MaxLength = SearchStringLen < MaxLength? MaxLength - SearchStringLen : 0;

  for(Idx = 0; Idx < MaxLength; Idx++, String++)
    if(*String == *SearchString)
      if(!AsciiStrnCmp(String, SearchString, SearchStringLen))
        return (char *) String;
  return NULL;
}

/**
  Internal types
 */
typedef struct _CRYPTO_USER {
  UINT32   UserType;
  EFI_GUID UserIdent;
} CRYPTO_USER;

#define USER_TYPE_DISK      0x10000001
#define USER_TYPE_RECOVERY  0x10010005
#define USER_TYPE_REGULAR   0x10060002

/**
  Macro Definitions ...

  KeySize       Size (in bytes) of a plist key (<key>Name</key>)
  StartTagSize  Size (in bytes) of a start tag
  EndTagSize    Size (in bytes) of an end tag

  BeginsWithStartTag  Check whether buffer starts with a given tag

  FindKey       Find a plist key in the buffer
  FindStartTag  Find a start tag in the buffer
  FindEndTag    Find an end tag in the buffer
 */

#define KeySize(Key)      (sizeof("<key>"Key"</key>")-1)
#define StartTagSize(Tag) (sizeof("<"Tag">")-1)
#define EndTagSize(Tag)   (sizeof("</"Tag">")-1)

#define BeginsWithStartTag(Tag, String, Length) \
  !AsciiStrnCmp(String, "<"Tag">", StartTagSize(Tag))
#define FindKey(Key, String, Length) \
  AsciiStrnStr(String, "<key>"Key"</key>", Length)
#define FindStartTag(Tag, String, Length) \
  AsciiStrnStr(String, "<"Tag">", Length)
#define FindEndTag(Tag, String, Length) \
  AsciiStrnStr(String, "</"Tag">", Length)

#define TAG_ARRAY       "array"
#define TAG_DICT        "dict"
#define KEY_USERTYPE    "UserType"
#define KEY_USERIDENT   "UserIdent"
#define KEY_CRYPTOUSERS "CryptoUsers"

/**
  Locate the end of an array within the plist file.

  Nested arrays should be skipped.

  @param  Start   Pointer to the start of the array.
  @param  LEngth  The length of the buffer pointed to by Start.

  @return A pointer to the </array> tag or NULL if it could not be found.
 */
STATIC
CHAR8 *
EFIAPI
FindArrayEnd(IN CHAR8 CONST *Start,
             IN UINTN        Length)
{
  CHAR8 *End;
  if(BeginsWithStartTag(TAG_ARRAY, Start, Length)) {
    Start  += StartTagSize(TAG_ARRAY);
    Length -= StartTagSize(TAG_ARRAY);
  }
  do {
    if((End = FindEndTag(TAG_ARRAY, Start, Length))) {
      if((Start = FindStartTag(TAG_ARRAY, Start, End - Start))) {
        Start = FindArrayEnd(Start, Length - (End - Start));
        if(Start)
          Start += EndTagSize(TAG_ARRAY);
      }
    }
  } while(Start && End);
  return End;
}

/**
  Locate the end of a dict within the plist file.

  Nested dicts should be skipped.

  @param  Start   Pointer to the start of the dict.
  @param  Length  The length of the buffer pointed to by Start.

  @return A pointer to the </dict> tag or NULL if it could not be found.
 */
STATIC
CHAR8 *
EFIAPI
FindDictEnd(IN CHAR8 CONST *Start,
            IN UINTN        Length)
{
  CHAR8 *End;
  if(BeginsWithStartTag(TAG_DICT, Start, Length)) {
    Start  += StartTagSize(TAG_DICT);
    Length -= StartTagSize(TAG_DICT);
  }
  do {
    if((End = FindEndTag(TAG_DICT, Start, Length))) {
      if((Start = FindStartTag(TAG_DICT, Start, End - Start))) {
        Start = FindDictEnd(Start, Length - (End - Start));
        if(Start)
          Start += EndTagSize(TAG_DICT);
      }
    }
  } while(Start && End);
  return End;
}

/**
  Extract the UserType value from a CryptoUsers dict.

  @param  Dict    Pointer to the start of the CryptoUser dict.
  @param  Length  Length of the CryptoUser dict.

  @return The UserType value or 0 if the UserType could not be found.
 */
STATIC
UINT32
EFIAPI
GetUserType(IN CHAR8 CONST *Dict,
            IN UINTN        Length)
{
  UINT32  UserType = 0;
  CHAR8  *UserTypeString;
  if((UserTypeString = FindKey(KEY_USERTYPE, Dict, Length))) {
    UserTypeString += KeySize(KEY_USERTYPE);
    Length         -= UserTypeString - Dict + KeySize(KEY_USERTYPE);
    while(Length && '>' != *UserTypeString++)
      Length--;
    while(Length && '0' <= *UserTypeString && *UserTypeString <= '9') {
      UserType = 10 * UserType + (*UserTypeString++ -'0');
      Length--;
    }
  }
  return UserType;
}

/**
  Parse the string representation of a GUID.

  @param  String  Pointer to the string containing the GUID.
  @param  Length  Maximum length of the string.
  @param  Guid    Where to store the GUID.

  @return A BOOLEAN indicating whether parsing was successful or not.
 */
STATIC
BOOLEAN
EFIAPI
ParseGuid(IN  CHAR8 CONST *String,
          IN  UINTN        Length,
          OUT EFI_GUID    *Guid)
{
#define ParseHex(Var, CharCount)                                          \
  do {                                                                    \
    UINTN _Idx;                                                           \
    for((Var) = 0, _Idx = 0;                                              \
        _Idx < (CharCount) && Length;                                     \
        _Idx++, Length--, String++) {                                     \
      CHAR8 Char = *String;                                               \
      if('0' <= Char && Char <= '9')                                      \
        (Var) = ((Var) << 4) + (Char - '0');                              \
      else if('a' <= Char && Char <= 'f')                                 \
        (Var) = ((Var) << 4) + (Char - 'a' + 10);                         \
      else if('A' <= Char && Char <= 'F')                                 \
        (Var) = ((Var) << 4) + (Char - 'A' + 10);                         \
      else                                                                \
        return FALSE;                                                     \
    }                                                                     \
  } while(0)
#define SkipOptional(Char)                                                \
  do {                                                                    \
    if(Length && (Char) == *String) {                                     \
      String++;                                                           \
      Length--;                                                           \
    }                                                                     \
  }while(0)
#define Skip(Char)                                                        \
  do {                                                                    \
    if(Length && (Char) == *String) {                                     \
      String++;                                                           \
      Length--;                                                           \
    }                                                                     \
    else                                                                  \
      return FALSE;                                                       \
  } while(0)
  UINT32 Data1;
  UINT16 Data2;
  UINT16 Data3;
  UINT8  Data4[8];
  UINTN  Idx;
  SkipOptional('{');
  ParseHex(Data1, 8);
  Skip('-');
  ParseHex(Data2, 4);
  Skip('-');
  ParseHex(Data3, 4);
  Skip('-');
  for(Idx = 0; Idx < 2; Idx++)
    ParseHex(Data4[Idx], 2);
  Skip('-');
  for(Idx = 2; Idx < sizeof(Data4); Idx++)
    ParseHex(Data4[Idx], 2);
  SkipOptional('}');
  Guid->Data1 = Data1;
  Guid->Data2 = Data2;
  Guid->Data3 = Data3;
  for(Idx = 0; Idx < sizeof(Data4); Idx++)
    Guid->Data4[Idx] = Data4[Idx];
  return TRUE;
}

/**
  Extract the UserIdent GUID from a CryptoUsers dict.

  @param  Dict        Pointer to the start of the CryptoUser dict.
  @param  Length      Length of the CryptoUser dict.
  @param  UserIdent   Where to store the UserIdent GUID

  @return Pointer to the UserIdent GUID in the dictionary, or NULL if it was
          not found.
 */
STATIC
CHAR8 CONST *
EFIAPI
GetUserIdent(IN  CHAR8 CONST *Dict,
             IN  UINTN        Length,
             OUT EFI_GUID    *Guid)
{
  CHAR8 *UserIdentString;
  if((UserIdentString = FindKey(KEY_USERIDENT, Dict, Length))) {
    UserIdentString += KeySize(KEY_USERIDENT);
    Length          -= UserIdentString - Dict + KeySize(KEY_USERIDENT);
    while(Length && '>' != *UserIdentString++)
      Length--;
    return ParseGuid(UserIdentString, Length, Guid)? UserIdentString: NULL;
  }
  return NULL;
}

/**
  Determine whether the User is of interest.

  Currently, only a user of type USER_TYPE_DISK is accepted.

  @param  User  Pointer to the CRYPTO_USER to test.

  @return A BOOLEAN indicating whether the user is of interest.
 */
STATIC
BOOLEAN
KeepUser(IN CRYPTO_USER CONST *User)
{
  return USER_TYPE_DISK == User->UserType;
}

/**
  Filter the EncryptedRoot.plist.wipekey file.

  The EncryptedRoot.plist.wipekey file contains a CryptoUsers array that
  contains passphrase wrapped KEK structures for each user.  This function
  attempts to remove all but one of the users: the disk password.

  @param  Src   Pointer to the original (decrypted) contents of the
                EncryptedRoot.plist.wipekey file.
  @param  Size  Size of the buffer pointed to by Src.
  @param  Dest  Where to store the filtered version of the file.  This should
                be at least Size bytes and may point to the same buffer as Src.

  @return The number of bytes stored in Dest.
 */
UINTN
EFIAPI
PlistFilter(IN  CHAR8 CONST *Src,
            IN  UINTN        Size,
            OUT CHAR8       *Dest)
{
  CRYPTO_USER  User;
  CHAR8       *CryptoUserStart;
  CHAR8       *UserArrayStart;
  CHAR8       *UserArrayEnd;
  CHAR8       *UserDictStart;
  CHAR8       *UserDictEnd;
  if((CryptoUserStart = FindKey(KEY_CRYPTOUSERS, Src, Size))) {
    if((UserArrayStart = FindStartTag(TAG_ARRAY,
                                      CryptoUserStart,
                                      Size - (CryptoUserStart - Src))) &&
       (UserArrayEnd = FindArrayEnd(UserArrayStart,
                                    Size - (CryptoUserStart - Src)))) {
      UserDictStart = FindStartTag(TAG_DICT,
                                   UserArrayStart,
                                   UserArrayEnd - UserArrayStart);
      while(UserDictStart &&
            (UserDictEnd = FindDictEnd(UserDictStart,
                                       UserArrayEnd - UserDictStart))) {
        User.UserType = GetUserType(UserDictStart, UserDictEnd - UserDictStart);
        if(!GetUserIdent(UserDictStart,
                         UserDictEnd - UserDictStart,
                         &User.UserIdent)) {
          gBS->SetMem(&User.UserIdent, sizeof(User.UserIdent), 0);
        }
        if(KeepUser(&User)) {
          UINTN Length1 = UserArrayStart - Src + StartTagSize(TAG_ARRAY);
          UINTN Length2 = UserDictEnd - UserDictStart + EndTagSize(TAG_DICT);
          UINTN Length3 = Size - (UserArrayEnd - Src);
          gBS->CopyMem(Dest, (VOID*)Src, Length1);
          gBS->CopyMem(Dest + Length1, UserDictStart, Length2);
          gBS->CopyMem(Dest + Length1 + Length2, UserArrayEnd, Length3);
          return Length1 + Length2 + Length3;
        }
        UserDictStart = FindStartTag(TAG_DICT,
                                     UserDictEnd,
                                     UserArrayEnd - UserDictEnd);
      }
    }
  }
  gBS->CopyMem(Dest, (VOID*)Src, Size);
  return Size;
}
