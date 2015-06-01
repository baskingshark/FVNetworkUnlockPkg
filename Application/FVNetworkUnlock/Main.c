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
#include <Library/SplashScreenLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Protocol/ConsoleControl.h>
#include "FileLoad.h"
#include "FV2.h"
#include "FV2Hook.h"
#include "KeyboardHook.h"
#include "SmBios.h"

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
  Get a "safe" version of the serial number.

  The serial number, as obtained from the SMBIOS tables, could contain
  characters that cannot appear in a filename (such as \, /, or :).  These
  "unsafe" characters are replaced by underscores _.
 
  It is the responsibility of the caller to free the returned string.

  @retval A pointer to a CHAR16 string containing the "sage" version of the
          serial number, or NULL if the serial number could not be found or
          a buffer containing the safe version could not be allocated.
 */
CHAR16*
EFIAPI
GetSafeSerialNumber()
{
  CHAR8       *SN;
  CHAR16      *SN16;
  UINTN        Length;
  UINTN        Idx;
  EFI_STATUS   Status;

  SN = GetSerialNumber();
  if(SN) {
    Length = AsciiStrLen(SN);
    Status = gBS->AllocatePool(EfiBootServicesData,
                               (Length + 1) * sizeof(CHAR16),
                               (VOID**)&SN16);
    if(!EFI_ERROR(Status)) {
      SN16 = AsciiStrToUnicodeStr(SN, SN16);
      for(Idx = 0; Idx < Length; Idx++)
        if(SN16[Idx] == L'\\'|| SN16[Idx] == L'/' || SN16[Idx] == L':')
          SN16[Idx] = L'_';
      return SN16;
    }
  }
  return NULL;
}

/**
  Load password file from boot device.
 
  The password is stored in a file based on the serial number of the
  machine (as obtained from the SMBIOS configuration tables).  Any invalid
  characters are replaced with _s)

  @param  FileSize      Where to store the size of the password file.
  @param  FileBuffer    Where to store the pointer to the password data.

  @retval EFI_SUCCESS           The file was loaded successfully.
  @retval EFI_OUT_OF_RESOURCES  There was insufficient memory to load the file.
  @retval EFI_NOT_FOUND         The serial number was not found.
  @retval EFI_NOT_FOUND         The file was not found.
  @retval EFI_INVALID_PARAMETER One or more of the parameters was invalid.
  @retval EFI_UNSUPPORTED       The device does not support any of the
                                required protocols.

  @retval ...                   Any of the errors generated by the Simple File
                                System Protocol or Load File Protocol when
                                reading files may also be returned.
 */
EFI_STATUS
EFIAPI
LoadPassword(OUT UINTN   *FileSize,
             OUT VOID   **FileBuffer)
{
  CHAR16      *Filename;
  EFI_STATUS   Status;

  if(!FileSize || !FileBuffer)
    return EFI_INVALID_PARAMETER;

  Filename = GetSafeSerialNumber();
  if(Filename) {
    // Attempt to load file
    Status = LoadFileFromBootDevice(Filename,
                                    FileSize,
                                    FileBuffer);
    gBS->FreePool(Filename);
  }
  else
    Status = EFI_NOT_FOUND;
  return Status;
}

/**
  Load splash screen image from boot device.

  The image is stored in one of the following files:
    * A file based on the serial number of the machine (as obtained from
      the SMBIOS configuration tables) with a '.png' extension.  Any invalid
      characters are replaced with _s)
    * A default file called logo.png

  @param  FileSize      Where to store the size of the password file.
  @param  FileBuffer    Where to store the pointer to the password data.

  @retval EFI_SUCCESS           The file was loaded successfully.
  @retval EFI_OUT_OF_RESOURCES  There was insufficient memory to load the file.
  @retval EFI_NOT_FOUND         The serial number was not found.
  @retval EFI_NOT_FOUND         The file was not found.
  @retval EFI_INVALID_PARAMETER One or more of the parameters was invalid.
  @retval EFI_UNSUPPORTED       The device does not support any of the
                                required protocols.

  @retval ...                   Any of the errors generated by the Simple File
                                System Protocol or Load File Protocol when
                                reading files may also be returned.
 */
EFI_STATUS
EFIAPI
LoadSplashScreen(OUT UINTN   *FileSize,
                 OUT VOID   **FileBuffer)
{
  CHAR16      *SerialNumber;
  CHAR16      *Filename;
  UINTN        SerialNumberSize;
  UINTN        Idx;
  EFI_STATUS   Status;

  if(!FileSize || !FileBuffer)
    return EFI_INVALID_PARAMETER;

  SerialNumber = GetSafeSerialNumber();
  if(SerialNumber) {
    SerialNumberSize = StrLen(SerialNumber);
    Status = gBS->AllocatePool(EfiBootServicesData,
                               (SerialNumberSize + 5) * sizeof(CHAR16),
                               (VOID**)&Filename);
    if(!EFI_ERROR(Status)) {
      for(Idx = 0; Idx < SerialNumberSize; Idx++)
        Filename[Idx] = SerialNumber[Idx];
      Filename[Idx++] = L'.';
      Filename[Idx++] = L'p';
      Filename[Idx++] = L'n';
      Filename[Idx++] = L'g';
      Filename[Idx++] = 0;
      // Attempt to load file
      Status = LoadFileFromBootDevice(Filename,
                                      FileSize,
                                      FileBuffer);
      gBS->FreePool(Filename);
    }
    gBS->FreePool(SerialNumber);
  }
  else
    Status = EFI_NOT_FOUND;
  if(EFI_ERROR(Status))
    Status = LoadFileFromBootDevice(L"logo.png", FileSize, FileBuffer);
  return Status;
}

/* Default time to display splash screen */
#ifndef SPLASH_SCREEN_DELAY
#define SPLASH_SCREEN_DELAY 2500000
#endif

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
  EFI_HANDLE        LoaderHandle;
  FV2_VOLUME       *Volumes;
  UINTN             VolumeCount;
  UINTN             FileSize;
  CHAR8            *FileBuffer;
  UINTN             SplashScreenSize;
  VOID             *SplashScreen;
  UINTN             Idx;

  Status = LoadSplashScreen(&SplashScreenSize, &SplashScreen);
  if(!EFI_ERROR(Status)) {
    Status = ShowSplashScreen(SplashScreen, SplashScreenSize);
    if(EFI_ERROR(Status))
      Print(L"Failed to show splash screen - %r\n", Status);
    else
      gBS->Stall(SPLASH_SCREEN_DELAY);
    gBS->FreePool(SplashScreen);
  }
  else
    Print(L"Failed to load splash screen - %r\n", Status);
  if(EFI_ERROR(Status))
    SwitchToTextMode();

  Print(L"Starting ...\n");
  ConnectControllers();
  Status = LocateFV2Volumes(&VolumeCount, &Volumes);
  if(!EFI_ERROR(Status)) {
    Print(L"Got %d boot loaders\n", VolumeCount);
    for(Idx = 0; Idx < VolumeCount; Idx++)
      HookVolume(&Volumes[Idx]);
    Status = LoadPassword(&FileSize, (VOID**)&FileBuffer);
    if(!EFI_ERROR(Status)) {
      Print(L"Got %d bytes at %p\n", FileSize, FileBuffer);
      Status = HookKeyboard(FileBuffer, FileSize);
      // Erase File Buffer now!
      SetMem(FileBuffer, FileSize, 0);
      gBS->FreePool(FileBuffer);
      if(!EFI_ERROR(Status)) {
        Status = gBS->LoadImage(FALSE,
                                gImageHandle,
                                Volumes[0].BootLoaderDevPath,
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
    FreeFV2Volumes(VolumeCount, Volumes);
  }
  else
    Print(L"Failed to locate next boot loader - %r\n", Status);

  return Status;
}
