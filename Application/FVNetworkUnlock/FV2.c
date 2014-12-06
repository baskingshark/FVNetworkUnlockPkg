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
#include <Library/BaseMemoryLib.h>
#include <Library/DevicePathLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Protocol/BlockIo.h>
#include <Protocol/SimpleFileSystem.h>
#include "FV2.h"

/**
  Path of boot loader
 */
STATIC
CONST
CHAR16 BOOT_LOADER_NAME[] = L"\\System\\Library\\CoreServices\\boot.efi";

/**
  Check whether a buffer contains a valid FV2 header.
 
  This currently just checks for the presence of the Core Storage signature
  ("CS" at offset 88)
 
  @param  Buffer      Pointer to the buffer containing the FV2 header.
  @param  BufferSize  Size of the buffer in bytes.  Must be at least 512 bytes.
 
  @retval TRUE        Buffer contains a FV2 header
  @retval FALSE       Buffer does not cotain a valid header
 */
STATIC
BOOLEAN
EFIAPI
CheckFV2Header(IN CONST UINT8 *Buffer,
               IN       UINTN  BufferSize)
{
  BOOLEAN Result = FALSE;
  
  if(BufferSize >= 512) {
    if(Buffer[88] == 'C' && Buffer[89] == 'S')
      Result = TRUE;
  }
  return Result;
}

/**
  Check whether a partition contains a valid FV2 header.
 
  @param  PartitionHandle   Handle to a partition which must support the Block
                            Io Protocol.
 
  @retval TRUE              Partition contains a valid FV2 header
  @retval FALSE             Partition does not contain a valid FV2 header, or
                            there was a problem reading the header.
 */
STATIC
BOOLEAN
EFIAPI
CheckFV2Handle(IN EFI_HANDLE PartitionHandle)
{
  EFI_BLOCK_IO_PROTOCOL *BlockIO;
  EFI_BLOCK_IO_MEDIA    *Media;
  EFI_STATUS             Status;
  VOID                  *Buffer;
  BOOLEAN                Result = FALSE;

  Status = gBS->OpenProtocol(PartitionHandle,
                             &gEfiBlockIoProtocolGuid,
                             (VOID**)&BlockIO,
                             gImageHandle,
                             NULL,
                             EFI_OPEN_PROTOCOL_GET_PROTOCOL);
  if(!EFI_ERROR(Status)) {
    Media = BlockIO->Media;
    if(Media->LogicalPartition) {
      Status = gBS->AllocatePool(EfiBootServicesData,
                                 Media->BlockSize,
                                 &Buffer);
      if(!EFI_ERROR(Status)) {
        Status = BlockIO->ReadBlocks(BlockIO,
                                     Media->MediaId,
                                     0,
                                     Media->BlockSize,
                                     Buffer);
        if(!EFI_ERROR(Status)) {
          Result = CheckFV2Header(Buffer, Media->BlockSize);
        }
        else
          Print(L"Failed to ReadBlocks - %r\n", Status);
        gBS->FreePool(Buffer);
      }
      else
        Print(L"Failed to AllocatePool - %r\n", Status);
    }
  }
  else
    Print(L"Failed to Open BLOCK_IO_PROTOCOL - %r\n", Status);

  return Result;
}

/**
  Identify the handle of the specified partition on the specified block device.
 
  This looks for a device with a device path with the provided prefix followed
  by a Hard Drive Media Device Path node.  This is checked against the required
  partition number.
 
  @param  DriveDevicePath   Device path of the drive.
  @param  PartitionNumber   Index of the required partition.
 
  @return The handle to the partition, or NULL if the partition was not found.
 */
STATIC
EFI_HANDLE
EFIAPI
LocatePartition(IN CONST EFI_DEVICE_PATH_PROTOCOL *DriveDevicePath,
                IN       UINT32                    PartitionNumber)
{
  EFI_DEVICE_PATH_PROTOCOL *DevicePath;
  EFI_DEVICE_PATH_PROTOCOL *CurrentNode;
  HARDDRIVE_DEVICE_PATH    *HardDrive;
  EFI_HANDLE               *Handles;
  UINTN                     DevicePathDetailSize;
  UINTN                     DevicePathSize;
  UINTN                     NumberOfHandles;
  UINTN                     Index;
  EFI_STATUS                Status;
  EFI_HANDLE                Result = NULL;

  DevicePathSize = GetDevicePathSize(DriveDevicePath);
  DevicePathDetailSize = DevicePathSize - END_DEVICE_PATH_LENGTH;
  Status = gBS->LocateHandleBuffer(ByProtocol,
                                   &gEfiBlockIoProtocolGuid,
                                   NULL,
                                   &NumberOfHandles,
                                   &Handles);
  if(!EFI_ERROR(Status)) {
    Print(L"Got %d block devices\n", NumberOfHandles);
    for(Index = 0; Index < NumberOfHandles; Index++) {
      DevicePath = DevicePathFromHandle(Handles[Index]);
      if(GetDevicePathSize(DevicePath) > DevicePathSize) {
        if(!CompareMem(DevicePath, DriveDevicePath, DevicePathDetailSize)) {
          CurrentNode = (VOID*)DevicePath + DevicePathDetailSize;
          if(DevicePathType(CurrentNode) == MEDIA_DEVICE_PATH &&
             DevicePathSubType(CurrentNode) == MEDIA_HARDDRIVE_DP) {
            HardDrive = (HARDDRIVE_DEVICE_PATH*)CurrentNode;
            if(HardDrive->PartitionNumber == PartitionNumber) {
              Result = Handles[Index];
              break;
            }
          }
        }
      }
    }
    gBS->FreePool(Handles);
  }
  else
    Print(L"Failed to locate BLOCK_IO_PROTOCOL - %r\n", Status);

  return Result;
}

/**
  Check whether the given handle is a FV2 Boot (Apple Recovery) partition.
 
  A handle is considered to be a FV2 boot partition if it is a disk partition
  and the preeceding partition on the disk is a FV2 partition.
 
  @param  Handle  The handle of the partition to check.
 
  @retval TRUE    The handle is a FV2 boot partition
  @retval FALSE   The handle does not belong to a boot partition or there was a
                  problem reading from the partition.
 */
STATIC
BOOLEAN
EFIAPI
IsFV2BootPartition(IN EFI_HANDLE Handle)
{
  EFI_DEVICE_PATH_PROTOCOL *HandleDevicePath;
  EFI_DEVICE_PATH_PROTOCOL *DriveDevicePath;
  EFI_DEVICE_PATH_PROTOCOL *CurrentNode;
  HARDDRIVE_DEVICE_PATH    *HardDrive;
  UINT32                    PartitionNumber;
  BOOLEAN                   Result = FALSE;

  HandleDevicePath = DevicePathFromHandle(Handle);
  if(HandleDevicePath) {
    DriveDevicePath = DuplicateDevicePath(HandleDevicePath);
    if(DriveDevicePath) {
      for(CurrentNode = DriveDevicePath;
          !IsDevicePathEnd(CurrentNode);
          CurrentNode = NextDevicePathNode(CurrentNode)) {
        if(DevicePathType(CurrentNode) == MEDIA_DEVICE_PATH &&
           DevicePathSubType(CurrentNode) == MEDIA_HARDDRIVE_DP) {
          HardDrive = (HARDDRIVE_DEVICE_PATH*)CurrentNode;
          if(HardDrive->MBRType == MBR_TYPE_EFI_PARTITION_TABLE_HEADER) {
            PartitionNumber = HardDrive->PartitionNumber;
            SetDevicePathEndNode(CurrentNode);
            Result = CheckFV2Handle(LocatePartition(DriveDevicePath,
                                                    PartitionNumber-1));
            break;
          }
        }
      }
      gBS->FreePool(DriveDevicePath);
    }
  }
  return Result;
}

/**
  Check whether the given handle contains a filesytem with a valid boot loader.

  @param  Handle  The handle of the partition to check.

  @retval TRUE    The handle is a partition containing a boot loader
  @retval FALSE   The handle does not belong to a boot partition or there was a
                  problem reading from the partition.
 */
STATIC
BOOLEAN
EFIAPI
HasBootLoader(IN EFI_HANDLE Handle)
{
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *SimpleFileSystem;
  EFI_FILE_PROTOCOL               *Root;
  EFI_FILE_PROTOCOL               *Boot;
  EFI_STATUS                       Status;
  BOOLEAN                          Result = FALSE;

  Status = gBS->OpenProtocol(Handle,
                             &gEfiSimpleFileSystemProtocolGuid,
                             (VOID**)&SimpleFileSystem,
                             gImageHandle,
                             NULL,
                             EFI_OPEN_PROTOCOL_GET_PROTOCOL);
  if(!EFI_ERROR(Status)) {
    Status = SimpleFileSystem->OpenVolume(SimpleFileSystem, &Root);
    if(!EFI_ERROR(Status)) {
      Status = Root->Open(Root,
                          &Boot,
                          (CHAR16*)BOOT_LOADER_NAME,
                          EFI_FILE_MODE_READ,
                          0);
      Root->Close(Root);
      if(!EFI_ERROR(Status)) {
        // Found potential boot loader
        Print(L"Found boot loader\n");
        Result = TRUE;
        Boot->Close(Boot);
      }
      else {
        // Either boot loader is not found, or an error
        // In either case, skip it
        Print(L"Failed to open boot file - %r\n", Status);
      }
    }
    else {
      // Failed to open volume, so can't check for boot loader
      // Skip it
      Print(L"Failed to open volume - %r\n", Status);
    }
  }
  else
    Print(L"Failed to open protocol - %r\n", Status);

  return Result;
}

/**
  Locate FV2 boot loaders.
 
  @param  BootLoaderCount   Location to store the number of boot loaders found.
  @param  BootLoaders       Location to store pointer to list of boot loaders.
 
  @retval EFI_SUCCESS             The search was successful.
  @retval EFI_NOT_FOUND           No boot loaders were found.
  @retval EFI_OUT_OF_RESOURCES    There is not enough memory to complete the
                                  search.
  @retval EFI_INVALID_PARAMETER   One or more of the parameters was invalid.
 */
EFI_STATUS
EFIAPI
LocateFV2BootLoaders(OUT UINTN        *BootLoaderCount,
                     OUT BOOT_LOADER **BootLoaders)
{
  BOOT_LOADER *Loaders;
  BOOT_LOADER *Loader;
  EFI_HANDLE  *Handles;
  EFI_STATUS   Status;
  UINTN        HandleCount;
  UINTN        BootCount;
  UINTN        Index;

  if(!BootLoaderCount || !BootLoaders)
    return EFI_INVALID_PARAMETER;

  Status = gBS->LocateHandleBuffer(ByProtocol,
                                   &gEfiSimpleFileSystemProtocolGuid,
                                   NULL,
                                   &HandleCount,
                                   &Handles);
  Print(L"Found %d file systems ...\n", HandleCount);
  if(!EFI_ERROR(Status)) {
    for(BootCount = Index = 0; Index < HandleCount; Index++) {
      Print(L"Checking FS %d\n", Index);
      if(HasBootLoader(Handles[Index]) && IsFV2BootPartition(Handles[Index]))
        BootCount++;
      else
        Handles[Index] = NULL;
    }
    // Allocate struct of boot loaders
    Status = gBS->AllocatePool(EfiBootServicesData,
                               BootCount * sizeof(BOOT_LOADER),
                               (VOID**)&Loaders);
    if(!EFI_ERROR(Status)) {
      for(BootCount = Index = 0; Index < HandleCount; Index++) {
        if(Handles[Index] != NULL) {
          Loader = Loaders + BootCount;
          Loader->VolumeHandle = Handles[Index];
          Loader->BootLoader = FileDevicePath(Handles[Index], BOOT_LOADER_NAME);
          if(!Loader->BootLoader) {
            FreeFV2BootLoaders(BootCount, Loaders);
            Status = EFI_OUT_OF_RESOURCES;
            break;
          }
          else
            BootCount++;
        }
      }
      if(BootCount) {
        *BootLoaderCount = BootCount;
        *BootLoaders = Loaders;
      }
      else
        Status = EFI_NOT_FOUND;
    }
    gBS->FreePool(Handles);
  }
  else
    Print(L"Failed to LoacteHandleBuffer - %r\n", Status);

  return Status;
}

/**
  Free list of FV2 boot loaders.
 
  @param  BootLoaderCount   Number of boot loaders returned by
                            LocateFV2BootLoaders.
  @param  BootLoaders       Pointer to boot loaders returned by
                            LocateFV2BootLoaders.
 */
VOID
EFIAPI
FreeFV2BootLoaders(IN UINTN        BootLoaderCount,
                   IN BOOT_LOADER *BootLoaders)
{
  UINTN Index;
  for(Index = 0; Index < BootLoaderCount; Index++)
    gBS->FreePool(BootLoaders[Index].BootLoader);
  gBS->FreePool(BootLoaders);
}