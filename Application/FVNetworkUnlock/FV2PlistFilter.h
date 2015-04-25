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

#ifndef __FV2_PLIST_FILTER__
#define __FV2_PLIST_FILTER__

#include <Uefi.h>

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
            OUT CHAR8       *Dest);

#endif
