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

#ifndef __FIXED_TEXT_INPUT_H__
#define __FIXED_TEXT_INPUT_H__

#include <Uefi.h>
#include <Protocol/SimpleTextIn.h>

/**
  Create a Simple Text Input instance that returns a fixed input.

  @param  Data          The data that should be returned by the Simple Text
                        Input protocol
  @param  Length        The length of the sequence pointed to by Data
  @param  SimpleTextIn  A Pointer to where the allocated Simple Text Input
                        should be stored
  @param  Handle        Where the handle on which the instance has been
                        installed should be stored

  @retval EFI_SUCCESS             The protocol instance was successfully created
  @retval EFI_OUT_OF_RESOURCES    The protocol instance could not be allocated
  @retval EFI_INVALID_PARAMETER   One or more of the parameters are invalid
 */
EFI_STATUS
EFIAPI
CreateFixedTextInput(IN  CONST CHAR8                     *Data,
                     IN  CONST UINTN                      Length,
                     OUT EFI_SIMPLE_TEXT_INPUT_PROTOCOL **SimpleTextIn,
                     OUT EFI_HANDLE                      *Handle);

/**
  Release a Simple Text Input instance.

  @param  SimpleTextIn   The instance to free

  @retval EFI_SUCCESS             The protocol instance was successfully freed
  @retval EFI_NOT_FOUND           The protocol instance was not found
  @retval EFI_ACCESS_DENIED       The protocol instance is in-use
  @retval EFI_INVALID_PARAMETER   SimpleTextIn is NULL
 */
EFI_STATUS
EFIAPI
FreeFixedTextInput(IN EFI_SIMPLE_TEXT_INPUT_PROTOCOL *SimpleTextIn);

#endif
