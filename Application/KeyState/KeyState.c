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
#include <Protocol/ConsoleControl.h>
#include <Protocol/KeyState.h>

EFI_STATUS
EFIAPI
HideSplashScreen()
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

 @param[in] ImageHandle    The firmware allocated handle for the EFI image.
 @param[in] SystemTable    A pointer to the EFI System Table.

 @retval EFI_SUCCESS       The entry point is executed successfully.
 @retval other             Some error occurs when executing this entry point.

 **/
EFI_STATUS
EFIAPI
UefiMain (
          IN EFI_HANDLE        ImageHandle,
          IN EFI_SYSTEM_TABLE *SystemTable
          )
{
#define KEY(x) { KEY_STATE_##x, #x }
  STATIC CONST struct {
    UINT16 Mask;
    CHAR8* Name;
  } MODS[] = {
    KEY(MOD_LEFT_COMMAND), KEY(MOD_RIGHT_COMMAND),
    KEY(MOD_LEFT_OPTION),  KEY(MOD_RIGHT_OPTION),
    KEY(MOD_LEFT_CONTROL), KEY(MOD_RIGHT_CONTROL),
    KEY(MOD_LEFT_SHIFT),   KEY(MOD_RIGHT_SHIFT),
  };
  STATIC CONST UINTN MOD_COUNT = sizeof(MODS)/sizeof(MODS[0]);
  STATIC CONST struct {
    UINT16 Key;
    CHAR8* Name;
  } KEYS[] = {
    KEY(A),             KEY(B),             KEY(C),
    KEY(D),             KEY(E),             KEY(F),
    KEY(G),             KEY(H),             KEY(I),
    KEY(J),             KEY(K),             KEY(L),
    KEY(M),             KEY(N),             KEY(O),
    KEY(P),             KEY(Q),             KEY(R),
    KEY(S),             KEY(T),             KEY(U),
    KEY(V),             KEY(W),             KEY(X),
    KEY(Y),             KEY(Z),             KEY(1),
    KEY(2),             KEY(3),             KEY(4),
    KEY(5),             KEY(6),             KEY(7),
    KEY(8),             KEY(9),             KEY(0),
    KEY(ENTER),         KEY(ESCAPE),        KEY(BACKSPACE),
    KEY(TAB),           KEY(SPACE),         KEY(MINUS),
    KEY(EQUAL),         KEY(LEFT_BRACKET),  KEY(RIGHT_BRACKET),
    KEY(BACKSLASH),     KEY(UNKNOWN),       KEY(SEMICOLON),
    KEY(QUOTE),         KEY(GRAVE),         KEY(COMMA),
    KEY(PERIOD),        KEY(SLASH),         KEY(CAPS_LOCK),
    KEY(F1),            KEY(F2),            KEY(F3),
    KEY(F4),            KEY(F5),            KEY(F6),
    KEY(F7),            KEY(F8),            KEY(F9),
    KEY(F10),           KEY(F11),           KEY(F12),
    KEY(PRINT_SCREEN),  KEY(SCROLL_LOCK),   KEY(PAUSE),
    KEY(INSERT),        KEY(HOME),          KEY(PAGE_UP),
    KEY(DELETE),        KEY(END),           KEY(PAGE_DOWN),
    KEY(RIGHT),         KEY(LEFT),          KEY(DOWN),
    KEY(UP),            KEY(NUM_LOCK),      KEY(NUM_DIV),
    KEY(NUM_MUL),       KEY(NUM_MINUS),     KEY(NUM_PLUS),
    KEY(NUM_ENTER),     KEY(NUM_1),         KEY(NUM_2),
    KEY(NUM_3),         KEY(NUM_4),         KEY(NUM_5),
    KEY(NUM_6),         KEY(NUM_7),         KEY(NUM_8),
    KEY(NUM_9),         KEY(NUM_0),         KEY(NUM_PERIOD),
    KEY(SECTION),       KEY(APPLICATION),   KEY(NUM_EQUAL),
    KEY(F13),           KEY(F14),           KEY(F15),
  };
  STATIC CONST UINTN KEY_COUNT = sizeof(KEYS)/sizeof(KEYS[0]);

  KEY_STATE_PROTOCOL *KeyState;
  UINT16              Modifier;
  UINTN               KeyCount;
  CHAR16              Keys[32];
  UINTN               Idx1;
  UINTN               Idx2;
  EFI_STATUS          Status;

  HideSplashScreen();
  Print(L"Starting ...\n");
  Status = gBS->LocateProtocol(&gKeyStateProtocolGuid,
                               NULL,
                               (VOID**)&KeyState);
  if(!EFI_ERROR(Status)) {
    while(TRUE) {
      KeyCount = sizeof(Keys)/sizeof(Keys[0]);
      Status = KeyState->ReadKeyState(KeyState, &Modifier, &KeyCount, Keys);
      if(!EFI_ERROR(Status)) {
        for(Idx2 = 0; Idx2 < MOD_COUNT; Idx2++)
          if(Modifier & MODS[Idx2].Mask)
            Print(L"%a ", MODS[Idx2].Name);
        if(KeyCount >0) {
          for(Idx1 = 0; Idx1 < KeyCount; Idx1++) {
            for(Idx2 = 0; Idx2 < KEY_COUNT; Idx2++) {
              if(KEYS[Idx2].Key == Keys[Idx1]) {
                Print(L"%a ", KEYS[Idx2].Name);
                break;
              }
            }
            if(Idx2 == KEY_COUNT) {
              Print(L"%04x ", Keys[Idx1]);
            }
          }
        }
        if(Modifier || KeyCount) {
          Print(L"\n");
          gBS->Stall(90000);
        }
      }
      gBS->Stall(10000);
    }
  }
  else
    Print(L"Failed to locate KeyState protocol - %r\n", Status);
  return Status;
}