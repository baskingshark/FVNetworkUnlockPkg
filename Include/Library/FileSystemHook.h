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

#ifndef __FILE_SYSTEM_HOOK_H__
#define __FILE_SYSTEM_HOOK_H__

#include <Protocol/SimpleFileSystem.h>

/**
  User hook function called AFTER a file is opened.

  This is used to determine whether this file is of interest.

  @param  This        A pointer to the EFI_FILE_PROTOCOL instance that is the
                      file handle that has been opened.
  @param  FileName    The full path name of the file opened.
  @param  OpenMode    The mode used to open the file.
  @param  Attributes  The aatribute bits for a newly created file.
  @param  Data        Pointer to the hook's data.

  @return A BOOLEAN indicating whether the file is of interest. Hooks are only
          installed for files of interest.
 */
typedef
BOOLEAN
(EFIAPI *HOOKED_FILE_OPENED)
  (IN     EFI_FILE_PROTOCOL *This,
   IN     CHAR16            *FileName,
   IN     UINT64             OpenMode,
   IN     UINT64             Attributes,
   IN     VOID              *Data);

/**
  User hook function called when a file is closed.

  @param  This          A pointer to the EFI_FILE_PROTOCOL instance that is the
                        file handle to close
  @param  Data          Pointer to the hook's data.

  @return EFI_SUCCESS   The hook function closed the file.
 */
typedef
EFI_STATUS
(EFIAPI *HOOKED_FILE_CLOSE)
  (IN     EFI_FILE_PROTOCOL *This,
   IN     VOID              *Data);

/**
  User hook function called when a file is deleted.

  @param  This                    A pointer to the EFI_FILE_PROTOCOL instance
                                  that is the file handle to delete
  @param  Data                    Pointer to the hook's data.

  @return EFI_SUCCESS             The file was closed and deleted and the
                                  handle was closed
  @return EFI_WARN_DELETE_FAILURE The handle was closed but the file was not
                                  deleted
 */
typedef
EFI_STATUS
(EFIAPI *HOOKED_FILE_DELETE)
  (IN     EFI_FILE_PROTOCOL *This,
   IN     VOID              *Data);

/**
  User hook function called when a file is read.

  @param  This        A pointer to the EFI_FILE_PROTOCOL instance that is the
                      file handle to read data from.
  @param  BufferSize  On input, the size of the Buffer in bytes.
                      On output, the amount of data returned in Buffer in bytes.
  @param  Buffer      The buffer into which the data is read.
  @param  Data        Pointer to the hook's data.

  @return Any of return codes returned by EFI_FILE_PROTOCOL.Read
 */
typedef
EFI_STATUS
(EFIAPI *HOOKED_FILE_READ)
  (IN     EFI_FILE_PROTOCOL *This,
   IN OUT UINTN             *BufferSize,
   OUT    VOID              *Buffer,
   IN     VOID              *Data);

/**
  User hook function called when a file is written.

  @param  This        A pointer to the EFI_FILE_PROTOCOL instance that is the
                      file handle to write data to.
  @param  BufferSize  On input, the size of the Buffer in bytes.
                      On output, the amount of data written in bytes.
  @param  Buffer      The buffer into which the data is read.
  @param  Data        Pointer to the hook's data.

  @return Any of the return codes returned by EFI_FILE_PROTOCOL.Write
 */
typedef
EFI_STATUS
(EFIAPI *HOOKED_FILE_WRITE)
  (IN     EFI_FILE_PROTOCOL *This,
   IN OUT UINTN             *BufferSize,
   IN     VOID              *Buffer,
   IN     VOID              *Data);

/**
  User hook function called when setting the file's position.

  @param  This      A pointer to the EFI_FILE_PROTOCOL instance that is the
                    file handle to set the requested position on.
  @param  Position  The byte position from the start of the file to set.
  @param  Data      Pointer to the hook's data.

  @return Any of the return codes returned by EFI_FILE_PROTOCOL.SetPosition
 */
typedef
EFI_STATUS
(EFIAPI *HOOKED_FILE_SET_POSITION)
  (IN     EFI_FILE_PROTOCOL *This,
   IN     UINT64             Position,
   IN     VOID              *Data);

/**
  User hook function called when getting the file's positions.

  @param  This      A pointer to the EFI_FILE_PROTOCOL instance that is the file
                    handle to get the current position on.
  @param  Position  The address to return the file’s current position value.
  @param  Data      Pointer to the hook's data.

  @return Any of the return codes returned by EFI_FILE_PROTOCOL.GetPosition
 */
typedef
EFI_STATUS
(EFIAPI *HOOKED_FILE_GET_POSITION)
  (IN     EFI_FILE_PROTOCOL *This,
   OUT    UINT64            *Position,
   IN     VOID              *Data);

/**
  User hook function called when getting info about a file.

  @param  This              A pointer to the EFI_FILE_PROTOCOL instance that is
                            the file handle the requested information is for.
  @param  InformationType   The type identifier for the information being
                            requested.
  @param  BufferSize        On input, the size of Buffer in bytes.
                            On output, the amount of data returned in Buffer.
  @param  Buffer            A pointer to the data buffer to return.
  @param  Data              Pointer to the hook's data.

  @return Any of the return codes returned by EFI_FILE_PROTOCOL.GetInfo
 */
typedef
EFI_STATUS
(EFIAPI *HOOKED_FILE_GET_INFO)
  (IN     EFI_FILE_PROTOCOL *This,
   IN     EFI_GUID          *InformationType,
   IN OUT UINTN             *BufferSize,
   OUT    VOID              *Buffer,
   IN     VOID              *Data);

/**
  User hook founction called when setting information about a file.

  @param  This              A pointer to the EFI_FILE_PROTOCOL instance that is
                            the file handle the requested information is for.
  @param  InformationType   The type identifier for the information being set.
  @param  BufferSize        The size of Buffer in bytes.
  @param  Buffer            A pointer to the data buffer to write.
  @param  Data              Pointer to the hook's data.

  @return Any of the return codes returned by EFI_FILE_PROTOCOL.SetInfo
 */
typedef
EFI_STATUS
(EFIAPI *HOOKED_FILE_SET_INFO)
  (IN     EFI_FILE_PROTOCOL *This,
   IN     EFI_GUID          *InformationType,
   IN     UINTN              BufferSize,
   IN     VOID              *Buffer,
   IN     VOID              *Data);

/**
  User hook function called when flushing data to the file.

  @param  This  A pointer to the EFI_FILE_PROTOCOL instance that is the file
                handle to flush.
  @param  Data  Pointer to the hook's data.

  @return Any of the return codes returned by EFI_FILE_PROTOCOL.Flush
 */
typedef
EFI_STATUS
(EFIAPI *HOOKED_FILE_FLUSH)
  (IN     EFI_FILE_PROTOCOL *This,
   IN     VOID              *Data);

/**
  Structure containing function pointers called on file events.
 */
typedef
struct _HOOKED_FILE_HOOKS {
  HOOKED_FILE_OPENED       Opened;

  HOOKED_FILE_CLOSE        Close;
  HOOKED_FILE_DELETE       Delete;
  HOOKED_FILE_READ         Read;
  HOOKED_FILE_WRITE        Write;
  HOOKED_FILE_GET_POSITION GetPosition;
  HOOKED_FILE_SET_POSITION SetPosition;
  HOOKED_FILE_GET_INFO     GetInfo;
  HOOKED_FILE_SET_INFO     SetInfo;
  HOOKED_FILE_FLUSH        Flush;
} HOOKED_FILE_HOOKS;

/**
  Hook file activity on a given device.

  @param  Handle  Handle of the device to install filesystem hooks on.
  @param  Hooks   Pointer to hook functions.  NULL functions will have default
                  functionality.
  @param  Data    Hook data passed to each hook function.

  @return EFI_SUCCESS           The file system was successfully hooked.
  @return EFI_INVALID_PARAMETER One or more of the parameters are invalid.
  @return EFI_UNSUPPORTED       The provided handle does not support the Simple
                                File System Protocol.
  @return EFI_ACCESS_DENIED     The existing file system is in use.
  @return EFI_OUT_OF_RESOURCES  The files system could not be hooked due to a
                                lack of resources.
 */
EFI_STATUS
EFIAPI
HookSimpleFileSystem(IN          EFI_HANDLE         Handle,
                     IN          HOOKED_FILE_HOOKS *Hooks,
                     IN OPTIONAL VOID              *Data);

#endif
