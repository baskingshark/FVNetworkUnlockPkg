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
#include <Protocol/SimpleTextIn.h>

typedef struct {
  EFI_SIMPLE_TEXT_INPUT_PROTOCOL SimpleTextIn;
  EFI_HANDLE                     Handle;
  UINTN                          Index;
  UINTN                          Count;
  EFI_INPUT_KEY                  Data[0];
} FIXED_TEXT_INPUT_PROTOCOL;

/**
  Implementation of Reset function of the Simple Text Input Protocol
 */
STATIC
EFI_STATUS
EFIAPI
Reset(IN EFI_SIMPLE_TEXT_INPUT_PROTOCOL *This,
      IN BOOLEAN                         ExtendedVerification)
{
  return EFI_SUCCESS;
}

/**
  Implementation of ReadKeyStroke function of the Simple Text Input Protocol
 */
STATIC
EFI_STATUS
EFIAPI
ReadKeyStroke(IN  EFI_SIMPLE_TEXT_INPUT_PROTOCOL *This,
              OUT EFI_INPUT_KEY                  *Key)
{
  FIXED_TEXT_INPUT_PROTOCOL *FixedTextIn = (FIXED_TEXT_INPUT_PROTOCOL*)This;
  if(!Key)
    return EFI_INVALID_PARAMETER;
  if(FixedTextIn->Index < FixedTextIn->Count) {
    *Key = FixedTextIn->Data[FixedTextIn->Index];
    FixedTextIn->Data[FixedTextIn->Index].ScanCode    = 0;
    FixedTextIn->Data[FixedTextIn->Index].UnicodeChar = 0;
    FixedTextIn->Index++;
    return EFI_SUCCESS;
  }
  return EFI_NOT_READY;
}

/**
  Called when the SIMPLE_TEXT_INPUT_PROTOCOL.WaitForKey is waited on.
 
  @param  Event     The Event that has been waited on.
  @param  Context   Should be a pointer to the FIXED_TEXT_INPUT_PROTOCOL
                    containing Event.
 */
STATIC
VOID
EFIAPI
WaitForKeyCallback(IN EFI_EVENT Event,
                   IN VOID*     Context)
{
  // Called whenever Wait is called on the event
  FIXED_TEXT_INPUT_PROTOCOL *FixedTextIn = (FIXED_TEXT_INPUT_PROTOCOL*)Context;
  if(FixedTextIn->Index < FixedTextIn->Count)
    gBS->SignalEvent(Event);
}

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
                     OUT EFI_HANDLE                      *Handle)
{
  FIXED_TEXT_INPUT_PROTOCOL *FixedTextIn;
  EFI_STATUS Status;
  UINTN Index;

  if((Length && !Data) || !SimpleTextIn || !Handle)
    return EFI_INVALID_PARAMETER;

  Status = gBS->AllocatePool(EfiBootServicesCode,
                             (sizeof(*FixedTextIn)
                              + Length * sizeof(EFI_INPUT_KEY)),
                             (VOID**)&FixedTextIn);
  if(!EFI_ERROR(Status)) {
    Status = gBS->CreateEvent(EVT_NOTIFY_WAIT,
                              TPL_CALLBACK,
                              WaitForKeyCallback,
                              FixedTextIn,
                              &FixedTextIn->SimpleTextIn.WaitForKey);
    if(!EFI_ERROR(Status)) {
      FixedTextIn->Handle = NULL;
      Status = gBS->InstallProtocolInterface(&FixedTextIn->Handle,
                                             &gEfiSimpleTextInProtocolGuid,
                                             EFI_NATIVE_INTERFACE,
                                             &FixedTextIn->SimpleTextIn);
      if(!EFI_ERROR(Status)) {
        FixedTextIn->SimpleTextIn.Reset         = Reset;
        FixedTextIn->SimpleTextIn.ReadKeyStroke = ReadKeyStroke;
        FixedTextIn->Count = Length;
        FixedTextIn->Index = 0;
        for(Index = 0; Index < Length; Index++) {
          FixedTextIn->Data[Index].ScanCode    = 0;
          FixedTextIn->Data[Index].UnicodeChar = Data[Index];
        }
      }
      else {
        Print(L"Failed to InstallProtocolInterface - %r\n", Status);
      }
    }
    else {
      Print(L"Failed to CreateEvent - %r\n", Status);
      FixedTextIn->SimpleTextIn.WaitForKey = NULL;
    }
  }
  else {
    Print(L"Failed to AllocatePool for protocol instance - %r\n", Status);
    FixedTextIn = NULL;
  }

  if(EFI_ERROR(Status)) {
    if(FixedTextIn) {
      if(FixedTextIn->SimpleTextIn.WaitForKey) {
        gBS->CloseEvent(FixedTextIn->SimpleTextIn.WaitForKey);
      }
      gBS->FreePool(FixedTextIn);
    }
  }
  else {
    *SimpleTextIn = &FixedTextIn->SimpleTextIn;
    *Handle       = FixedTextIn->Handle;
  }
  return Status;
}

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
FreeFixedTextInput(IN EFI_SIMPLE_TEXT_INPUT_PROTOCOL *SimpleTextIn)
{
  FIXED_TEXT_INPUT_PROTOCOL *FixedTextIn;
  EFI_STATUS                 Status;
  UINTN                      Index;

  if(!SimpleTextIn)
    return EFI_INVALID_PARAMETER;
  FixedTextIn = (FIXED_TEXT_INPUT_PROTOCOL*)SimpleTextIn;
  Status = gBS->UninstallProtocolInterface(FixedTextIn->Handle,
                                           &gEfiSimpleTextInProtocolGuid,
                                           &(FixedTextIn->SimpleTextIn));
  if(!EFI_ERROR(Status)) {
    if(FixedTextIn->SimpleTextIn.WaitForKey)
      gBS->CloseEvent(FixedTextIn->SimpleTextIn.WaitForKey);
    for(Index = FixedTextIn->Index; Index < FixedTextIn->Count; Index++) {
      FixedTextIn->Data[Index].ScanCode = 0;
      FixedTextIn->Data[Index].UnicodeChar = 0;
    }
    gBS->FreePool(FixedTextIn);
  }

  return Status;
}
