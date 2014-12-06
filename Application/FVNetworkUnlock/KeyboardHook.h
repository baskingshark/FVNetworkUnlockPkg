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

#ifndef __KEYBOARD_HOOK_H__
#define __KEYBOARD_HOOK_H__

#include <Uefi.h>

/**
  Hook console and key state protocols to return provided data.

  @param  Data        Data to be returned when keyboard is scanned.
  @param  DataLength  Length of data (in bytes) pointed to by Data.

  @retval EFI_SUCCESS           The Console and Key State protocol instance (if
                                it exists) were successfully replaced.
  @retval EFI_OUT_OF_RESOURCES  The replacement Console/Key State protocol
                                could not be created.
  @retval INVALID_PARAMETER     One or more of the parameters are invalid.
 */
EFI_STATUS
EFIAPI
HookKeyboard(IN CONST CHAR8* Data,
             IN CONST UINTN  DataLength);

/**
 Remove hooks on console and key state protocols.
 */
VOID
EFIAPI
UnhookKeyboard(VOID);

#endif
