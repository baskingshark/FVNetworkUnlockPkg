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
#include <Guid/FileInfo.h>
#include <Library/DevicePathLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Protocol/LoadFile.h>
#include <Protocol/SimpleFileSystem.h>

/**
  Allocate a replacement buffer.
 
  Any existing buffer is freed before the new buffer is allocated.  Memory
  should be freed with FreePool.

  @param  Size    The size of the new buffer.
  @param  Buffer  Pointer to location containing the current buffer/where the
                  pointer to the new buffer will be stored.
  @retval EFI_SUCCESS           The buffer was allocated successfully.
  @retval EFI_OUT_OF_RESOURCES  The requested buffer could not be allocated.
 */
STATIC
EFI_STATUS
EFIAPI
ReplaceBuffer(IN     UINTN   Size,
              IN OUT VOID  **Buffer)
{
  if(*Buffer) {
    gBS->FreePool(*Buffer);
    *Buffer = NULL;
  }

  return (Size ? gBS->AllocatePool(EfiBootServicesData, Size, Buffer)
               : EFI_SUCCESS);
}

/**
  Load a file using the Load File Protocol.
 
  @param  LoadFileProtocol  Pointer to the Load File Protocol instance to use
  @param  FilePath          Path of the file to load
  @param  Size              Location to store the size of the file
  @param  Buffer            Location to store the pointer to the data
 
  @retval EFI_SUCCESS           The file was loaded successfully.
  @retval EFI_OUT_OF_RESOURCES  There was insufficient memory to load the file.
  @retval EFI_NO_MEDIA          No medium was present to load the file.
  @retval EFI_DEVICE_ERROR      The file was not loaded due to a device error.
  @retval EFI_NO_RESPONSE       The remote system did not respond.
  @retval EFI_NOT_FOUND         The file was not found.
  @retval EFI_ABORTED           The file load process was manually cancelled.
 */
STATIC
EFI_STATUS
EFIAPI
LoadViaLoadFile(IN  EFI_LOAD_FILE_PROTOCOL  *LoadFileProtocol,
                IN  CHAR16                  *FilePath,
                OUT UINTN                   *Size,
                OUT VOID                   **Buffer)
{
  EFI_DEVICE_PATH_PROTOCOL *DevicePath;
  EFI_STATUS                Status;
  UINTN                     FileSize;
  VOID                     *FileBuffer;

  DevicePath = FileDevicePath(NULL, FilePath);
  if(!DevicePath)
    return EFI_OUT_OF_RESOURCES;

  FileSize   = 0;
  FileBuffer = NULL;
  do {
    Status = ReplaceBuffer(FileSize, &FileBuffer);
    if(!EFI_ERROR(Status))
      Status = LoadFileProtocol->LoadFile(LoadFileProtocol,
                                          DevicePath,
                                          FALSE,
                                          &FileSize,
                                          FileBuffer);
  } while(EFI_BUFFER_TOO_SMALL == Status);

  if(!EFI_ERROR(Status)) {
    *Buffer = FileBuffer;
    *Size   = FileSize;
  }
  else if(FileBuffer)
    gBS->FreePool(FileBuffer);

  gBS->FreePool(DevicePath);
  return Status;
}

/**
  Load a file using the Simple File System Protocol.

  @param  Volume    Pointer to the Simple File System Protocol instance to use
  @param  FilePath  Path of the file to load
  @param  Size      Location to store the size of the file
  @param  Buffer    Location to store the pointer to the data

  @retval EFI_SUCCESS           The file was loaded successfully.
  @retval EFI_OUT_OF_RESOURCES  There was insufficient memory to load the file.
  @retval EFI_NOT_FOUND         The file was not found.
  @retval EFI_UNSUPPORTED       The volume does not support the requested file system type.
  @retval EFI_NO_MEDIA          The device has no medium.
  @retval EFI_DEVICE_ERROR      The device reported an error.
  @retval EFI_VOLUME_CORRUPTED  The file system structures are corrupted.
  @retval EFI_ACCESS_DENIED     The service denied access to the file.
 */
STATIC
EFI_STATUS
EFIAPI
LoadViaSimpleFileSystem(IN  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *Volume,
                        IN  CHAR16                           *FilePath,
                        OUT UINTN                            *Size,
                        OUT VOID                            **Buffer)
{
  EFI_FILE_PROTOCOL *RootDirectory;
  EFI_FILE_PROTOCOL *File;
  EFI_FILE_INFO     *FileInfo;
  VOID              *FileBuffer;
  UINTN              InfoSize;
  UINTN              FileSize;
  EFI_STATUS         Status;

  Status = Volume->OpenVolume(Volume, &RootDirectory);
  if(!EFI_ERROR(Status)) {
    Print(L"Opening %s\n", FilePath);
    Status = RootDirectory->Open(RootDirectory,
                                 &File,
                                 FilePath,
                                 EFI_FILE_MODE_READ,
                                 0);
    if(!EFI_ERROR(Status)) {
      FileInfo = NULL;
      InfoSize = 0;
      do {
        Print(L"Resizing buffer to %d bytes\n", InfoSize);
        Status = ReplaceBuffer(InfoSize, (VOID**)&FileInfo);
        Print(L"Got %d bytes @ %p - %r\n", InfoSize, FileInfo, Status);
        if(!EFI_ERROR(Status)) {
          Status = File->GetInfo(File, &gEfiFileInfoGuid, &InfoSize, FileInfo);
          Print(L"%r\n", Status);
        }
      } while(EFI_BUFFER_TOO_SMALL == Status);
      if(!EFI_ERROR(Status) && FileInfo &&
         !(FileInfo->Attribute & EFI_FILE_DIRECTORY)) {
        FileBuffer = NULL;
        FileSize   = FileInfo->FileSize;
        Status = ReplaceBuffer(FileSize, &FileBuffer);
        if(!EFI_ERROR(Status)) {
          Print(L"Read(%p, %d, %p) = ", File, FileSize, FileBuffer);
          Status = File->Read(File, &FileSize, FileBuffer);
          Print(L"%r\n", Status);
          if(!EFI_ERROR(Status)) {
            *Buffer = FileBuffer;
            *Size   = FileSize;
          }
          else
            gBS->FreePool(FileBuffer);
        }
        else
          Print(L"Failed to allocate buffer for file - %r\n", Status);
        gBS->FreePool(FileInfo);
      }
      else
        Print(L"Failed to retrieve size of file - %r\n", Status);
      File->Close(File);
    }
    RootDirectory->Close(RootDirectory);
  }
  return Status;
}

/**
  Load a file from a device.
 
  The device must support the Simple File System Protocol or the Load File
  Protocol.

  @param  Device    The device from which to load the file
  @param  FilePath  Path of the file to load
  @param  Size      Location to store the size of the file
  @param  Buffer    Location to store the pointer to the data

  @retval EFI_SUCCESS           The file was loaded successfully.
  @retval EFI_OUT_OF_RESOURCES  There was insufficient memory to load the file.
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
LoadFile(IN  EFI_HANDLE   Device,
         IN  CHAR16      *FilePath,
         OUT UINTN       *Size,
         OUT VOID       **Buffer) {
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *SimpleFileSystemProtocol;
  EFI_LOAD_FILE_PROTOCOL          *LoadFileProtocol;
  EFI_STATUS                       Status;

  if(!Device || !FilePath || !Size || !Buffer)
    return EFI_INVALID_PARAMETER;

  Status = gBS->OpenProtocol(Device,
                             &gEfiSimpleFileSystemProtocolGuid,
                             (VOID**)&SimpleFileSystemProtocol,
                             gImageHandle,
                             NULL,
                             EFI_OPEN_PROTOCOL_GET_PROTOCOL);
  if(!EFI_ERROR(Status)) {
    Print(L"Attempting load via SimpleFileSystem\n");
    Status = LoadViaSimpleFileSystem(SimpleFileSystemProtocol,
                                     FilePath,
                                     Size,
                                     Buffer);
  }
  else if(EFI_UNSUPPORTED == Status) {
    Status = gBS->OpenProtocol(Device,
                               &gEfiLoadFileProtocolGuid,
                               (VOID**)&LoadFileProtocol,
                               gImageHandle,
                               NULL,
                               EFI_OPEN_PROTOCOL_GET_PROTOCOL);
    if(!EFI_ERROR(Status)) {
      Print(L"Attempting load via LoadFileSystem\n");
      Status = LoadViaLoadFile(LoadFileProtocol,
                               FilePath,
                               Size,
                               Buffer);
    }
    else
      Print(L"Failed to OpenProtocol - %r\n", Status);
  }
  else
    Print(L"Failed to OpenProtocol - %r\n", Status);

  return Status;
}