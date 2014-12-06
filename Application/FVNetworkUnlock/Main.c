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

#include <Uefi.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Protocol/ConsoleControl.h>
#include <Protocol/LoadedImage.h>
#include "FileLoad.h"
#include "FV2.h"
#include "KeyboardHook.h"

/**
  Connect all controllers and drivers
 
  @retval EFI_SUCCESS           The operation completed successfully.
  @retval EFI_OUT_OF_RESOURCES  There was not enough memory to proceed.
 */
STATIC
EFI_STATUS
EFIAPI
ConnectControllers(VOID)
{
  EFI_HANDLE *HandleBuffer;
  EFI_STATUS  Status;
  UINTN       HandleCount;
  UINTN       Index;

  Status = gBS->LocateHandleBuffer(AllHandles,
                                   NULL,
                                   NULL,
                                   &HandleCount,
                                   &HandleBuffer);
  if(!EFI_ERROR(Status)) {
    for(Index = 0; Index < HandleCount; Index++)
      (VOID) gBS->ConnectController(HandleBuffer[Index], NULL, NULL, TRUE);
    if(HandleBuffer)
      gBS->FreePool(HandleBuffer);
  }
  return Status;
}

/**
  Switch to text mode, if the system supports the Console Control Protoocl.

  @retval EFI_SUCCESS   The console was switched to text mode.
  @retval EFI_NOT_FOUND The system does not support the Console Control Protocol
*/
EFI_STATUS
EFIAPI
SwitchToTextMode(VOID)
{
  EFI_CONSOLE_CONTROL_PROTOCOL *ConsoleControl;
  EFI_STATUS                    Status;

  Status = gBS->LocateProtocol(&gEfiConsoleControlProtocolGuid,
                               NULL,
                               (VOID**)&ConsoleControl);
  if(!EFI_ERROR(Status)) {
    Status = ConsoleControl->SetMode(ConsoleControl,
                                     EfiConsoleControlScreenText);
  }
  return Status;
}

/**
  The entry point for the application.

  @param ImageHandle    The firmware allocated handle for the EFI image.
  @param SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.
**/
EFI_STATUS
EFIAPI
UefiMain(IN EFI_HANDLE        ImageHandle,
         IN EFI_SYSTEM_TABLE *SystemTable)
{
  EFI_STATUS        Status;
  EFI_LOADED_IMAGE *LoadedImage;
  EFI_HANDLE        LoaderHandle;
  BOOT_LOADER      *BootLoaders;
  UINTN             BootLoaderCount;
  UINTN             FileSize;
  CHAR8            *FileBuffer;

  SwitchToTextMode();
  Print(L"Starting ...\n");
  ConnectControllers();
  Status = LocateFV2BootLoaders(&BootLoaderCount, &BootLoaders);
  if(!EFI_ERROR(Status)) {
    Print(L"Got %d boot loaders\n", BootLoaderCount);
    Status = gBS->OpenProtocol(gImageHandle,
                               &gEfiLoadedImageProtocolGuid,
                               (VOID**)&LoadedImage,
                               gImageHandle,
                               NULL,
                               EFI_OPEN_PROTOCOL_GET_PROTOCOL);
    if(!EFI_ERROR(Status)) {
      Status = LoadFile(LoadedImage->DeviceHandle,
                        L"\\password",
                        &FileSize,
                        (VOID**)&FileBuffer);
      Print(L"Got %d bytes at %p\n", FileSize, FileBuffer);
      if(!EFI_ERROR(Status)) {
        Status = HookKeyboard(FileBuffer, FileSize);
        // Erase File Buffer now!
        SetMem(FileBuffer, FileSize, 0);
        gBS->FreePool(FileBuffer);
        if(!EFI_ERROR(Status)) {
          Status = gBS->LoadImage(FALSE,
                                  gImageHandle,
                                  BootLoaders[0].BootLoader,
                                  NULL,
                                  0,
                                  &LoaderHandle);
          if(!EFI_ERROR(Status)) {
            Status = gBS->StartImage(LoaderHandle, NULL, NULL);
            if(EFI_ERROR(Status))
              Print(L"Failed to start boot loader - %r\n", Status);
            gBS->UnloadImage(LoaderHandle);
          }
          else
            Print(L"Failed to load boot loader - %r\n", Status);
          UnhookKeyboard();
        }
        else
          Print(L"Failed to create new system table - %r\n", Status);
      }
      else
        Print(L"Failed to load password file - %r\n", Status);
    }
    else
      Print(L"Failed to open LOADED_IMAGE_PROTOCOL - %r\n", Status);
    FreeFV2BootLoaders(BootLoaderCount, BootLoaders);
  }
  else
    Print(L"Failed to locate next boot loader - %r\n", Status);

  return Status;
}
