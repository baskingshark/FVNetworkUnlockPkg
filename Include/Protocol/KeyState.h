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

#ifndef __KEY_STATE_H__
#define __KEY_STATE_H__

#include <Uefi.h>

#define KEY_STATE_PROTOCOL_GUID \
  {0x5b213447, 0x6e73, 0x4901, {0xa4, 0xf1, 0xb8, 0x64, 0xf3, 0xb7, 0xa1, 0x72}}

typedef struct _KEY_STATE_PROTOCOL KEY_STATE_PROTOCOL;

//
// Modifier keys (ORed together)
//
// Control
#define KEY_STATE_MOD_LEFT_CONTROL  0x01
#define KEY_STATE_MOD_RIGHT_CONTROL 0x10
// Shift
#define KEY_STATE_MOD_LEFT_SHIFT    0x02
#define KEY_STATE_MOD_RIGHT_SHIFT   0x20
// Option
#define KEY_STATE_MOD_LEFT_OPTION   0x04
#define KEY_STATE_MOD_RIGHT_OPTION  0x40
// Command
#define KEY_STATE_MOD_LEFT_COMMAND  0x08
#define KEY_STATE_MOD_RIGHT_COMMAND 0x80

//
// Standard keys
// These follow the same pattern as the USB scan codes (but + 0x7000)
// See http://www.usb.org/developers/hidpage/Hut1_12v2.pdf for USB scan codes.
//
enum {
  KEY_STATE_A = 0x7004,
  KEY_STATE_B,
  KEY_STATE_C,
  KEY_STATE_D,
  KEY_STATE_E,
  KEY_STATE_F,
  KEY_STATE_G,
  KEY_STATE_H,
  KEY_STATE_I,
  KEY_STATE_J,
  KEY_STATE_K,
  KEY_STATE_L,
  KEY_STATE_M,
  KEY_STATE_N,
  KEY_STATE_O,
  KEY_STATE_P,
  KEY_STATE_Q,
  KEY_STATE_R,
  KEY_STATE_S,
  KEY_STATE_T,
  KEY_STATE_U,
  KEY_STATE_V,
  KEY_STATE_W,
  KEY_STATE_X,
  KEY_STATE_Y,
  KEY_STATE_Z,
  KEY_STATE_1,
  KEY_STATE_2,
  KEY_STATE_3,
  KEY_STATE_4,
  KEY_STATE_5,
  KEY_STATE_6,
  KEY_STATE_7,
  KEY_STATE_8,
  KEY_STATE_9,
  KEY_STATE_0,
  KEY_STATE_ENTER,
  KEY_STATE_ESCAPE,
  KEY_STATE_BACKSPACE,
  KEY_STATE_TAB,
  KEY_STATE_SPACE,
  KEY_STATE_MINUS,          // - _
  KEY_STATE_EQUAL,          // = +
  KEY_STATE_LEFT_BRACKET,   // [ {
  KEY_STATE_RIGHT_BRACKET,  // ] }
  KEY_STATE_BACKSLASH,      // \ |
  KEY_STATE_UNKNOWN,
  KEY_STATE_SEMICOLON,      // ; :
  KEY_STATE_QUOTE,          // ' "
  KEY_STATE_GRAVE,          // ` ~
  KEY_STATE_COMMA,          // , <
  KEY_STATE_PERIOD,         // . >
  KEY_STATE_SLASH,          // / ?
  KEY_STATE_CAPS_LOCK,
  KEY_STATE_F1,
  KEY_STATE_F2,
  KEY_STATE_F3,
  KEY_STATE_F4,
  KEY_STATE_F5,
  KEY_STATE_F6,
  KEY_STATE_F7,
  KEY_STATE_F8,
  KEY_STATE_F9,
  KEY_STATE_F10,
  KEY_STATE_F11,
  KEY_STATE_F12,
  KEY_STATE_PRINT_SCREEN,   // F13
  KEY_STATE_SCROLL_LOCK,    // F14
  KEY_STATE_PAUSE,          // F15
  KEY_STATE_INSERT,
  KEY_STATE_HOME,
  KEY_STATE_PAGE_UP,
  KEY_STATE_DELETE,
  KEY_STATE_END,
  KEY_STATE_PAGE_DOWN,
  KEY_STATE_RIGHT,
  KEY_STATE_LEFT,
  KEY_STATE_DOWN,
  KEY_STATE_UP,
  KEY_STATE_NUM_LOCK,       // Num Clear
  KEY_STATE_NUM_DIV,
  KEY_STATE_NUM_MUL,
  KEY_STATE_NUM_MINUS,
  KEY_STATE_NUM_PLUS,
  KEY_STATE_NUM_ENTER,
  KEY_STATE_NUM_1,          // End
  KEY_STATE_NUM_2,          // Down
  KEY_STATE_NUM_3,          // PgDn
  KEY_STATE_NUM_4,          // Left
  KEY_STATE_NUM_5,
  KEY_STATE_NUM_6,          // Right
  KEY_STATE_NUM_7,          // Home
  KEY_STATE_NUM_8,          // Up
  KEY_STATE_NUM_9,          // PgUp
  KEY_STATE_NUM_0,          // Insert
  KEY_STATE_NUM_PERIOD,     // Delete
  KEY_STATE_SECTION,        // § ± (Non-US \ and |)
  KEY_STATE_APPLICATION,
  KEY_STATE_POWER,
  KEY_STATE_NUM_EQUAL,
  KEY_STATE_F13,
  KEY_STATE_F14,
  KEY_STATE_F15,
};

/**
  Read the current state of the keyboard.
 
  @param  This        Protocol instance pointer.
  @param  Modifiers   Pointer to the location to store the currently pressed
                      modifer keys.
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
  @retval EFI_INVALID_PARAMETER   One or more of the parameters are invalid.
 */

typedef
EFI_STATUS
(EFIAPI *KEY_STATE_PROTOCOL_READ_KEY_STATE)
(
 IN     KEY_STATE_PROTOCOL *This,
 OUT    UINT16             *Modifiers,
 IN OUT UINTN              *KeyCount,
 OUT    CHAR16             *Keys          OPTIONAL
 )
;


struct _KEY_STATE_PROTOCOL {
  UINT64                             Signature;
  KEY_STATE_PROTOCOL_READ_KEY_STATE  ReadKeyState;
};

extern EFI_GUID gKeyStateProtocolGuid;

#endif
