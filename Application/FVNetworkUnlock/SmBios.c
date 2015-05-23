/**
 * Copyright (c) 2014, baskingshark
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
#include <Guid/SmBios.h>
#include <IndustryStandard/SmBios.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiLib.h>

/**
  Locate the SMBIOS Structure Table.
 
  @param  Table               A pointer to the SMBIOS Structure Table.
  @param  TableSize           The total length of the SMBIOS Structure Table
                              in bytes.
  @param  NumberOfStructure   Number of structures present in the SMBIOS
                              Structure Table.
 
  @retval EFI_SUCCESS         The SMBIOS Structure Table was found.
  @retval EFI_NOT_FOUND       The SMBIOS Entry Point structure was not found or
                              there was a problem validating it.
 */
STATIC
EFI_STATUS
EFIAPI
FindSmBiosTables(OUT SMBIOS_STRUCTURE **Table,
                 OUT UINTN             *TableSize,
                 OUT UINTN             *NumberOfStructures)
{
  SMBIOS_TABLE_ENTRY_POINT *SmBiosEntry;
  EFI_STATUS                Status;
  UINTN                     Idx;
  UINT8                     Sum;

  Status = EfiGetSystemConfigurationTable(&gEfiSmbiosTableGuid,
                                          (VOID**)&SmBiosEntry);
  if(!EFI_ERROR(Status)) {
    // Validate SmBios Entry Point
    if(CompareMem(&SmBiosEntry->AnchorString,
                  "_SM_",
                  sizeof(&SmBiosEntry->AnchorString))) {
      for(Sum = 0, Idx = 0; Idx < SmBiosEntry->EntryPointLength; Idx++)
        Sum += ((UINT8*)SmBiosEntry)[Idx];
      if(!Sum) {
        // Double cast is needed to convert 32-bit pointers to 64-bit pointers
        *Table = (SMBIOS_STRUCTURE*)(UINTN)SmBiosEntry->TableAddress;
        *TableSize = SmBiosEntry->TableLength;
        *NumberOfStructures = SmBiosEntry->NumberOfSmbiosStructures;
        Status = EFI_SUCCESS;
      }
      else {
        Print(L"Invalid checksum on SMBIOS Entry Point structure\n");
        Status = EFI_NOT_FOUND;
      }
    }
    else {
      Print(L"Invalid anchor string on SMBIOS Entry Point structure\n");
      Status = EFI_NOT_FOUND;
    }
  }
  return Status;
}

/**
  Find the next SMBIOS structure in the SMBION Structure table.
 
  @param  Structure   Pointer to current SMBIOS Structure
  @param  MaxLength   Size of the buffer containing this and future structures
 
  @return Pointer to the next structure, or NULL if there was a problem.
 */
STATIC
SMBIOS_STRUCTURE*
EFIAPI
FindNextSmBiosStructure(IN SMBIOS_STRUCTURE *Structure,
                        IN UINTN             MaxLength)
{
  SMBIOS_STRUCTURE *Next;
  CHAR8            *Current;
  CHAR8            *End;

  if(MaxLength < sizeof(SMBIOS_STRUCTURE))
    return NULL;

  Current = (CHAR8*)Structure + Structure->Length;
  End     = (CHAR8*)Structure + MaxLength - 1;
  Next    = NULL;
  while(Current < End) {
    // Skip string table
    if((Current = ScanMem8(Current, End-Current, 0)) &&
       (End - Current > 0) &&
       (!*++Current)) {
      // Found end of structure
      Next = (SMBIOS_STRUCTURE*)(Current + 1);
      break;
    }
  }
  return Next;
}

/**
  Find the requested SMBIOS structure of the specified type.
  
  @param  Type    The SMBIOS structure type to return.
  @param  Index   Which instance of the reuested type to return.
  @param  Struct  Pointer to the requested SMBIOS structure.
  @param  Size    Size of the SMBIOS structure.
 
  @retval EFI_SUCCESS     The SMBIOS structure was found.
  @retval EFI_NOT_FOUND   The requested SMBIOS structure was not found.
 */
EFI_STATUS
EFIAPI
FindSmBiosTableIndex(IN  UINT8              Type,
                     IN  UINTN              Index,
                     OUT SMBIOS_STRUCTURE **Struct,
                     OUT UINTN             *Size)
{
  SMBIOS_STRUCTURE *Table;
  SMBIOS_STRUCTURE *NextTable;
  UINTN             TableSize;
  UINTN             StructureCount;
  UINTN             Idx;
  EFI_STATUS        Status;

#ifdef __GNUC__
  // Work around GCC issue (at least in 4.3.0):
  // If FindSmBiosTables reports an error it doesn't initialize Table,
  // TableSize and StructureCount.  And we don't use them.
  // But GCC thinks we do.
  Table          = NULL;
  TableSize      = 0;
  StructureCount = 0;
#endif
  Status = FindSmBiosTables(&Table, &TableSize, &StructureCount);
  if(!EFI_ERROR(Status)) {
    Status = EFI_NOT_FOUND;
    for(Idx = 0; Idx < StructureCount; Idx++) {
      if(!(NextTable = FindNextSmBiosStructure(Table, TableSize))) {
        // No end of table ... bail out now
        break;
      }
      if(Table->Type == Type && !Index--) {
        // Found required table
        *Struct = Table;
        *Size = (UINT8*)NextTable - (UINT8*)Table;
        Status = EFI_SUCCESS;
        break;
      }
      TableSize -= (UINT8*)NextTable - (UINT8*)Table;
      Table = NextTable;
    }
  }
  else
    Print(L"Failed to find SMBIOS tables\n");
  return Status;
}

/**
  Find the first SMBIOS structure of the specified type.

  @param  Type    The SMBIOS structure type to return.
  @param  Struct  Pointer to the requested SMBIOS structure.
  @param  Size    Size of the SMBIOS structure.

  @retval EFI_SUCCESS     The SMBIOS structure was found.
  @retval EFI_NOT_FOUND   The requested SMBIOS structure was not found.
 */
EFI_STATUS
EFIAPI
FindSmBiosTable(IN  UINT8              Type,
                OUT SMBIOS_STRUCTURE **Struct,
                OUT UINTN             *Size)
{
  return FindSmBiosTableIndex(Type, 0, Struct, Size);
}

/**
  Find the requested string in the SMBIOS structure.
 
  @param  Struct  Pointer to the SMBIOS structure.
  @param  Size    Size of the SMBIOS structure.
  @param  Index   String index.
 
  @return A pointer to the string or NULL if the string was not found.
 */
CHAR8*
EFIAPI
FindSmBiosString(IN SMBIOS_STRUCTURE     *Struct,
                 IN UINTN                 Size,
                 IN SMBIOS_TABLE_STRING   Index)
{
  CHAR8 *Start;
  CHAR8 *End;
  CHAR8 *Result;
  CHAR8 *Next;
  UINTN  Idx;

  if(!Index || sizeof(SMBIOS_STRUCTURE) > Size || Struct->Length > Size)
    return NULL;

  Start  = (CHAR8*)Struct + Struct->Length;
  Result = NULL;
  End    = (CHAR8*)Struct + Size;
  Idx    = 0;
  while(Start && Start < End && Idx++ < Index) {
    if((Next = ScanMem8(Start, End-Start, 0)) && Idx == Index) {
      Result = Start;
      break;
    }
    Start = Next+1;
  }
  return Result;
}

/**
  Minimum size of a type 1 SMBIOS table if it contains a SerialNumber field.
 */
#define MIN_TYPE1_WITH_SN \
  (OFFSET_OF(SMBIOS_TABLE_TYPE1, SerialNumber) + sizeof(SMBIOS_TABLE_STRING))

/**
  Get serial number from SMBIOS tables.

  @return A pointer to the serial number string, or NULL if the serial number
          was not found.
*/
CHAR8*
EFIAPI
GetSerialNumber()
{
  SMBIOS_STRUCTURE    *Table;
  SMBIOS_TABLE_STRING  SerialNumberId;
  UINTN                Size;
  CHAR8               *SerialNumber;
  EFI_STATUS           Status;

  Status = FindSmBiosTable(1, &Table, &Size);
  if(!EFI_ERROR(Status) && Table->Length > MIN_TYPE1_WITH_SN) {
    SerialNumberId = ((SMBIOS_TABLE_TYPE1*)Table)->SerialNumber;
    SerialNumber   = FindSmBiosString(Table, Size, SerialNumberId);
  }
  else
    SerialNumber = NULL;
  return SerialNumber;
}