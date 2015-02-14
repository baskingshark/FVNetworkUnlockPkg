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
#include <Library/BaseMemoryLib.h>
#include "AesGeneric.h"

/**
 * Multiply an element of GF(2^128) by x.
 *
 * Note: this implementation works byte-by-byte so is not the most efficient
 * method.
 *
 * @param Src   Location of the element of GF(2^128), in little-endian format.
 * @param Dest  Where to store the result of multiplaction by x.  This can be
 *              the same location as Src.
 */
STATIC
VOID
EFIAPI
GfMul128(IN  UINT8 CONST Src[static 16],
         OUT UINT8       Dest[static 16])
{
  UINTN Carry = Src[15] >> 7;
  UINTN Idx;
  for(Idx = 15; Idx > 0; --Idx) {
    Dest[Idx] = (Src[Idx] << 1) | (Src[Idx-1] >> 7);
  }
  Dest[0] = (Src[0] << 1) ^ (Carry * 0x87);
}

/**
 * XOR two AES-sized (16 byte) blocks and store the results.
 *
 * Note: this implementation works byte-by-byte so is not the most efficient
 * method.
 *
 * @param Dest  Where to store the result of the XOR.  This can be the same as
 *              either Src1 or Src2.
 * @param Src1  Location of the first block.
 * @param Src2  Location of the second block.
 */
STATIC
VOID
EFIAPI
XorBlock(IN  UINT8 CONST Src1[16],
         IN  UINT8 CONST Src2[16],
         OUT UINT8       Dest[16])
{
  UINTN Idx;
  for(Idx = 0; Idx < 16; Idx++)
    (Dest)[Idx] = (Src1)[Idx] ^ (Src2)[Idx];
}

/**
 * XTS-AES encryption.
 *
 * @param KeySize   Size of the key in bits - must be 256 or 512.
 * @param Key       Pointer to the key - the first half is used to encrypt the
 *                  data, while the second half is used to encrypt the initial
 *                  tweak.
 * @param IV        Pointer to the tweak IV.
 * @param Size      Size of the data (in bytes) - must be at least 16 bytes.
 * @param Src       Pointer to the data.
 * @param Dest      Pointer to the location to store the encrypted data (this
 *                  may be the same as Src).
 *
 * @retval  EFI_SUCCESS             The data was encrypted.
 * @retval  EFI_INVALID_PARAMETER   One or more of the parameters were invalid.
 */
EFI_STATUS
EFIAPI
XtsAesCipher(IN  UINTN CONST KeySize,
             IN  UINT8 CONST Key[static KeySize/8],
             IN  UINT8 CONST IV[static 16],
             IN  UINTN CONST Size,
             IN  UINT8 CONST Src[static Size],
             OUT UINT8       Dest[static Size])
{
  UINT8 Buffer[16];
  UINT8 Tweak[16];
  UINTN FullBlockSize;
  UINTN PartBlockSize;
  UINTN Idx;
  UINTN Nk;

  if((256 != KeySize && 512 != KeySize) ||
     !Key || !IV || Size < 16 || !Src || !Dest)
    return EFI_INVALID_PARAMETER;

  FullBlockSize = Size - (Size % 16);
  Nk = KeySize / 64;
  Cipher(Nk, IV, Key + KeySize / 16, Tweak);
  for(Idx = 0; Idx < FullBlockSize; Idx += 16) {
    XorBlock(Src + Idx, Tweak, Buffer);
    Cipher(Nk, Buffer, Key, Buffer);
    XorBlock(Buffer, Tweak, Dest + Idx);
    GfMul128(Tweak, Tweak);
  }
  // Handle partial block with ciphertext stealing
  if(Idx < Size) {
    PartBlockSize = Size % 16;
    // Reamining plaintext
    CopyMem(Buffer, Src + Idx, PartBlockSize);
    // Move final block into place
    CopyMem(Dest + Idx, Dest + Idx - 16, PartBlockSize);
    // Stolen ciphertext
    CopyMem(Buffer + PartBlockSize,
            Dest + Idx - 16 + PartBlockSize,
            16 - PartBlockSize);
    // Encrypt penultimate block
    XorBlock(Buffer, Tweak, Buffer);
    Cipher(Nk, Buffer, Key, Buffer);
    XorBlock(Buffer, Tweak, Dest + Idx - 16);
  }

  return EFI_SUCCESS;
}

/**
 * XTS-AES decryption.
 *
 * @param KeySize   Size of the key in bits - must be 256 or 512.
 * @param Key       Pointer to the key - the first half is used to encrypt the
 *                  data, while the second half is used to encrypt the initial
 *                  tweak.
 * @param IV        Pointer to the tweak IV.
 * @param Size      Size of the data (in bytes) - must be at least 16 bytes.
 * @param Src       Pointer to the encrypted data.
 * @param Dest      Pointer to the location to store the decrypted data (this
 *                  may be the same as Src).
 *
 * @retval  EFI_SUCCESS             The data was decrypted.
 * @retval  EFI_INVALID_PARAMETER   One or more of the parameters were invalid.
 */
EFI_STATUS
EFIAPI
InvXtsAesCipher(IN  UINTN CONST KeySize,
                IN  UINT8 CONST Key[static KeySize/8],
                IN  UINT8 CONST IV[static 16],
                IN  UINTN CONST Size,
                IN  UINT8 CONST Src[static Size],
                OUT UINT8       Dest[static Size])
{
  UINT8 Buffer[16];
  UINT8 Tweak[16];
  UINTN FullBlockSize;
  UINTN PartBlockSize;
  UINTN Idx;
  UINTN Nk;

  if((256 != KeySize && 512 != KeySize) ||
     !Key || !IV || Size < 16 || !Src || !Dest)
    return EFI_INVALID_PARAMETER;

  FullBlockSize = Size % 16 ? Size - 16 - (Size % 16) : Size;
  Nk = KeySize / 64;

  Cipher(Nk, IV, Key + KeySize / 16, Tweak);
  for(Idx = 0; Idx < FullBlockSize; Idx += 16) {
    XorBlock(Src + Idx, Tweak, Buffer);
    InvCipher(Nk, Buffer, Key, Buffer);
    XorBlock(Buffer, Tweak, Dest + Idx);
    GfMul128(Tweak, Tweak);
  }
  // Handle partial block with ciphertext stealing
  if(Idx < Size) {
    UINT8 Tweak2[16];
    // With partial final block, penultimate block uses next tweak
    GfMul128(Tweak, Tweak2);
    XorBlock(Src + Idx, Tweak2, Buffer);
    InvCipher(Nk, Buffer, Key, Buffer);
    XorBlock(Buffer, Tweak2, Dest + Idx);
    // Shuffle data
    PartBlockSize = Size % 16;
    Idx += 16;
    // Reamining ciphertext
    CopyMem(Buffer, Src + Idx, PartBlockSize);
    // Stolen ciphertext
    CopyMem(Buffer + PartBlockSize,
            Dest + Idx - 16 + PartBlockSize,
            16 - PartBlockSize);
    // Move final block into place
    CopyMem(Dest + Idx, Dest + Idx - 16, PartBlockSize);
    // Decrypt penultimate block
    XorBlock(Buffer, Tweak, Buffer);
    InvCipher(Nk, Buffer, Key, Buffer);
    XorBlock(Buffer, Tweak, Dest + Idx - 16);
  }

  return EFI_SUCCESS;
}
