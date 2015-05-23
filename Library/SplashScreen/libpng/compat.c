/**
 * Copyright (c) 2015, baskingshark
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
#include "setjmp.h"
#include "stddef.h"

/**
 * Implementation of standard library routines that are not available in the
 * EDK.  These include:
 *  malloc/free/abort (defined in stdlib.h)
 *  memcmp/memcpy/memset (defined in string.h)
 *  longjmp (defined in setjmp.h)
 *
 * These are mapped to the nearest equivalents in the EDK.
 */

/** stdlib.h **/
void *
malloc(size_t size)
{
  EFI_STATUS   Status;
  VOID        *Buffer;
  Status = gBS->AllocatePool(EfiBootServicesData,
                             size,
                             &Buffer);
  if(EFI_ERROR(Status))
    Print(L"AllocatePool returned error - %r\n", Status);
  return EFI_SUCCESS == Status ? Buffer : NULL;
}

void free(void *ptr)
{
  if(ptr)
    gBS->FreePool(ptr);
}

void abort()
{
  gBS->Exit(gImageHandle, 0, 0, NULL);
}

/** string.h **/
int memcmp(const void *s1, const void *s2, size_t n)
{
  return (int) CompareMem(s1, s2, n);
}

void *memcpy(void *dst, const void *src, size_t n)
{
  return CopyMem(dst, src, n);
}

void *memset(void *b, int c, size_t len)
{
  return SetMem(b, len, c);
}

/** setjmp.h **/
void longjmp(jmp_buf env, int val)
{
  LongJump((BASE_LIBRARY_JUMP_BUFFER*)env, val);
}
