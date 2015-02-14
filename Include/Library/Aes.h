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

#ifndef __AES_H__
#define __AES_H__

#include <Uefi.h>

/**
 * Implementation of the AES Cipher - see section 5.1 of FIPS 197.
 *
 * @param Keysize   Size of the key in bits - must be 128, 192 or 256
 * @param In        Data to encrypt (16 bytes)
 * @param Key       Key to use (must match size in keysize)
 * @param Out       Where to store the encrypted data
 *
 * @retval EFI_SUCCESS            The data was successfully encrypted
 * @retval EFI_INVALID_PARAMETER  One of the parameters was incorrect
 */

EFI_STATUS
EFIAPI
AesCipher(IN  UINTN CONST Keysize,
          IN  UINT8 CONST In[16],
          IN  UINT8 CONST Key[static Keysize/8],
          OUT UINT8       Out[16]);

/**
 * Implementation of the Inverse AES Cipher - see section 5.3 of FIPS 197.
 *
 * @param keysize   Size of the key in bits - must be 128, 192 or 256
 * @param in        Data to decrypt (16 bytes)
 * @param key       Key to use (must match size in keysize)
 * @param out       Where to store the decrypted data
 *
 * @retval EFI_SUCCESS            The data was successfully encrypted
 * @retval EFI_INVALID_PARAMETER  One of the parameters was incorrect
 */
EFI_STATUS
EFIAPI
InvAesCipher(IN  UINTN CONST Keysize,
             IN  UINT8 CONST In[16],
             IN  UINT8 CONST Key[static Keysize/8],
             OUT UINT8       Out[16]);

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
             OUT UINT8       Dest[static Size]);

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
                OUT UINT8       Dest[static Size]);

#endif
