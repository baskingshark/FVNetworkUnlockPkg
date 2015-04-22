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
#include <Protocol/DevicePath.h>

typedef struct _FV2_VOLUME {
  EFI_HANDLE                CSVolumeHandle;
  EFI_HANDLE                BootVolumeHandle;
  EFI_DEVICE_PATH_PROTOCOL *BootLoaderDevPath;
} FV2_VOLUME;

/**
  Locate FV2 volumes.

  @param  VolumeCount   Location to store the number of volumes found.
  @param  Volumes       Location to store pointer to list of volumes.

  @retval EFI_SUCCESS             The search was successful.
  @retval EFI_NOT_FOUND           No volumes were found.
  @retval EFI_OUT_OF_RESOURCES    There is not enough memory to complete the
                                  search.
  @retval EFI_INVALID_PARAMETER   One or more of the parameters was invalid.
 */
EFI_STATUS
EFIAPI
LocateFV2Volumes(OUT UINTN       *VolumeCount,
                 OUT FV2_VOLUME **Volumes);

/**
  Free list of FV2 boot loaders.

  @param  VolumeCount   Number of volumes returned by LocateFV2Volumes.
  @param  Volumes       Pointer to volumes returned by LocateFV2Volumes.
 */
VOID
EFIAPI
FreeFV2Volumes(IN UINTN       VolumeCount,
               IN FV2_VOLUME *Volumes);

#endif
