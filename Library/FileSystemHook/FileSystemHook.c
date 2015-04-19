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
#include <Library/BaseLib.h>
#include <Library/FileSystemHook.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Protocol/SimpleFileSystem.h>

/**
 ** Internal Types
 **/

/* HOOKED_SIMPLE_FILE_SYSTEM */
#if EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_REVISION != 0x00010000
#error EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_REVISION mismatch
#endif

typedef
struct _HOOKED_SIMPLE_FILE_SYSTEM {
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  SimpleFileSystemProtocol;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *Original;
  HOOKED_FILE_HOOKS                Hooks;
  VOID                            *Data;
} HOOKED_SIMPLE_FILE_SYSTEM;

#define HOOKED_FILE_SYSTEM_TO_EFI_FILE_SYSTEM(x) \
  (&((x)->SimpleFileSystemProtocol))
#define EFI_FILE_SYSTEM_TO_HOOKED_FILE_SYSTEM(x) \
  ((HOOKED_SIMPLE_FILE_SYSTEM*)(x))

/* HOOKED_FILE */
#if EFI_FILE_PROTOCOL_REVISION != 0x00010000
#error EFI_FILE_PROTOCOL_REVISION mismatch
#endif

typedef
struct _HOOKED_FILE {
  EFI_FILE_PROTOCOL  FileProtocol;
  EFI_FILE_PROTOCOL *Original;
  CHAR16            *Path;
  BOOLEAN            HooksActive;
  HOOKED_FILE_HOOKS  Hooks;
  VOID              *Data;
} HOOKED_FILE;

#define HOOKED_FILE_TO_EFI_FILE(x) (&((x)->FileProtocol))
#define EFI_FILE_TO_HOOKED_FILE(x) ((HOOKED_FILE*)(x))
#define HOOKED(Hf, Hook)           ((Hf)->HooksActive && ((Hf)->Hooks.Hook))

// Function prototypes
STATIC
EFI_STATUS
EFIAPI
FP_Open(IN  EFI_FILE_PROTOCOL*,
        OUT EFI_FILE_PROTOCOL**,
        IN  CHAR16*,
        IN  UINT64,
        IN  UINT64);

STATIC
EFI_STATUS
EFIAPI
FP_Close(IN EFI_FILE_PROTOCOL*);

STATIC
EFI_STATUS
EFIAPI
FP_Delete(IN EFI_FILE_PROTOCOL*);

STATIC
EFI_STATUS
EFIAPI
FP_Read(IN     EFI_FILE_PROTOCOL*,
        IN OUT UINTN*,
        OUT    VOID*);

STATIC
EFI_STATUS
EFIAPI
FP_Write(IN     EFI_FILE_PROTOCOL*,
         IN OUT UINTN*,
         OUT    VOID*);

STATIC
EFI_STATUS
EFIAPI
FP_GetPosition(IN  EFI_FILE_PROTOCOL*,
               OUT UINT64*);

STATIC
EFI_STATUS
EFIAPI
FP_SetPosition(IN EFI_FILE_PROTOCOL*,
               IN UINT64);

STATIC
EFI_STATUS
EFIAPI
FP_GetInfo(IN     EFI_FILE_PROTOCOL*,
           IN     EFI_GUID*,
           IN OUT UINTN*,
           OUT    VOID*);

STATIC
EFI_STATUS
EFIAPI
FP_SetInfo(IN EFI_FILE_PROTOCOL*,
           IN EFI_GUID*,
           IN UINTN,
           IN VOID*);

STATIC
EFI_STATUS
EFIAPI
FP_Flush(IN EFI_FILE_PROTOCOL*);

/**
  Create a file path given a directory and path.
 
  @param  Directory   The path of the source directory (must be well formed).
                      Can be NULL, which is assumed to be '\'.
  @param  Path        The path of the file/directory relative to Directory.
                      Can be NULL, which is assumed to be '.\'
 
  @return A new file path, or NULL if an error occurred
**/
STATIC
CHAR16*
EFIAPI
CreatePath(IN OPTIONAL CHAR16 *Directory,
           IN OPTIONAL CHAR16 *Path)
{
  EFI_STATUS  Status;
  CHAR16     *Result;
  UINTN       DirLen;
  UINTN       PathLen;
  UINTN       Size;
  UINTN       Idx;
  UINTN       NewLength;

  DirLen    = Directory ? StrLen(Directory) : 0;
  PathLen   = Path      ? StrLen(Path)      : 0;
  NewLength = DirLen + PathLen + 1 + 1;

  Status = gBS->AllocatePool(EfiBootServicesData,
                             (DirLen + PathLen + 2) * sizeof(CHAR16),
                             (VOID**)&Result);
  if(!EFI_ERROR(Status)) {
    if((Size = DirLen))
      gBS->CopyMem(Result, Directory, DirLen * sizeof(CHAR16));
    // Trailing '\'?
    if(!Size || Result[Size-1] != L'\\')
      Result[Size++] = L'\\';
    // Is Path absolute?
    if(Path && Path[0] == L'\\')
        Size = 0;
    // Append Path
    for(Idx = 0; Idx < PathLen; ++Idx) {
      if(Path[Idx] == L'.') {
        // Skip '.\'?
        if(Idx + 1 < PathLen && Path[Idx + 1] == L'\\') {
          Idx += 1;
        }
        // Skip '..\'?
        else if(Idx + 2 < PathLen &&
                Path[Idx + 1] == L'.' &&
                Path[Idx + 2] == L'\\') {
          Idx += 2;
          // Strip trailing '\'
          if(Size > 2)
            Size--;
          // Strip directory name
          while(Size > 1 && Result[Size-1] != L'\\')
            Size--;
        }
        else
          Result[Size++] = Path[Idx];
      }
      // Simplify //
      else if(Path[Idx] == L'\\' &&
              Idx + 1 < PathLen &&
              Path[Idx + 1] == L'\\')
        ; // Do nothing
      else
        Result[Size++] = Path[Idx];
    }
    Result[Size] = 0;
  }
  else {
    Print(L"Failed to allocate buffer for path - %r\n", Status);
    Result = NULL;
  }
  return Result;
}

/**
  Create a HOOKED_FILE from an EFI_FILE_PROTOCOL.
 
  @param  Directory   Path name of the containing directory.
                      Can be NULL for root directory.
  @param  Path        Path name of the file/directory being opened.
                      Can be NULL for root directory.
  @param  OrigFile    The EFI_FILE_PROTOCOL that has been opened.
  @param  OpenMode    The OpenMode used when OrigFile was opened.
  @param  Attributes  The Attributes used when OrigFile was opened.
  @param  Hooks       Pointer to struct of hook functions.
  @param  Data        Pointer to the hook's data.
  @param  HookedFile  Where to store the pointer to the HOOKED_FILE.
 
  @return EFI_SUCCESS           The HOOKED_FILE was successfully created.
  @return EFI_OUT_OF_RESOURCES  There was insufficient memory to create the
                                HOOKED_FILE.
 **/
STATIC
EFI_STATUS
EFIAPI
CreateFile(IN OPTIONAL CHAR16             *Directory,
           IN OPTIONAL CHAR16             *Path,
           IN          EFI_FILE_PROTOCOL  *OrigFile,
           IN          UINT64              OpenMode,
           IN          UINT64              Attributes,
           IN          HOOKED_FILE_HOOKS  *Hooks,
           IN OPTIONAL VOID               *Data,
           OUT         HOOKED_FILE       **HookedFile)
{
  HOOKED_FILE *Hf;
  EFI_STATUS   Status;

  Status = gBS->AllocatePool(EfiBootServicesData,
                             sizeof(*Hf),
                             (VOID**)&Hf);
  if(!EFI_ERROR(Status)) {
    gBS->SetMem(Hf, sizeof(*Hf), 0);
    Hf->FileProtocol.Revision    = EFI_FILE_PROTOCOL_REVISION;
    Hf->FileProtocol.Open        = FP_Open;
    Hf->FileProtocol.Close       = FP_Close;
    Hf->FileProtocol.Delete      = FP_Delete;
    Hf->FileProtocol.Read        = FP_Read;
    Hf->FileProtocol.Write       = FP_Write;
    Hf->FileProtocol.GetPosition = FP_GetPosition;
    Hf->FileProtocol.SetPosition = FP_SetPosition;
    Hf->FileProtocol.GetInfo     = FP_GetInfo;
    Hf->FileProtocol.SetInfo     = FP_SetInfo;
    Hf->FileProtocol.Flush       = FP_Flush;
    Hf->Original                 = OrigFile;
    Hf->Path                     = CreatePath(Directory, Path);
    Hf->Hooks                    = *Hooks;
    Hf->Data                     = Data;
    Hf->HooksActive              = (Hf->Hooks.Opened &&
                                    Hf->Hooks.Opened(Hf->Original,
                                                     Hf->Path,
                                                     OpenMode,
                                                     Attributes,
                                                     Hf->Data));
    *HookedFile                  = Hf;
  }
  else
    Print(L"Failed to allocate memory for HOOKED_FILE - %r\n", Status);
  return Status;
}

/**
  Free resources used by a HOOKED_FILE.
 
  The EFI_FILE_PROTOCOL is not closed and must be done elsewhere.

  @param  HookedFile  The HOOKED_FILE to free.

  @return EFI_SUCCESS           The HOOKED_FILE was successfully freed.
  @return EFI_INVALID_PARAMETER HookedFile was NULL.
 **/
STATIC
EFI_STATUS
EFIAPI
FreeFile(IN EFI_FILE_PROTOCOL *HookedFile)
{
  HOOKED_FILE *Hf;
  if(!HookedFile)
    return EFI_INVALID_PARAMETER;

  Hf = EFI_FILE_TO_HOOKED_FILE(HookedFile);

  if(Hf->Path)
    gBS->FreePool(Hf->Path);
  gBS->FreePool(Hf);
  return EFI_SUCCESS;
}

//
// EFI_FILE_PROTOCOL implementation
//

/**
  Opens a new file relative to the source file's location.

  Implementation for hooked files.

  @param  This        A pointer to the EFI_FILE_PROTOCOL instance that is the
                      file handle to the source location. This would typically
                      be an open handle to a directory.
  @param  NewHandle   A pointer to the location to return the opened handle for
                      the new file.
  @param  FileName    The Null-terminated string of the name of the file to be
                      opened. The file name may contain the following path
                      modifiers: "\", ".", and "..".
  @param  OpenMode    The mode to open the file. The only valid combinations 
                      that the file may be opened with are: Read, Read/Write,
                      or Create/Read/Write.
  @param  Attributes  Only valid for EFI_FILE_MODE_CREATE, in which case these
                      are the attribute bits for the newly created file.

  @retval EFI_SUCCESS           The file was opened.
  @retval EFI_NOT_FOUND         The specified file could not be found on the
                                device.
  @retval EFI_NO_MEDIA          The device has no medium.
  @retval EFI_MEDIA_CHANGED     The device has a different medium in it or the
                                medium is no longer supported.
  @retval EFI_DEVICE_ERROR      The device reported an error.
  @retval EFI_VOLUME_CORRUPTED  The file system structures are corrupted.
  @retval EFI_WRITE_PROTECTED   An attempt was made to create a file, or open a
                                file for write when the media is
                                write-protected.
  @retval EFI_ACCESS_DENIED     The service denied access to the file.
  @retval EFI_OUT_OF_RESOURCES  Not enough resources were available to open the
                                file.
  @retval EFI_VOLUME_FULL       The volume is full.
 **/
STATIC
EFI_STATUS
EFIAPI
FP_Open(IN  EFI_FILE_PROTOCOL  *This,
        OUT EFI_FILE_PROTOCOL **NewHandle,
        IN  CHAR16             *FileName,
        IN  UINT64              OpenMode,
        IN  UINT64              Attributes)
{
  EFI_FILE_PROTOCOL *NewFile;
  HOOKED_FILE       *HookedThis;
  HOOKED_FILE       *NewHookedFile;
  EFI_STATUS         Status;

  if(!This)
    return EFI_INVALID_PARAMETER;

  HookedThis = EFI_FILE_TO_HOOKED_FILE(This);
  Status     = HookedThis->Original->Open(HookedThis->Original,
                                          &NewFile,
                                          FileName,
                                          OpenMode,
                                          Attributes);
  if(!EFI_ERROR(Status)) {
    Status = CreateFile(HookedThis->Path,
                        FileName,
                        NewFile,
                        OpenMode,
                        Attributes,
                        &HookedThis->Hooks,
                        HookedThis->Data,
                        &NewHookedFile);
    if(!EFI_ERROR(Status)) {
      *NewHandle = HOOKED_FILE_TO_EFI_FILE(NewHookedFile);
    }
    else {
      Print(L"CreateFile failed - %r\n", Status);
      NewFile->Close(NewFile);
    }
  }
  else
    Print(L"Open failed - %r\n", Status);
  return Status;
}

/**
  Closes a specified file handle.

  Implementation for hooked files.

  @param  This        A pointer to the EFI_FILE_PROTOCOL instance that is the
                      file handle to close.

  @retval EFI_SUCCESS The file was closed.
 **/
STATIC
EFI_STATUS
EFIAPI
FP_Close(IN EFI_FILE_PROTOCOL *This)
{
  HOOKED_FILE *Hf;
  EFI_STATUS   Status;
  if(!This)
    return EFI_INVALID_PARAMETER;

  Hf = EFI_FILE_TO_HOOKED_FILE(This);
  Status = (HOOKED(Hf, Close)?
            Hf->Hooks.Close(Hf->Original, Hf->Data):
            Hf->Original->Close(Hf->Original));
  FreeFile(This);
  return Status;
}

/**
  Close and delete the file handle.

  Implementation for hooked files.

  @param  This                    A pointer to the EFI_FILE_PROTOCOL instance
                                  that is the handle to the file to delete.

  @retval EFI_SUCCESS             The file was closed and deleted, and the
                                  handle was closed.
  @retval EFI_WARN_DELETE_FAILURE The handle was closed, but the file was not
                                  deleted.
 **/
STATIC
EFI_STATUS
EFIAPI
FP_Delete(IN EFI_FILE_PROTOCOL *This) {
  HOOKED_FILE *Hf;
  EFI_STATUS   Status;
  if(!This)
    return EFI_INVALID_PARAMETER;

  Hf = EFI_FILE_TO_HOOKED_FILE(This);
  Status = (HOOKED(Hf, Delete)?
            Hf->Hooks.Delete(Hf->Original, Hf->Data):
            Hf->Original->Delete(Hf->Original));
  FreeFile(This);
  return Status;
}

/**
  Reads data from a file.

  Implementation for hooked files.

  @param  This        A pointer to the EFI_FILE_PROTOCOL instance that is the
                      file handle to read data from.
  @param  BufferSize  On input, the size of the Buffer. On output, the amount
                      of data returned in Buffer. In both cases, the size is
                      measured in bytes.
  @param  Buffer      The buffer into which the data is read.

  @retval EFI_SUCCESS           Data was read.
  @retval EFI_NO_MEDIA          The device has no medium.
  @retval EFI_DEVICE_ERROR      The device reported an error.
  @retval EFI_DEVICE_ERROR      An attempt was made to read from a deleted file.
  @retval EFI_DEVICE_ERROR      On entry, the current file position is beyond 
                                the end of the file.
  @retval EFI_VOLUME_CORRUPTED  The file system structures are corrupted.
  @retval EFI_BUFFER_TO_SMALL   The BufferSize is too small to read the current
                                directory entry. BufferSize has been updated 
                                with the size needed to complete the request.
 **/
STATIC
EFI_STATUS
EFIAPI
FP_Read(IN     EFI_FILE_PROTOCOL *This,
        IN OUT UINTN             *BufferSize,
        OUT    VOID              *Buffer)
{
  HOOKED_FILE *Hf;
  if(!This)
    return EFI_INVALID_PARAMETER;

  Hf = EFI_FILE_TO_HOOKED_FILE(This);
  return (HOOKED(Hf, Read)?
          Hf->Hooks.Read(Hf->Original,
                         BufferSize,
                         Buffer,
                         Hf->Data):
          Hf->Original->Read(Hf->Original,
                             BufferSize,
                             Buffer));
}

/**
  Writes data to a file.

  Implementation for hooked files.

  @param  This        A pointer to the EFI_FILE_PROTOCOL instance that is the
                      file handle to write data to.
  @param  BufferSize  On input, the size of the Buffer. On output, the amount
                      of data actually written. In both cases, the size is
                      measured in bytes.
  @param  Buffer      The buffer of data to write.

  @retval EFI_SUCCESS           Data was written.
  @retval EFI_UNSUPPORTED       Writes to open directory files are not
                                supported.
  @retval EFI_NO_MEDIA          The device has no medium.
  @retval EFI_DEVICE_ERROR      The device reported an error.
  @retval EFI_DEVICE_ERROR      An attempt was made to write to a deleted file.
  @retval EFI_VOLUME_CORRUPTED  The file system structures are corrupted.
  @retval EFI_WRITE_PROTECTED   The file or medium is write-protected.
  @retval EFI_ACCESS_DENIED     The file was opened read only.
  @retval EFI_VOLUME_FULL       The volume is full.
 **/
STATIC
EFI_STATUS
EFIAPI
FP_Write(IN     EFI_FILE_PROTOCOL *This,
         IN OUT UINTN             *BufferSize,
         IN     VOID              *Buffer)
{
  HOOKED_FILE *Hf;
  if(!This)
    return EFI_INVALID_PARAMETER;

  Hf = EFI_FILE_TO_HOOKED_FILE(This);
  return (HOOKED(Hf, Write)?
          Hf->Hooks.Write(Hf->Original,
                          BufferSize,
                          Buffer,
                          Hf->Data):
          Hf->Original->Write(Hf->Original,
                              BufferSize,
                              Buffer));
}

/**
  Sets a file's current position.

  Implementation for hooked files.

  @param  This            A pointer to the EFI_FILE_PROTOCOL instance that is
                          the file handle to set the requested position on.
  @param  Position        The byte position from the start of the file to set.

  @retval EFI_SUCCESS       The position was set.
  @retval EFI_UNSUPPORTED   The seek request for nonzero is not valid on open
                            directories.
  @retval EFI_DEVICE_ERROR  An attempt was made to set the position of a 
                            deleted file.
 **/
STATIC
EFI_STATUS
EFIAPI
FP_SetPosition(IN EFI_FILE_PROTOCOL *This,
               IN UINT64             Position)
{
  HOOKED_FILE *Hf;
  if(!This)
    return EFI_INVALID_PARAMETER;

  Hf = EFI_FILE_TO_HOOKED_FILE(This);
  return (HOOKED(Hf, SetPosition)?
          Hf->Hooks.SetPosition(Hf->Original,
                                Position,
                                Hf->Data):
          Hf->Original->SetPosition(Hf->Original,
                                    Position));
}

/**
  Returns a file's current position.

  Implementation for hooked files.

  @param  This            A pointer to the EFI_FILE_PROTOCOL instance that is
                          the file handle to get the current position on.
  @param  Position        The address to return the file's current position
                          value.

  @retval EFI_SUCCESS       The position was returned.
  @retval EFI_UNSUPPORTED   The request is not valid on open directories.
  @retval EFI_DEVICE_ERROR  An attempt was made to get the position from a
                            deleted file.
 **/
STATIC
EFI_STATUS
EFIAPI
FP_GetPosition(IN  EFI_FILE_PROTOCOL *This,
               OUT UINT64            *Position)
{
  HOOKED_FILE *Hf;
  if(!This)
    return EFI_INVALID_PARAMETER;

  Hf = EFI_FILE_TO_HOOKED_FILE(This);
  return (HOOKED(Hf, GetPosition)?
          Hf->Hooks.GetPosition(Hf->Original,
                                Position,
                                Hf->Data):
          Hf->Original->GetPosition(Hf->Original,
                                    Position));
}

/**
  Returns information about a file.

  Implementation for hooked files.

  @param  This            A pointer to the EFI_FILE_PROTOCOL instance that is
                          the file handle the requested information is for.
  @param  InformationType The type identifier for the information being
                          requested.
  @param  BufferSize      On input, the size of Buffer. On output, the amount 
                          of data returned in Buffer. In both cases, the size
                          is measured in bytes.
  @param  Buffer          A pointer to the data buffer to return. The buffer's
                          type is indicated by InformationType.

  @retval EFI_SUCCESS           The information was returned.
  @retval EFI_UNSUPPORTED       The InformationType is not known.
  @retval EFI_NO_MEDIA          The device has no medium.
  @retval EFI_DEVICE_ERROR      The device reported an error.
  @retval EFI_VOLUME_CORRUPTED  The file system structures are corrupted.
  @retval EFI_BUFFER_TOO_SMALL  The BufferSize is too small to read the current
                                directory entry. BufferSize has been updated
                                with the size needed to complete the request.
 **/
STATIC
EFI_STATUS
EFIAPI
FP_GetInfo(IN     EFI_FILE_PROTOCOL *This,
           IN     EFI_GUID          *InformationType,
           IN OUT UINTN             *BufferSize,
           OUT    VOID              *Buffer)
{
  HOOKED_FILE *Hf;
  if(!This)
    return EFI_INVALID_PARAMETER;

  Hf = EFI_FILE_TO_HOOKED_FILE(This);
  return (HOOKED(Hf, GetInfo)?
          Hf->Hooks.GetInfo(Hf->Original,
                            InformationType,
                            BufferSize,
                            Buffer,
                            Hf->Data):
          Hf->Original->GetInfo(Hf->Original,
                                InformationType,
                                BufferSize,
                                Buffer));
}

/**
  Sets information about a file.

  Implementation for hooked files.

  @param  File            A pointer to the EFI_FILE_PROTOCOL instance that is 
                          the file handle the information is for.
  @param  InformationType The type identifier for the information being set.
  @param  BufferSize      The size, in bytes, of Buffer.
  @param  Buffer          A pointer to the data buffer to write. The buffer's 
                          type is indicated by InformationType.

  @retval EFI_SUCCESS           The information was set.
  @retval EFI_UNSUPPORTED       The InformationType is not known.
  @retval EFI_NO_MEDIA          The device has no medium.
  @retval EFI_DEVICE_ERROR      The device reported an error.
  @retval EFI_VOLUME_CORRUPTED  The file system structures are corrupted.
  @retval EFI_WRITE_PROTECTED   InformationType is EFI_FILE_INFO_ID and the
                                media is read-only.
  @retval EFI_WRITE_PROTECTED   InformationType is
                                EFI_FILE_PROTOCOL_SYSTEM_INFO_ID and the media
                                is read only.
  @retval EFI_WRITE_PROTECTED   InformationType is
                                EFI_FILE_SYSTEM_VOLUME_LABEL_ID and the media is
                                read-only.
  @retval EFI_ACCESS_DENIED     An attempt is made to change the name of a file
                                to a file that is already present.
  @retval EFI_ACCESS_DENIED     An attempt is being made to change the
                                EFI_FILE_DIRECTORY Attribute.
  @retval EFI_ACCESS_DENIED     An attempt is being made to change the size of
                                a directory.
  @retval EFI_ACCESS_DENIED     InformationType is EFI_FILE_INFO_ID and the 
                                file was opened read-only and an attempt is 
                                being made to modify a field other than
                                Attribute.
  @retval EFI_VOLUME_FULL       The volume is full.
  @retval EFI_BAD_BUFFER_SIZE   BufferSize is smaller than the size of the type
                                indicated by InformationType.
 **/
STATIC
EFI_STATUS
EFIAPI
FP_SetInfo(IN EFI_FILE_PROTOCOL *This,
           IN EFI_GUID          *InformationType,
           IN UINTN              BufferSize,
           IN VOID              *Buffer)
{
  HOOKED_FILE *Hf;
  if(!This)
    return EFI_INVALID_PARAMETER;

  Hf = EFI_FILE_TO_HOOKED_FILE(This);
  return (HOOKED(Hf, SetInfo)?
          Hf->Hooks.SetInfo(Hf->Original,
                            InformationType,
                            BufferSize,
                            Buffer,
                            Hf->Data):
          Hf->Original->SetInfo(Hf->Original,
                                InformationType,
                                BufferSize,
                                Buffer));
}

/**
  Flushes all modified data associated with a file to a device.

  Implementation for hooked files.

  @param  This  A pointer to the EFI_FILE_PROTOCOL instance that is the file
                handle to flush.

  @retval EFI_SUCCESS          The data was flushed.
  @retval EFI_NO_MEDIA         The device has no medium.
  @retval EFI_DEVICE_ERROR     The device reported an error.
  @retval EFI_VOLUME_CORRUPTED The file system structures are corrupted.
  @retval EFI_WRITE_PROTECTED  The file or medium is write-protected.
  @retval EFI_ACCESS_DENIED    The file was opened read-only.
  @retval EFI_VOLUME_FULL      The volume is full.
 **/
STATIC
EFI_STATUS
EFIAPI
FP_Flush(IN EFI_FILE_PROTOCOL *This) {
  HOOKED_FILE *Hf;
  if(!This)
    return EFI_INVALID_PARAMETER;

  Hf = EFI_FILE_TO_HOOKED_FILE(This);
  return (HOOKED(Hf, Flush)?
          Hf->Hooks.Flush(Hf->Original, Hf->Data):
          Hf->Original->Flush(Hf->Original));
}

/**
 ** FILE_SYSTEM
 **/

/**
  Open the root directory on a volume.
 
  Implementation for hooked file system.

  @param  This  A pointer to the volume to open the root directory.
  @param  Root  A pointer to the location to return the opened file handle for
                the root directory.

  @retval EFI_SUCCESS           The device was opened.
  @retval EFI_UNSUPPORTED       This volume does not support the requested file
                                system type.
  @retval EFI_NO_MEDIA          The device has no medium.
  @retval EFI_DEVICE_ERROR      The device reported an error.
  @retval EFI_VOLUME_CORRUPTED  The file system structures are corrupted.
  @retval EFI_ACCESS_DENIED     The service denied access to the file.
  @retval EFI_OUT_OF_RESOURCES  The volume was not opened due to lack of
                                resources.
  @retval EFI_MEDIA_CHANGED     The device has a different medium in it or the
                                medium is no longer supported. Any existing
                                file handles for this volume are no longer
                                valid. To access the files on the new medium,
                                the volume must be reopened with OpenVolume().
 */
STATIC
EFI_STATUS
EFIAPI
SFSP_OpenVolume(IN  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *This,
                OUT EFI_FILE_PROTOCOL               **Root)
{
  HOOKED_SIMPLE_FILE_SYSTEM *Hsfs = (HOOKED_SIMPLE_FILE_SYSTEM*)This;
  EFI_FILE_PROTOCOL         *RealRoot;
  HOOKED_FILE               *HookedRoot;
  EFI_STATUS                 Status;

  if(!This || !Root)
    return EFI_INVALID_PARAMETER;

  Status = Hsfs->Original->OpenVolume(Hsfs->Original, &RealRoot);
  if(!EFI_ERROR(Status)) {
    Status = CreateFile(NULL,
                        NULL,
                        RealRoot,
                        // TODO - CHECK THESE
                        EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE,
                        EFI_FILE_DIRECTORY,
                        &Hsfs->Hooks,
                        Hsfs->Data,
                        &HookedRoot);
    if(!EFI_ERROR(Status)) {
      *Root = HOOKED_FILE_TO_EFI_FILE(HookedRoot);
    }
    else {
      Print(L"CreateFile failed - %r\n", Status);
      RealRoot->Close(RealRoot);
    }
  }
  else
    Print(L"OpenVolume failed - %r\n", Status);

  return Status;
}

/**
  Create a hooked File File System instance.
 
  @param  Hooks   Pointer to hook functions.
  @param  Data    Pointer to the hook's data.
  @param  Orig    The original EFI_SIMPLE_FILE_SYSTEM_PROTOCOL instance.
  @param  Hooked  Where to store the Hooked File System.
 
  @return EFI_SUCCESS           The Hooked File System was successfully created.
  @return EFI_OUT_OF_RESOURCES  The file system was not hooked becuase of a lack
                                of resources.
  */
STATIC
EFI_STATUS
EFIAPI
CreateFileSystem(IN  HOOKED_FILE_HOOKS                *Hooks,
                 IN  VOID                             *Data,
                 IN  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *Orig,
                 OUT EFI_SIMPLE_FILE_SYSTEM_PROTOCOL **Hooked)
{
  HOOKED_SIMPLE_FILE_SYSTEM *Hsfs;
  EFI_STATUS                 Status;

  Status = gBS->AllocatePool(EfiBootServicesData,
                             sizeof(*Hsfs),
                             (VOID**)&Hsfs);
  if(!EFI_ERROR(Status)) {
    gBS->SetMem(Hsfs, sizeof(*Hsfs), 0);
    Hsfs->SimpleFileSystemProtocol.Revision =
      EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_REVISION;
    Hsfs->SimpleFileSystemProtocol.OpenVolume = SFSP_OpenVolume;
    Hsfs->Original = Orig;
    Hsfs->Hooks    = *Hooks;
    Hsfs->Data     = Data;
    *Hooked        = HOOKED_FILE_SYSTEM_TO_EFI_FILE_SYSTEM(Hsfs);
  }
  return Status;
}

/**
  Free resources used by a HOOKED_SIMPLE_FILE_SYSTEM.

  @param  Hooked  The HOOKED_SIMPLE_FILE_SYSTEM to free.

  @return EFI_SUCCESS           The HOOKED_SIMPLE_FILE_SYSTEM was successfully
                                freed.
  @return EFI_INVALID_PARAMETER Hooked was NULL.
 **/
STATIC
EFI_STATUS
EFIAPI
FreeFileSystem(IN EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *Hooked)
{
  HOOKED_SIMPLE_FILE_SYSTEM *Hsfs;
  if(!Hooked)
    return EFI_INVALID_PARAMETER;

  Hsfs = EFI_FILE_SYSTEM_TO_HOOKED_FILE_SYSTEM(Hooked);

  gBS->FreePool(Hsfs);
  return EFI_SUCCESS;
}

/**
 ** PUBLIC API
 **/

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
                     IN OPTIONAL VOID              *Data)
{
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *Hooked;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *Sfsp;
  EFI_STATUS                       Status;

  if(!Hooks || !Hooks->Opened)
    return EFI_INVALID_PARAMETER;

  Status = gBS->OpenProtocol(Handle,
                             &gEfiSimpleFileSystemProtocolGuid,
                             (VOID**)&Sfsp,
                             gImageHandle,
                             NULL,
                             EFI_OPEN_PROTOCOL_GET_PROTOCOL);
  if(!EFI_ERROR(Status)) {
    Status = CreateFileSystem(Hooks, Data, Sfsp, &Hooked);
    if(!EFI_ERROR(Status)) {
      Status = gBS->ReinstallProtocolInterface(Handle,
                                               &gEfiSimpleFileSystemProtocolGuid,
                                               Sfsp,
                                               Hooked);
      if(EFI_ERROR(Status)) {
        Print(L"ReinstallProtocolInterface failed - %r\n", Status);
        FreeFileSystem(Hooked);
      }
    }
    else
      Print(L"CreateFileSystem failed - %r\n", Status);
  }
  else
    Print(L"OpenProtocol failed - %r\n", Status);

  return Status;
}
