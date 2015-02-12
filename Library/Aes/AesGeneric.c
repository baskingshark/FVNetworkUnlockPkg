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

/**
 * This is an implementation of the AES algorithm described in FIPS 197.
 */
#include <Uefi.h>

/**
 * Constants:
 *  Nb: Number of columns (32-bit words) comprising the State.
 *      For AES, this is constant (4)
 *  Nk: Number of 32-bit words compising the Cipher Key.
 *      This can be 4, 6 or 8 for 128 bit, 192 bit or 256 bit respectively.
 *  Nr: Number of rounds, which is a function of Nk and Nb.
 *      This is 10, 12 or 14 for 128 bit, 192 bit or 256 bit respectively.
 */
#define Nb 4
#define NkFromKeySize(x)  ((x)/32)
#define NrFromNk(x)       ((x)+6)

/**
 * S-box substitution values - See section 5.1.1 of FIPS 197.
 */
STATIC
UINT8
CONST
SBox[256] = {
  0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
  0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
  0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
  0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
  0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
  0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
  0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
  0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
  0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
  0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
  0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
  0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
  0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
  0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
  0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
  0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16
};

/**
 * Inverse S-box substitution values - See section 5.3.2 of FIPS 197.
 */
STATIC
UINT8
CONST
InvSBox[256] = {
  0x52, 0x09, 0x6a, 0xd5, 0x30, 0x36, 0xa5, 0x38, 0xbf, 0x40, 0xa3, 0x9e, 0x81, 0xf3, 0xd7, 0xfb,
  0x7c, 0xe3, 0x39, 0x82, 0x9b, 0x2f, 0xff, 0x87, 0x34, 0x8e, 0x43, 0x44, 0xc4, 0xde, 0xe9, 0xcb,
  0x54, 0x7b, 0x94, 0x32, 0xa6, 0xc2, 0x23, 0x3d, 0xee, 0x4c, 0x95, 0x0b, 0x42, 0xfa, 0xc3, 0x4e,
  0x08, 0x2e, 0xa1, 0x66, 0x28, 0xd9, 0x24, 0xb2, 0x76, 0x5b, 0xa2, 0x49, 0x6d, 0x8b, 0xd1, 0x25,
  0x72, 0xf8, 0xf6, 0x64, 0x86, 0x68, 0x98, 0x16, 0xd4, 0xa4, 0x5c, 0xcc, 0x5d, 0x65, 0xb6, 0x92,
  0x6c, 0x70, 0x48, 0x50, 0xfd, 0xed, 0xb9, 0xda, 0x5e, 0x15, 0x46, 0x57, 0xa7, 0x8d, 0x9d, 0x84,
  0x90, 0xd8, 0xab, 0x00, 0x8c, 0xbc, 0xd3, 0x0a, 0xf7, 0xe4, 0x58, 0x05, 0xb8, 0xb3, 0x45, 0x06,
  0xd0, 0x2c, 0x1e, 0x8f, 0xca, 0x3f, 0x0f, 0x02, 0xc1, 0xaf, 0xbd, 0x03, 0x01, 0x13, 0x8a, 0x6b,
  0x3a, 0x91, 0x11, 0x41, 0x4f, 0x67, 0xdc, 0xea, 0x97, 0xf2, 0xcf, 0xce, 0xf0, 0xb4, 0xe6, 0x73,
  0x96, 0xac, 0x74, 0x22, 0xe7, 0xad, 0x35, 0x85, 0xe2, 0xf9, 0x37, 0xe8, 0x1c, 0x75, 0xdf, 0x6e,
  0x47, 0xf1, 0x1a, 0x71, 0x1d, 0x29, 0xc5, 0x89, 0x6f, 0xb7, 0x62, 0x0e, 0xaa, 0x18, 0xbe, 0x1b,
  0xfc, 0x56, 0x3e, 0x4b, 0xc6, 0xd2, 0x79, 0x20, 0x9a, 0xdb, 0xc0, 0xfe, 0x78, 0xcd, 0x5a, 0xf4,
  0x1f, 0xdd, 0xa8, 0x33, 0x88, 0x07, 0xc7, 0x31, 0xb1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xec, 0x5f,
  0x60, 0x51, 0x7f, 0xa9, 0x19, 0xb5, 0x4a, 0x0d, 0x2d, 0xe5, 0x7a, 0x9f, 0x93, 0xc9, 0x9c, 0xef,
  0xa0, 0xe0, 0x3b, 0x4d, 0xae, 0x2a, 0xf5, 0xb0, 0xc8, 0xeb, 0xbb, 0x3c, 0x83, 0x53, 0x99, 0x61,
  0x17, 0x2b, 0x04, 0x7e, 0xba, 0x77, 0xd6, 0x26, 0xe1, 0x69, 0x14, 0x63, 0x55, 0x21, 0x0c, 0x7d
};

/**
 * SubWord implementation - See section 5.2 of FIPS 197.
 */
STATIC
UINT32
EFIAPI
SubWord(IN UINT32 Word) {
  UINT32 Result = 0;
  UINTN  Idx;
  for(Idx = 0; Idx < sizeof(Word); Idx++)
    Result = (Result << 8) | SBox[(Word >> (24 - 8*Idx)) & 0xff];
  return Result;
}

/**
 * KeyExpansion implementation - See section 5.2 of FIPS 197.
 */
STATIC
VOID
EFIAPI
KeyExpansion(IN  UINT8 CONST* Key,
             IN  UINTN CONST  Nk,
             IN  UINTN CONST  Nr,
             OUT UINT32*      W) {
  STATIC UINT8 CONST RConBytes[] = {
    0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36
  };
#define RotWord(x) ((((UINT32)(x)) >> 24) | ((x) << 8))
#define Rcon(x) (((UINT32)RConBytes[(x)-1]) << 24)

  UINTN i;
  for(i = 0; i < Nk; i++) {
    W[i] = (Key[4*i+0] << 24) | (Key[4*i+1] << 16) |
    (Key[4*i+2] <<  8) | (Key[4*i+3] <<  0);
  }
  for(; i < Nb * (Nr+1); i++) {
    UINT32 temp = W[i-1];
    if(i % Nk == 0)
      temp = SubWord(RotWord(temp)) ^ Rcon(i/Nk);
    else if(Nk > 6 && (i % Nk == 4))
      temp = SubWord(temp);
    W[i] = W[i - Nk] ^ temp;
  }
#undef Rcon
#undef RotWord
}

typedef UINT8 State[4][Nb];

/**
 * SubBytes implementation - see section 5.1.1 of FIPS 197
 */
STATIC
VOID
EFIAPI
SubBytes(IN OUT State* S) {
  UINTN i;

  for(i = 0; i < sizeof(State); i++)
    ((UINT8*)*S)[i] = SBox[((UINT8*)*S)[i]];
}

/**
 * InvSubBytes implementation - see section 5.3.2 of FIPS 197
 */
STATIC
VOID
EFIAPI
InvSubBytes(IN OUT State* S) {
  UINTN i;
  for(i = 0; i < sizeof(State); i++)
    ((UINT8*)*S)[i] = InvSBox[((UINT8*)*S)[i]];
}

/**
 * ShiftRows implementation - see section 5.1.2 of FIPS 197
 */
STATIC
VOID
EFIAPI
ShiftRows(IN OUT State* S) {
  UINT8 temp;
  // Row 1, shift 1 left
  temp = (*S)[1][0];
  (*S)[1][0] = (*S)[1][1];
  (*S)[1][1] = (*S)[1][2];
  (*S)[1][2] = (*S)[1][3];
  (*S)[1][3] = temp;
  // Row 2, shift 2 left (swap 0 & 2, 1 & 3)
  temp = (*S)[2][0];
  (*S)[2][0] = (*S)[2][2];
  (*S)[2][2] = temp;
  temp = (*S)[2][1];
  (*S)[2][1] = (*S)[2][3];
  (*S)[2][3] = temp;
  // Row 3, shift 3 left (shift 1 right)
  temp = (*S)[3][3];
  (*S)[3][3] = (*S)[3][2];
  (*S)[3][2] = (*S)[3][1];
  (*S)[3][1] = (*S)[3][0];
  (*S)[3][0] = temp;
}

/**
 * InvShiftRows implementation - see section 5.3.1 of FIPS 197
 */
STATIC
VOID
EFIAPI
InvShiftRows(IN OUT State *S) {
  UINT8 temp;
  // Row 1, shift 1 right
  temp = (*S)[1][3];
  (*S)[1][3] = (*S)[1][2];
  (*S)[1][2] = (*S)[1][1];
  (*S)[1][1] = (*S)[1][0];
  (*S)[1][0] = temp;
  // Row 2, shift 2 right (swap 0 & 2, 1 & 3)
  temp = (*S)[2][0];
  (*S)[2][0] = (*S)[2][2];
  (*S)[2][2] = temp;
  temp = (*S)[2][1];
  (*S)[2][1] = (*S)[2][3];
  (*S)[2][3] = temp;
  // Row 3, shift 3 right (shift 1 left)
  temp = (*S)[3][0];
  (*S)[3][0] = (*S)[3][1];
  (*S)[3][1] = (*S)[3][2];
  (*S)[3][2] = (*S)[3][3];
  (*S)[3][3] = temp;
}


#define xtime(x) (((x)<<1) ^ (((x>>7) & 1) ? 0x1b : 0))
#define multiply(x,y) (\
(((y>>0) & 1) * (x)) ^\
(((y>>1) & 1) * xtime(x)) ^\
(((y>>2) & 1) * xtime(xtime(x))) ^\
(((y>>3) & 1) * xtime(xtime(xtime(x)))) ^\
(((y>>4) & 1) * xtime(xtime(xtime(xtime(x))))))

/**
 * MixColumns implementation - see section 5.1.3 of FIPS 197
 */
STATIC
VOID
EFIAPI
MixColumns(IN OUT State *S) {
  // s'(0,c) = ({02}•s(0,c))+({03}•s(1,c))+s(2,c)+s(3,c)
  // s'(1,c) = s(0,c)+({02}•s(1,c))+({03}•s(2,c))+s(3,c)
  // s'(2,c) = s(0,c)+s(1,c)+({02}•s(2,c))+({03}•s(3,c))
  // s'(4,c) = ({03}•s(0,c))+s(1,c)+s(2,c)+({02}•s(3,c))
  //
  // Becomes ...
  //
  // s'(0,c) = s(0,c)+s(1,c)+s(2,c)+s(3,c) + ({02}•(s(0,c)+s(1,c))) - s(0,c)
  // s'(1,c) = s(0,c)+s(1,c)+s(2,c)+s(3,c) + ({02}•(s(1,c)+s(2,c))) - s(1,c)
  // s'(2,c) = s(0,c)+s(1,c)+s(2,c)+s(3,c) + ({02}•(s(2,c)+s(3,c))) - s(2,c)
  // s'(4,c) = s(0,c)+s(1,c)+s(2,c)+s(3,c) + ({02}•(s(0,c)+s(3,c))) - s(3,c)

  UINTN c;
  for(c = 0; c < 4; c++) {
    UINT8 All = (*S)[0][c] ^ (*S)[1][c] ^ (*S)[2][c] ^ (*S)[3][c];
    UINT8 S0c = (*S)[0][c];
    UINT8 Sq;

    // s'(0,c) = ...
    Sq = (*S)[0][c] ^ (*S)[1][c];
    Sq = xtime(Sq);
    (*S)[0][c] ^= All ^ Sq;
    // s'(1,c) = ...
    Sq = (*S)[1][c] ^ (*S)[2][c];
    Sq = xtime(Sq);
    (*S)[1][c] ^= All ^ Sq;
    // s'(2,c) = ...
    Sq = (*S)[2][c] ^ (*S)[3][c];
    Sq = xtime(Sq);
    (*S)[2][c] ^= All ^ Sq;
    // s'(3,c) = ...
    Sq = (*S)[3][c] ^ S0c;
    Sq = xtime(Sq);
    (*S)[3][c] ^= All ^ Sq;
  }
}

/**
 * InvMixColumns implementation - see section 5.3.3 of FIPS 197
 */
STATIC
VOID
EFIAPI
InvMixColumns(IN OUT State *S) {
  // s'(0,c) = ({0e}•s(0,c))+({0b}•s(1,c))+({0d}•s(2,c))+({09}•s(3,c))
  // s'(1,c) = ({09}•s(0,c))+({0e}•s(1,c))+({0b}•s(2,c))+({0d}•s(3,c))
  // s'(2,c) = ({0d}•s(0,c))+({09}•s(1,c))+({0e}•s(2,c))+({0b}•s(3,c))
  // s'(4,c) = ({0b}•s(0,c))+({0d}•s(1,c))+({09}•s(2,c))+({0e}•s(3,c))

  UINTN c;
  for(c = 0; c < 4; c++) {
    UINT8 S0c = (*S)[0][c];
    UINT8 S1c = (*S)[1][c];
    UINT8 S2c = (*S)[2][c];
    UINT8 S3c = (*S)[3][c];

    // s'(0,c) = ...
    (*S)[0][c] = (multiply(S0c, 0x0e) ^
                  multiply(S1c, 0x0b) ^
                  multiply(S2c, 0x0d) ^
                  multiply(S3c, 0x09));
    // s'(1,c) = ...
    (*S)[1][c] = (multiply(S0c, 0x09) ^
                  multiply(S1c, 0x0e) ^
                  multiply(S2c, 0x0b) ^
                  multiply(S3c, 0x0d));
    // s'(2,c) = ...
    (*S)[2][c] = (multiply(S0c, 0x0d) ^
                  multiply(S1c, 0x09) ^
                  multiply(S2c, 0x0e) ^
                  multiply(S3c, 0x0b));
    // s'(3,c) = ...
    (*S)[3][c] = (multiply(S0c, 0x0b) ^
                  multiply(S1c, 0x0d) ^
                  multiply(S2c, 0x09) ^
                  multiply(S3c, 0x0e));
  }
}

/**
 * AddRoundKey Implementation - see section 5.1.4 of FIPS 197
 */
STATIC
VOID
EFIAPI
AddRoundKey(IN OUT State*       S,
            IN     UINT32 CONST W[4]) {
  UINTN i;
  UINTN j;
  for(i = 0; i < 4; i++)
    for(j = 0; j < 4; j++)
      (*S)[j][i] ^= (W[i] >> (24 - 8*j)) & 0xff;
}

/**
 * Implementation of the AES Cipher - see section 5.1 of FIPS 197.
 *
 * @param Nk    Size of the key in words - must be 4, 6 or 8.
 * @param In    Data to encrypt (16 bytes)
 * @param Key   Key to use (must match size in keysize)
 * @param Out   Where to store the encrypted data
 */
VOID
EFIAPI
Cipher(IN  UINTN CONST Nk,
       IN  UINT8 CONST In[16],
       IN  UINT8 CONST Key[static 4*Nk],
       OUT UINT8       Out[16]) {
  State  State;
  UINT32 W[60]; // Maximum of 14 rounds => maximum of 60 words of key
  UINTN  Nr;    // Number of rounds
  UINTN  Idx;
  UINTN  Round;

  Nr = NrFromNk(Nk);

  KeyExpansion(Key, Nk, Nr, W);

  for(Idx = 0; Idx < 16; Idx++)
    State[Idx % 4][Idx / 4] = In[Idx];

  AddRoundKey(&State, W);
  for(Round = 1; Round < Nr; Round++) {
    SubBytes(&State);
    ShiftRows(&State);
    MixColumns(&State);
    AddRoundKey(&State, W + 4*Round);
  }

  SubBytes(&State);
  ShiftRows(&State);
  AddRoundKey(&State, W + 4*Nr);

  for(Idx = 0; Idx < 16; Idx++)
    Out[Idx] = State[Idx % 4][Idx/4];
}

/**
 * Implementation of the AES Cipher - see section 5.1 of FIPS 197.
 *
 * @param keysize   Size of the key in bits - must be 128, 192 or 256
 * @param in        Data to encrypt (16 bytes)
 * @param key       Key to use (must match size in keysize)
 * @param out       Where to store the encrypted data
 *
 * @retval EFI_SUCCESS            The data was successfully encrypted
 * @retval EFI_INVALID_PARAMETER  One of the parameters was incorrect
 */
EFI_STATUS
EFIAPI
AesCipher(IN  UINTN CONST Keysize,
          IN  UINT8 CONST In[16],
          IN  UINT8 CONST Key[static Keysize/8],
          OUT UINT8       Out[16]) {
  if(128 != Keysize && 192 != Keysize && 256 != Keysize)
    return EFI_INVALID_PARAMETER;

  Cipher(NkFromKeySize(Keysize), In, Key, Out);

  return EFI_SUCCESS;
}

/**
 * Implementation of the AES Cipher - see section 5.1 of FIPS 197.
 *
 * @param Nk    Size of the key in words - must be 4, 6 or 8.
 * @param In    Data to encrypt (16 bytes)
 * @param Key   Key to use (must match size in keysize)
 * @param Out   Where to store the encrypted data
 */
VOID
EFIAPI
InvCipher(IN  CONST UINTN Nk,
          IN  CONST UINT8 In[16],
          IN  CONST UINT8 Key[static 4*Nk],
          OUT       UINT8 Out[16]) {
  State  State;
  UINT32 W[60]; // Maximum of 14 rounds => maximum of 60 words of key
  UINTN  Nr;    // Number of rounds
  UINTN  Idx;
  UINTN  Round;

  Nr = NrFromNk(Nk);

  KeyExpansion(Key, Nk, Nr, W);

  for(Idx = 0; Idx < 16; Idx++)
    State[Idx % 4][Idx / 4] = In[Idx];

  AddRoundKey(&State, W + 4*Nr);
  for(Round = Nr - 1; Round > 0; Round--) {
    InvShiftRows(&State);
    InvSubBytes(&State);
    AddRoundKey(&State, W + 4*Round);
    InvMixColumns(&State);
  }

  InvShiftRows(&State);
  InvSubBytes(&State);
  AddRoundKey(&State, W);

  for(Idx = 0; Idx < 16; Idx++)
    Out[Idx] = State[Idx % 4][Idx/4];
}

/**
 * Implementation of the Inverse AES Cipher - see section 5.3 of FIPS 197.
 *
 * @param Keysize   Size of the key in bits - must be 128, 192 or 256
 * @param In        Data to decrypt (16 bytes)
 * @param Key       Key to use (must match size in keysize)
 * @param Out       Where to store the decrypted data
 *
 * @retval EFI_SUCCESS            The data was successfully encrypted
 * @retval EFI_INVALID_PARAMETER  One of the parameters was incorrect
 */
EFI_STATUS
EFIAPI
InvAesCipher(IN  CONST UINTN Keysize,
             IN  CONST UINT8 In[16],
             IN  CONST UINT8 Key[static Keysize/8],
             OUT       UINT8 Out[16]) {
  if(128 != Keysize && 192 != Keysize && 256 != Keysize)
    return EFI_INVALID_PARAMETER;

  InvCipher(NkFromKeySize(Keysize), In, Key, Out);

  return EFI_SUCCESS;
}
