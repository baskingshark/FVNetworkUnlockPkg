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

#ifndef __SM_BIOS_H__
#define __SM_BIOS_H__

#include <IndustryStandard/SmBios.h>

/**
  Find the requested SMBIOS structure of the specified type.

  @param  Type    The SMBIOS structure type to return.
  @param  Index   Which instance of the reuested type to return.
  @param  Struct  Pointer to the requested SMBIOS structure.
  @param  Size    Size of the SMBIOS structure.

  @retval EFI_SUCCESS     The SMBIOS structure was found.
  @retval EFI_NOT_FOUND   The requested SMBIOS structure was not found.
 */
EFI_STATUS
EFIAPI
FindSmBiosTableIndex(IN  UINT8              Type,
                     IN  UINTN              Index,
                     OUT SMBIOS_STRUCTURE **Struct,
                     OUT UINTN             *Size);

/**
  Find the first SMBIOS structure of the specified type.

  @param  Type    The SMBIOS structure type to return.
  @param  Struct  Pointer to the requested SMBIOS structure.
  @param  Size    Size of the SMBIOS structure.

  @retval EFI_SUCCESS     The SMBIOS structure was found.
  @retval EFI_NOT_FOUND   The requested SMBIOS structure was not found.
 */
EFI_STATUS
EFIAPI
FindSmBiosTable(IN  UINT8              Type,
                OUT SMBIOS_STRUCTURE **Struct,
                OUT UINTN             *Size);

/**
  Find the requested string in the SMBIOS structure.

  @param  Struct  Pointer to the SMBIOS structure.
  @param  Size    Size of the SMBIOS structure.
  @param  Index   String index.

  @return A pointer to the string or NULL if the string was not found.
 */
CHAR8*
EFIAPI
FindSmBiosString(IN SMBIOS_STRUCTURE     *Struct,
                 IN UINTN                 Size,
                 IN SMBIOS_TABLE_STRING   Index);

/**
 Get serial number from SMBIOS tables.

 @return A pointer to the serial number string, or NULL if the serial number
 was not found.
 */
CHAR8*
EFIAPI
GetSerialNumber();

#endif