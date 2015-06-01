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
#include <Protocol/KeyState.h>

/**
  Key press information, contains key code and mofifier flags.
 */
typedef struct _KEY_PRESS {
  UINT16 KeyCode;
  UINT16 Modifiers;
} KEY_PRESS;

/**
  Macros for key presses.
  key - key pressed without shift
  KEY - key pressed with shift
 */
#define key(x) { KEY_STATE_##x, 0 }
#define KEY(x) { KEY_STATE_##x, KEY_STATE_MOD_LEFT_SHIFT }

/**
  Translation table for ASCII characters between 32 & 127
 */
STATIC
CONST
KEY_PRESS
KEY_MAP[] = {
  /*   */key(SPACE),        /* ! */KEY(1),            /* " */KEY(QUOTE),
  /* # */KEY(3),            /* $ */KEY(4),            /* % */KEY(5),
  /* & */KEY(7),            /* ' */key(QUOTE),        /* ( */KEY(9),
  /* ) */KEY(0),            /* * */KEY(8),            /* + */KEY(EQUAL),
  /* , */key(COMMA),        /* - */key(MINUS),        /* . */key(PERIOD),
  /* / */key(SLASH),        /* 0 */key(0),            /* 1 */key(1),
  /* 2 */key(2),            /* 3 */key(3),            /* 4 */key(4),
  /* 5 */key(5),            /* 6 */key(6),            /* 7 */key(7),
  /* 8 */key(8),            /* 9 */key(9),            /* : */KEY(SEMICOLON),
  /* ; */key(SEMICOLON),    /* < */KEY(COMMA),        /* = */key(EQUAL),
  /* > */KEY(PERIOD),       /* / */KEY(SLASH),        /* @ */KEY(2),
  /* A */KEY(A),            /* B */KEY(B),            /* C */KEY(C),
  /* D */KEY(D),            /* E */KEY(E),            /* F */KEY(F),
  /* G */KEY(G),            /* H */KEY(H),            /* I */KEY(I),
  /* J */KEY(J),            /* K */KEY(K),            /* L */KEY(L),
  /* M */KEY(M),            /* N */KEY(N),            /* O */KEY(O),
  /* P */KEY(P),            /* Q */KEY(Q),            /* R */KEY(R),
  /* S */KEY(S),            /* T */KEY(T),            /* U */KEY(U),
  /* V */KEY(V),            /* W */KEY(W),            /* X */KEY(X),
  /* Y */KEY(Y),            /* Z */KEY(Z),            /* [ */key(LEFT_BRACKET),
  /* \ */key(BACKSLASH),    /* ] */key(RIGHT_BRACKET),/* ^ */KEY(6),
  /* _ */KEY(MINUS),        /* ` */key(GRAVE),        /* a */key(A),
  /* b */key(B),            /* c */key(C),            /* d */key(D),
  /* e */key(E),            /* f */key(F),            /* g */key(G),
  /* h */key(H),            /* i */key(I),            /* j */key(J),
  /* k */key(K),            /* l */key(L),            /* m */key(M),
  /* n */key(N),            /* o */key(O),            /* p */key(P),
  /* q */key(Q),            /* r */key(R),            /* s */key(S),
  /* t */key(T),            /* u */key(U),            /* v */key(V),
  /* w */key(W),            /* x */key(X),            /* y */key(Y),
  /* z */key(Z),            /* { */KEY(LEFT_BRACKET), /* | */KEY(BACKSLASH),
  /* } */KEY(RIGHT_BRACKET),/* ~ */KEY(GRAVE),
};

/**
  Convert ASCII character to key press.

  @param  Char        ASCII Character to convert
  @param  Key         Pointer to location to store keypress information.

  @retval EFI_SUCCESS             The character was converted.
  @retval EFI_INVALID_PARAMETER   Key was NULL.
 */
STATIC
EFI_STATUS
EFIAPI
ConvertKey(IN  CHAR8      Char,
           OUT KEY_PRESS *Key)
{
  if(!Key)
    return EFI_INVALID_PARAMETER;
  if(!(' ' <= Char && Char <= '~')) {
    // Special cases not in character map ...
    switch(Char) {
      case '\n':
      case '\r':
        // EOL characters
        Key->KeyCode   = KEY_STATE_ENTER;
        Key->Modifiers = 0;
        break;
      default:
        Key->KeyCode   = 0;
        Key->Modifiers = 0;
    }
  }
  else
    *Key = KEY_MAP[Char - ' '];
  return EFI_SUCCESS;
}

/**
  Current state of virtual keyboard
 */
typedef enum _KEY_PRESS_STATE {
  KEY_IDLE,         // Virtual keyboard is idle
  KEY_DOWN,         // A key is being pressed
  KEY_UP,           // A key has been released
  KEY_FIRST         // Awaiting first scan by Apple boot loader
                    //   Used to detect boot options
} KEY_PRESS_STATE;

/**
  Fixed Key State Protocol - Provide a virtual keyboard to simulate key presses
 */
typedef struct _FIXED_KEY_STATE_PROTOCOL FIXED_KEY_STATE_PROTOCOL;
struct _FIXED_KEY_STATE_PROTOCOL {
  KEY_STATE_PROTOCOL  KeyStateProtocol;
  KEY_STATE_PROTOCOL *Old;
  UINTN               Index;
  UINTN               Count;
  KEY_PRESS_STATE     KeyState;
  EFI_EVENT           Timer;
  KEY_PRESS           CurrentKey;
  KEY_PRESS           Data[0];
};

/**
  Timer intervals for keyboard events.
    TIMER_MS    - Macro to convert ms to timer units
    DOWN_TIME   - Duration of key press
    UP_TIME     - Time before next key press can occur
    IGNORE_TIME - Time before next key press after first scan
 */
#define TIMER_MS(x) ((x)*10000UL)
#define DOWN_TIME   TIMER_MS(20)
#define UP_TIME     TIMER_MS(20)
#define IGNORE_TIME TIMER_MS(3000)

/**
  Read from virtual keyboard.
 
  @param  This        Protocol instance pointer.
  @param  Modifiers   Pointer to the location to store the currently pressed
                      modifier keys.
  @param  KeyCount    On entry, contains the number of keys that can be stored
                      in the buffer pointed to by Keys.  On exit, contains the
                      number of keys currently pressed.
  @param  Keys        Pointer to the location to store the keys currently
                      pressed. Can be NULL if KeyCount is zero.
 
  @retval EFI_SUCCESS             The keyboard state was retrieved.
  @retval EFI_BUFFER_TOO_SMALL    The buffer pointed to by Keys is too small to
                                  hold the list of pressed keys.  KeyCount will
                                  be updated with the number of keys currently
                                  pressed.
  @retval EFI_INVALID_PARAMETER   One or more of the parameters are invalud.
 */
STATIC
EFI_STATUS
EFIAPI
ReadKeyState(IN     KEY_STATE_PROTOCOL *This,
             OUT    UINT16             *Modifiers,
             IN OUT UINTN              *KeyCount,
             OUT    CHAR16             *Keys          OPTIONAL)
{
  FIXED_KEY_STATE_PROTOCOL *FixedKey;
  BOOLEAN                   KeyAvailable;

  if(!Modifiers || !KeyCount || (*KeyCount && !Keys))
    return EFI_INVALID_PARAMETER;

  FixedKey = (FIXED_KEY_STATE_PROTOCOL*)This;

  // Handle first check ... called by boot loader to check for special keys
  if(KEY_FIRST == FixedKey->KeyState) {
    // For now, do nothing.
    *Modifiers = KEY_STATE_MOD_LEFT_COMMAND | KEY_STATE_MOD_RIGHT_COMMAND;
    *KeyCount  = 1;
    *Keys      = KEY_STATE_V;
    FixedKey->KeyState = (gBS->SetTimer(FixedKey->Timer,
                                        TimerRelative,
                                        IGNORE_TIME) ? KEY_IDLE : KEY_UP);
    return EFI_SUCCESS;
  }

  KeyAvailable = (KEY_DOWN == FixedKey->KeyState ||
                  (KEY_IDLE == FixedKey->KeyState &&
                   (FixedKey->Index < FixedKey->Count)));
  if(!KeyAvailable) {
    // No key data available
    *Modifiers = 0;
    *KeyCount  = 0;
    return EFI_SUCCESS;
  }

  // Do we have space for the key?
  if(*KeyCount < 1) {
    *Modifiers = FixedKey->CurrentKey.Modifiers;
    *KeyCount  = 1;
    return EFI_BUFFER_TOO_SMALL;
  }

  // Pre-existing key ...
  if(KEY_DOWN != FixedKey->KeyState) {
    EFI_STATUS Status;
    FixedKey->CurrentKey = FixedKey->Data[FixedKey->Index];
    FixedKey->Data[FixedKey->Index].KeyCode   = 0;
    FixedKey->Data[FixedKey->Index].Modifiers = 0;
    FixedKey->Index++;
    Status = gBS->SetTimer(FixedKey->Timer,
                           TimerRelative,
                           DOWN_TIME);
    FixedKey->KeyState = (Status? KEY_IDLE : KEY_DOWN);
  }
  *Modifiers = FixedKey->CurrentKey.Modifiers;
  *KeyCount  = 1;
  *Keys      = FixedKey->CurrentKey.KeyCode;
  return EFI_SUCCESS;
}

/**
  Virtual Keyboard Timer Event handler.
 
  The KeyboardTimerHandler should be triggered after:
    - the DOWN_TIME has expired, in which case the timer is reset for UP_TIME.
    - the UP_TIME has expired, the keyboard is made idle.

  @param  Event     Timer event that has fired
  @param  Context   Pointer to the Fixed Key State Protocol instance
 */
STATIC
VOID
EFIAPI
KeyboardTimerHandler(IN EFI_EVENT  Event,
                     IN VOID      *Context)
{
  FIXED_KEY_STATE_PROTOCOL *FixedKey = (FIXED_KEY_STATE_PROTOCOL*)Context;
  EFI_STATUS Status;

  FixedKey->CurrentKey.KeyCode   = 0;
  FixedKey->CurrentKey.Modifiers = 0;
  if(KEY_DOWN == FixedKey->KeyState) {
    Status = gBS->SetTimer(FixedKey->Timer,
                           TimerRelative,
                           UP_TIME);
    FixedKey->KeyState = (Status? KEY_IDLE: KEY_UP);
  }
  else
    FixedKey->KeyState = KEY_IDLE;
}

/**
  Create a virtual keyboard that implements the Key State Protocol.
 
  @param  Data                    Pointer the ASCII characters to generate
  @param  Length                  Number of characters pointed to by Data
  @param  KeyStateProtocol        Pointer to the location to store the pointer
                                  to the KEY_STATE_PROTOCOL instance.
 
  @retval EFI_SUCCESS             The KEY_STATE_PROTOCOL was created.
  @retval EFI_OUT_OF_RESOURCES    The KEY_STATE_PROTOCOL could not be allocated
  @retval EFI_INVALID_PARAMETER   One or more of the parameters are invalid.

 */
EFI_STATUS
EFIAPI
CreateFixedKeyState(IN     CONST CHAR8         *Data,
                    IN     CONST UINTN          Length,
                    IN OUT KEY_STATE_PROTOCOL **KeyStateProtocol)
{
  FIXED_KEY_STATE_PROTOCOL *KeyState;
  EFI_STATUS                Status;
  UINTN                     Index;

  if((Length && !Data) || !KeyStateProtocol)
    return EFI_INVALID_PARAMETER;

  Status = gBS->AllocatePool(EfiBootServicesCode,
                             (sizeof(FIXED_KEY_STATE_PROTOCOL)
                              + Length * sizeof(KEY_PRESS)),
                             (VOID**)&KeyState);
  if(!EFI_ERROR(Status)) {
    KeyState->KeyStateProtocol.Signature    = 0;
    KeyState->KeyStateProtocol.ReadKeyState = ReadKeyState;
    KeyState->Index = 0;
    KeyState->Count = Length;
    KeyState->KeyState = KEY_FIRST;
    KeyState->CurrentKey.KeyCode   = 0;
    KeyState->CurrentKey.Modifiers = 0;
    Status = gBS->CreateEvent(EVT_TIMER | EVT_NOTIFY_SIGNAL,
                              TPL_NOTIFY,
                              KeyboardTimerHandler,
                              KeyState,
                              &KeyState->Timer);
    if(!EFI_ERROR(Status)) {
      gBS->LocateProtocol(&gKeyStateProtocolGuid, NULL, (VOID**)&KeyState->Old);
      for(Index = 0; Index < Length; Index++) {
        ConvertKey(Data[Index], &KeyState->Data[Index]);
      }
      *KeyStateProtocol = &KeyState->KeyStateProtocol;
    }
    else {
      Print(L"Failed to CreateEvent for key press timer - %r\n", Status);
      gBS->FreePool(KeyState);
    }
  }
  else {
    Print(L"Failed to AllocatePool for protocol instance - %r\n", Status);
  }

  return Status;
}

/**
  Release a virtual keyboard that implements the Key State Protocol.

  @param  KeyState                The instance to free

  @retval EFI_SUCCESS             The protocol instance was successfully freed
  @retval EFI_NOT_FOUND           The protocol instance was not found
  @retval EFI_ACCESS_DENIED       The protocol instance is in-use
  @retval EFI_INVALID_PARAMETER   KeyStateProtocol is NULL
*/
EFI_STATUS
EFIAPI
FreeFixedKeyState(IN KEY_STATE_PROTOCOL *KeyStateProtocol)
{
  FIXED_KEY_STATE_PROTOCOL *Fixed;
  UINTN                     Idx;

  if(!KeyStateProtocol)
    return EFI_INVALID_PARAMETER;

  Fixed = (FIXED_KEY_STATE_PROTOCOL*)KeyStateProtocol;

  // Release timer
  gBS->SetTimer(Fixed->Timer, TimerCancel, 0);
  gBS->CloseEvent(Fixed->Timer);

  // Erase outstanding key presses
  for(Idx = Fixed->Index; Idx < Fixed->Count; Idx++) {
    Fixed->Data[Idx].KeyCode   = 0;
    Fixed->Data[Idx].Modifiers = 0;
  }

  // Free memory
  gBS->FreePool(Fixed);
  return EFI_SUCCESS;
}