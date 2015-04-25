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

#ifndef __FV2_HOOK_H__
#define __FV2_HOOK_H__

#include <Uefi.h>
#include "FV2.h"

/**
  Hook file activity on the specified FV2_VOLUME.

  This allows filtering of the EncryptedRoot.plist.wipekey file and replacement
  of efires files.

  @param  Volume  The FileVault 2 volume to monitor.

  @return EFI_SUCCESS           The file system was successfully hooked.
  @return EFI_INVALID_PARAMETER One or more of the parameters are invalid.
  @return EFI_UNSUPPORTED       The provided handle does not support the Simple
                                File System Protocol.
  @return EFI_ACCESS_DENIED     The existing file system is in use.
  @return EFI_OUT_OF_RESOURCES  The file system could not be hooked due to a
                                lack of resources.
 */
EFI_STATUS
EFIAPI
HookVolume(IN FV2_VOLUME *Volume);

#endif
