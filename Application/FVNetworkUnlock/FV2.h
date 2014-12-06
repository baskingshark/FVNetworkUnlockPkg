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

#ifndef __FV2_H__
#define __FV2_H__

#include <Uefi.h>

typedef struct _BOOT_LOADER {
  EFI_HANDLE                VolumeHandle;
  EFI_DEVICE_PATH_PROTOCOL *BootLoader;
} BOOT_LOADER;

/**
  Locate FV2 boot loaders.

  @param  BootLoaderCount   Location to store the number of boot loaders found.
  @param  BootLoaders       Location to store pointer to list of boot loaders.

  @retval EFI_SUCCESS             The search was successful.
  @retval EFI_NOT_FOUND           No boot loaders were found.
  @retval EFI_OUT_OF_RESOURCES    There is not enough memory to complete the
                                  search.
  @retval EFI_INVALID_PARAMETER   One or more of the parameters was invalid.
 */
EFI_STATUS
EFIAPI
LocateFV2BootLoaders(OUT UINTN        *BootLoaderCount,
                     OUT BOOT_LOADER **BootLoaders);

/**
  Free list of FV2 boot loaders.

  @param  BootLoaderCount   Number of boot loaders returned by
                            LocateFV2BootLoaders.
  @param  BootLoaders       Pointer to boot loaders returned by
                            LocateFV2BootLoaders.
 */
VOID
EFIAPI
FreeFV2BootLoaders(IN UINTN        BootLoaderCount,
                   IN BOOT_LOADER *BootLoaders);

#endif
