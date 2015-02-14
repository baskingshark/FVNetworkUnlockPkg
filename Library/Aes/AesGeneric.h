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

#ifndef __AES_GENERIC_H__
#define __AES_GENERIC_H__

#include <Uefi.h>

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
       OUT UINT8       Out[16]);

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
          OUT       UINT8 Out[16]);

#endif
