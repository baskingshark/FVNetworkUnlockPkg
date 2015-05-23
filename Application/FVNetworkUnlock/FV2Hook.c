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
#include <Guid/FileInfo.h>
#include <Library/Aes.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/FileSystemHook.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Protocol/LoadedImage.h>
#include "FileLoad.h"
#include "FV2.h"
#include "FV2PlistFilter.h"

#define REQUIRED_FILE         L"EncryptedRoot.plist.wipekey"
#define REQUIRED_FILE_LENGTH  ((sizeof(REQUIRED_FILE) / 2) - 1)

/**
  Internal type holding information about open files.
 */
typedef struct _HOOKED_FILE {
  LIST_ENTRY         List;
  EFI_FILE_PROTOCOL *SrcFile;
  VOID              *Data;
  UINT64             Size;
  UINT64             Offset;
} HOOKED_FILE;

/**
  Linked list of open files.
 */
STATIC
LIST_ENTRY
HookedFiles = INITIALIZE_LIST_HEAD_VARIABLE(HookedFiles);

/**
  Create an entry in the HookedFiles list.
 
  @param  SrcFile   Original EFI_FILE_PROTOCOL used as a key.
  @param  FileSize  Size of the replacement file data.
  @param  FileData  Pointer to the replacement file data.
 
  @retval EFI_SUCCESS           The entry was created successfully.
  @retval EFI_OUT_OF_RESOURCES  There were insufficient resources to create the
                                entry in the HookedFiles list.
 */
STATIC
EFI_STATUS
EFIAPI
CreateFile(IN EFI_FILE_PROTOCOL *SrcFile,
           IN UINT64             FileSize,
           IN VOID              *FileData)
{
  HOOKED_FILE *File;
  EFI_STATUS   Status;

  Status = gBS->AllocatePool(EfiBootServicesData,
                             sizeof(HOOKED_FILE),
                             (VOID**)&File);
  if(!EFI_ERROR(Status)) {
    gBS->SetMem(File, sizeof(HOOKED_FILE), 0);
    File->SrcFile = SrcFile;
    File->Data    = FileData;
    File->Size    = FileSize;
    InsertTailList(&HookedFiles, &File->List);
  }
  return Status;
}

/**
  Retrieve an entry from the HookedFiles list.
 
  @param  SrcFile   The original EFI_FILE_PROTOCOL used as a key.
 
  @return A pointer to the entry in the list, or NULL if it could not be found.
 */
STATIC
HOOKED_FILE *
EFIAPI
GetFile(IN EFI_FILE_PROTOCOL *SrcFile)
{
  LIST_ENTRY *Node;
  for(Node = GetFirstNode(&HookedFiles);
      !IsNull(&HookedFiles, Node);
      Node = GetNextNode(&HookedFiles, Node)) {
    if(((HOOKED_FILE*)Node)->SrcFile == SrcFile)
      return (HOOKED_FILE*)Node;
  }
  return NULL;
}

/**
  Remove an entry from the HookedFiles list.
 
  @param  Node  The pointer to the entry in the HookedFiles list (as returned
                by GetFile).
 */
STATIC
VOID
EFIAPI
FreeFile(IN HOOKED_FILE* Node)
{
  gBS->FreePool(Node->Data);
  RemoveEntryList(&Node->List);
  gBS->FreePool(Node);
}

/**
  Process the EncryptedRoot.plist.wipekey file.
 
  This reads, decrypts, filters and then reencrypts the plist file to remove
  unwanted CryptoUsers from the list of CryptoUsers.
 
  @param  File    The EFI_FILE_PROTOCOLE for the original file.
  @param  Volume  Pointer to the FV2_VOLUME struct for the volume.

  @return A BOOLEAN indicating whether the file was successfuly processed and
          added to the list of hooked files.
 */
STATIC
BOOLEAN
EFIAPI
ProcessEncRootPlist(IN EFI_FILE_PROTOCOL *File,
                    IN FV2_VOLUME        *Volume)
{
  EFI_FILE_INFO *FileInfo;
  EFI_STATUS     Status;
  UINTN          BufferSize;
  UINTN          FileSize;
  VOID          *FileData;

  BufferSize = 0;
  FileInfo   = NULL;

  do {
    Status = File->GetInfo(File,
                           &gEfiFileInfoGuid,
                           &BufferSize,
                           (VOID*)FileInfo);
    if(EFI_BUFFER_TOO_SMALL == Status) {
      if(FileInfo) {
        gBS->FreePool(FileInfo);
        FileInfo = NULL;
      }
      Status = gBS->AllocatePool(EfiBootServicesData,
                                 BufferSize,
                                 (VOID**)&FileInfo);
      if(!EFI_ERROR(Status))
        Status = EFI_BUFFER_TOO_SMALL;
    }
  } while(EFI_BUFFER_TOO_SMALL == Status);

  if(!EFI_ERROR(Status) && FileInfo) {
    FileSize = FileInfo->FileSize;
    Status = gBS->AllocatePool(EfiBootServicesData,
                               FileSize,
                               (VOID**)&FileData);
    if(!EFI_ERROR(Status)) {
      Status = File->Read(File, &FileSize, FileData);
      if(!EFI_ERROR(Status)) {
        UINT8 Key[512/8];
        UINT8 Tweak[16] = {0};
        UINTN KeySize   = sizeof(Key);
        Status = GetXtsAesKey(Volume, &KeySize, Key);
        if(!EFI_ERROR(Status)) {
          Status = InvXtsAesCipher(KeySize * 8,
                                   Key,
                                   Tweak,
                                   FileSize,
                                   FileData,
                                   FileData);
          if(!EFI_ERROR(Status)) {
            if(!AsciiStrnCmp((CHAR8*)FileData, "<?xml", 5)) {
              UINTN NewFileSize = PlistFilter(FileData, FileSize, FileData);
              XtsAesCipher(KeySize * 8,
                           Key,
                           Tweak,
                           NewFileSize,
                           FileData,
                           FileData);
              Status = CreateFile(File, NewFileSize, FileData);
              // Zero out end of buffer to remove decrypted data
              gBS->SetMem(FileData + NewFileSize, FileSize - NewFileSize, 0);
            }
            else
              Status = EFI_INVALID_PARAMETER;
          }
          if(EFI_ERROR(Status)) {
            gBS->FreePool(FileData);
            Print(L"Processing of EncryptedRoot.plist.wipekey failed - %r\n",
                  Status);
          }
          // Zero out key
          gBS->SetMem(Key, sizeof(Key), 0);
        }
        else
          Print(L"Failed to get decryptio key - %r\n", Status);
      }
      else
        Print(L"Failed to read EncryptedRoot.plist.wipekey - %r\n", Status);
    }
    else
      Print(L"AllocatePool failed - %r\n", Status);
  }
  else
    Print(L"Failed to GetFileInfo for EncryptedRoot.plist.wipekey - %r\n",
          Status);

  if(FileInfo)
    gBS->FreePool(FileInfo);
  if(EFI_ERROR(Status))
    // On fail, rewind file to allow regular processing.
    File->SetPosition(File, 0);
  return !EFI_ERROR(Status);
}

/**
  Process an efires file.

  This looks for a replacement efires on the boot volume and, if found, uses
  it as a replacement.

  @param  File      The EFI_FILE_PROTOCOL for the original file.
  @param  FileName  The name of the efires file.

  @return A BOOLEAN indicating whether the file was successfuly processed and
          added to the list of hooked files.
 */
STATIC
BOOLEAN
EFIAPI
ProcessEfires(IN EFI_FILE_PROTOCOL *This,
              IN CHAR16            *FileName)
{
  EFI_STATUS                 Status;
  CHAR16                    *Start;
  CHAR16                    *Cur;
  UINTN                      FileSize;
  VOID                      *FileData;

  for(Cur = Start = FileName; *Cur; Cur++)
    if(L'\\' == *Cur)
      Start = Cur;
  Status = LoadFileFromBootDevice(Start + 1,
                                  &FileSize,
                                  &FileData);
  if(!EFI_ERROR(Status)) {
    Status = CreateFile(This, FileSize, FileData);
    if(EFI_ERROR(Status))
      gBS->FreePool(FileData);
  }
  return !EFI_ERROR(Status);
}

/**
  Open callback.
 
  Intercepts opens of EncryptedRoot.plist.wipekey and *.efires
 */
STATIC
BOOLEAN
EFIAPI
OpenCB(IN EFI_FILE_PROTOCOL *This,
       IN CHAR16            *FileName,
       IN UINT64             OpenMode,
       IN UINT64             Attributes,
       IN VOID              *Data)
{
  UINTN FileNameLen;

  FileNameLen = StrLen(FileName);
  if(FileNameLen >= REQUIRED_FILE_LENGTH &&
     !StrCmp(FileName + FileNameLen - REQUIRED_FILE_LENGTH, REQUIRED_FILE)) {
    return ProcessEncRootPlist(This, Data);
  }
  if(FileNameLen > 7 &&
     !StrCmp(FileName + FileNameLen -7, L".efires")) {
    return ProcessEfires(This, FileName);
  }
  return FALSE;
}

/**
  Close callback.
 */
STATIC
EFI_STATUS
EFIAPI
CloseCB(IN EFI_FILE_PROTOCOL *This,
        IN VOID              *Data)
{
  HOOKED_FILE *Node = GetFile(This);
  if(Node)
    FreeFile(Node);
  return This->Close(This);
}

/**
  Delete callback.
 */
STATIC
EFI_STATUS
EFIAPI
DeleteCB(IN EFI_FILE_PROTOCOL *This,
         IN VOID              *Data)
{
  HOOKED_FILE *Node = GetFile(This);
  if(Node)
    FreeFile(Node);
  return This->Delete(This);
}

/**
  Read callback.
 */
STATIC
EFI_STATUS
EFIAPI
ReadCB(IN     EFI_FILE_PROTOCOL *This,
       IN OUT UINTN             *BufferSize,
       OUT    VOID              *Buffer,
       IN     VOID              *Data)
{
  HOOKED_FILE *Node = GetFile(This);
  UINTN        Count;
  if(Node) {
    Count = MIN(*BufferSize, Node->Size - Node->Offset);
    if(Count) {
      gBS->CopyMem(Buffer, Node->Data, Count);
      Node->Offset += Count;
    }
    *BufferSize = Count;
    return EFI_SUCCESS;
  }
  else
    return This->Read(This, BufferSize, Buffer);
}

/**
  Write callback.
 */
STATIC
EFI_STATUS
EFIAPI
WriteCB(IN     EFI_FILE_PROTOCOL *This,
        IN OUT UINTN             *BufferSize,
        IN     VOID              *Buffer,
        IN     VOID              *Data)
{
  return EFI_WRITE_PROTECTED;
}

/**
  SetPosition callback.
 */
STATIC
EFI_STATUS
EFIAPI
SetPositionCB(IN EFI_FILE_PROTOCOL *This,
              IN UINT64             Position,
              IN VOID              *Data)
{
  HOOKED_FILE *Node = GetFile(This);
  if(Node) {
    Node->Offset = MIN(Node->Size, Position);
    return EFI_SUCCESS;
  }
  else
    return This->SetPosition(This, Position);
}

/**
  GetPosition callback.
 */
STATIC
EFI_STATUS
EFIAPI
GetPositionCB(IN  EFI_FILE_PROTOCOL *This,
              OUT UINT64            *Position,
              IN  VOID              *Data)
{
  HOOKED_FILE *Node = GetFile(This);
  if(Node) {
    *Position = Node->Offset;
    return EFI_SUCCESS;
  }
  else
    return This->GetPosition(This, Position);
}

/**
  GetInfo callback.
 */
STATIC
EFI_STATUS
EFIAPI
GetInfoCB(IN     EFI_FILE_PROTOCOL *This,
          IN     EFI_GUID          *InformationType,
          IN OUT UINTN             *BufferSize,
          OUT    VOID              *Buffer,
          IN     VOID              *Data)
{
  HOOKED_FILE *Node = GetFile(This);
  EFI_STATUS   Status;
  Status = This->GetInfo(This, InformationType, BufferSize, Buffer);
  if(!EFI_ERROR(Status) &&
     Node &&
     CompareGuid(InformationType, &gEfiFileInfoGuid)) {
    ((EFI_FILE_INFO*)Buffer)->FileSize = Node->Size;
  }
  return Status;
}

/**
  SetInfo callback.
 */
STATIC
EFI_STATUS
EFIAPI
SetInfoCB(IN EFI_FILE_PROTOCOL *This,
          IN EFI_GUID          *InformationType,
          IN UINTN              BufferSize,
          IN VOID              *Buffer,
          IN VOID              *Data)
{
  return EFI_WRITE_PROTECTED;
}

/**
  Hook file activity on the specified FV2_VOLUME.

  This allows filtering of the EncryptedRoot.plist.wipekey file and replacement
  of efires files.

  @param  Volume  The FileVault 2 volume to monitor.
 
  @return EFI_SUCCESS           The file system was successfully hooked.
  @return EFI_INVALID_PARAMETER One or more of the parameters are invalid.
  @return EFI_UNSUPPORTED       The provided handle does not support the Simple
                                File System Protocol.
  @return EFI_ACCESS_DENIED     The existing file system is in use.
  @return EFI_OUT_OF_RESOURCES  The file system could not be hooked due to a
                                lack of resources.
 */
EFI_STATUS
EFIAPI
HookVolume(IN FV2_VOLUME *Volume) {
  /**
    Set of hook functions used when hooking file activity
   */
  STATIC
  HOOKED_FILE_HOOKS Hfh = {
    OpenCB, CloseCB, DeleteCB,
    ReadCB, WriteCB,
    GetPositionCB, SetPositionCB,
    GetInfoCB, SetInfoCB,
    NULL
  };

  if(!Volume)
    return EFI_INVALID_PARAMETER;

  return HookSimpleFileSystem(Volume->BootVolumeHandle,
                              &Hfh,
                              Volume);
}
