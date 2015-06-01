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
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include "FixedKeyState.h"
#include "FixedTextInput.h"

/**
  Saved state to enable removal of hooks
 */
STATIC struct {
  EFI_HANDLE                      ConsoleInHandle;
  EFI_SIMPLE_TEXT_INPUT_PROTOCOL *ConIn;
  BOOLEAN                         ConsoleIsHooked;

  EFI_HANDLE                      KeyStateHandle;
  KEY_STATE_PROTOCOL             *KeyState;
  BOOLEAN                         KeyStateIsHooked;
} SavedState = {
  NULL, NULL, FALSE, NULL, NULL, FALSE
};

/**
  Replace ConIn/ConsoleInHandle in System Table with custom version.
 
  @param  Data        Data to be returned when console is read from
  @param  DataLength  Length of data (in bytes) pointed to by Data
 
  @retval EFI_SUCCESS           The console was successfully replaced.
  @retval EFI_OUT_OF_RESOURCES  The replacement Simple Text Input protocol
                                could not be created.
  @retval INVALID_PARAMETER     One or more of the parameters are invalid.
 */
STATIC
EFI_STATUS
EFIAPI
HookConsoleIn(IN CONST CHAR8 *Data,
              IN CONST UINTN  DataLength)
{
  EFI_SIMPLE_TEXT_INPUT_PROTOCOL *ConIn;
  EFI_HANDLE                      ConsoleInHandle;
  EFI_STATUS                      Status;

  SavedState.ConsoleInHandle = gST->ConsoleInHandle;
  SavedState.ConIn           = gST->ConIn;

  Status = CreateFixedTextInput(Data, DataLength, &ConIn, &ConsoleInHandle);
  if(!EFI_ERROR(Status)) {
    gST->ConIn           = ConIn;
    gST->ConsoleInHandle = ConsoleInHandle;
    gST->Hdr.CRC32       = 0;
    Status = gBS->CalculateCrc32(gST, gST->Hdr.HeaderSize, &gST->Hdr.CRC32);
  }
  else {
    SavedState.ConsoleInHandle = NULL;
    SavedState.ConIn           = NULL;
  }
  SavedState.ConsoleIsHooked = (EFI_SUCCESS == Status);

  return Status;
}

/**
  Restore the original ConIn/ConsoleInHandler in the System Table.
 */
STATIC
VOID
EFIAPI
UnhookConsoleIn()
{
  if(SavedState.ConsoleIsHooked) {
    if(!EFI_ERROR(FreeFixedTextInput(gST->ConIn))) {
      gST->ConIn           = SavedState.ConIn;
      gST->ConsoleInHandle = SavedState.ConsoleInHandle;
      gST->Hdr.CRC32       = 0;
      gBS->CalculateCrc32(gST, gBS->Hdr.HeaderSize, &gST->Hdr.CRC32);
      SavedState.ConIn           = NULL;
      SavedState.ConsoleInHandle = NULL;
      SavedState.ConsoleIsHooked = FALSE;
    }
  }
}

/**
  Replace a Key State Protocol instance with a custom version.

  @param  Data        Data to be returned when keyboard is scanned.
  @param  DataLength  Length of data (in bytes) pointed to by Data.

  @retval EFI_SUCCESS           The Key State protocol instance was successfully
                                replaced (if it exists).
  @retval EFI_OUT_OF_RESOURCES  The replacement Key State protocol could not be
                                created.
  @retval INVALID_PARAMETER     One or more of the parameters are invalid.
 */
STATIC
EFI_STATUS
EFIAPI
HookKeyState(IN CONST CHAR8 *Data,
             IN CONST UINTN  DataLength)
{
  KEY_STATE_PROTOCOL *OldKeyState;
  KEY_STATE_PROTOCOL *NewKeyState;
  EFI_HANDLE          Handle;
  EFI_STATUS          Status;
  UINTN               Size;

  Size = sizeof(EFI_HANDLE);
  Status = gBS->LocateHandle(ByProtocol,
                             &gKeyStateProtocolGuid,
                             NULL,
                             &Size,
                             (VOID**)&Handle);
  if(!EFI_ERROR(Status)) {
    Status = gBS->OpenProtocol(Handle,
                               &gKeyStateProtocolGuid,
                               (VOID**)&OldKeyState,
                               gImageHandle,
                               NULL,
                               EFI_OPEN_PROTOCOL_GET_PROTOCOL);
    if(!EFI_ERROR(Status)) {
      Status = CreateFixedKeyState(Data, DataLength, &NewKeyState);
      if(!EFI_ERROR(Status)) {
        Status = gBS->ReinstallProtocolInterface(Handle,
                                                 &gKeyStateProtocolGuid,
                                                 OldKeyState,
                                                 NewKeyState);
        if(!EFI_ERROR(Status)) {
          SavedState.KeyStateHandle   = Handle;
          SavedState.KeyState         = OldKeyState;
          SavedState.KeyStateIsHooked = TRUE;
        }
        else {
          Print(L"Failed to ReinstallProtocolInterface - %r\n", Status);
          FreeFixedKeyState(NewKeyState);
        }
      }
      else
        Print(L"Failed to CreateFixedKeyState - %r\n", Status);
    }
    else
      Print(L"Failed to OpenProtocol - %r\n", Status);
  }
  else {
    Print(L"Failed to LocateHandle - %r\n", Status);
    if(EFI_NOT_FOUND == Status)
      Status = EFI_SUCCESS;
  }

  return Status;
}

/**
  Restore the original Key State Protocol.
 */
STATIC
VOID
EFIAPI
UnhookKeyState()
{
  KEY_STATE_PROTOCOL *KeyState;
  EFI_STATUS Status;
  Status = gBS->OpenProtocol(SavedState.KeyStateHandle,
                             &gKeyStateProtocolGuid,
                             (VOID**)&KeyState,
                             gImageHandle,
                             NULL,
                             EFI_OPEN_PROTOCOL_GET_PROTOCOL);
  if(!EFI_ERROR(Status)) {
    Status = gBS->ReinstallProtocolInterface(SavedState.KeyStateHandle,
                                             &gKeyStateProtocolGuid,
                                             KeyState,
                                             SavedState.KeyState);
    if(!EFI_ERROR(Status)) {
      // Delete FixedKeyState
      SavedState.KeyState         = NULL;
      SavedState.KeyStateHandle   = NULL;
      SavedState.KeyStateIsHooked = FALSE;
    }
  }
}

/**
  Remove hooks on console and key state protocols.
 */
VOID
EFIAPI
UnhookKeyboard(VOID)
{
  if(SavedState.KeyStateIsHooked)
    UnhookKeyState();
  if(SavedState.ConsoleIsHooked)
    UnhookConsoleIn();
}

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
             IN CONST UINTN  DataLength)
{
  EFI_STATUS Status;

  UnhookKeyboard();

  Status = HookConsoleIn(Data, DataLength);
  if(!EFI_ERROR(Status)) {
    Status = HookKeyState(Data, DataLength);
    if(EFI_ERROR(Status))
       UnhookConsoleIn();
  }
  return Status;
}
