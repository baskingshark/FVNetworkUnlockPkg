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

#ifndef __SPLASHSCREEN_H__
#define __SPLASHSCREEN_H__

#include <Uefi.h>

/**
  Display a splash screen.

  The splash screen is passed in PNG format.  Any valid PNG file should be
  supported.

  @param  Buffer      A pointer to a buffer containing a PNG file.
  @param  BufferSize  Size of the buffer containing the PNG file.

  @retval EFI_SUCCESS           The splash screen was drawn successfully.
  @retval EFI_UNSUPPORTED       The PNG image is not valid/supported.
  @retval EFI_NOT_FOUND         The Graphics Output Protocol/UGA Draw Protocol/
                                Console Control Protocol was not found.
  @retval EFI_OUT_OF_RESOURCES  There was insufficient memory to decode the
                                image
  @retval EFI_DEVCE_ERROR       The display device had an error and could not
                                display the image
 */
EFI_STATUS
EFIAPI
ShowSplashScreen(IN VOID  *Buffer,
                 IN UINTN  BufferSize);

/**
  Hide the splash screen.

  Attempt to hide the splash screen.  Currently, this is only possible if the
  Console Control Protocol is supported.

  @retval EFI_SUCCESS           The splash screen was hidden successfully
  @retval EFI_NOT_FOUND         The Console Control protocol was not found
 **/
EFI_STATUS
EFIAPI
HideSplashScreen();

#endif
