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
#include <Protocol/ConsoleControl.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/UgaDraw.h>
#include "libpng/png.h"

/**
  Structure holding information about an image and the protocol used to
  display it
 */
typedef struct _IMAGE_INFO {
  BOOLEAN                           UseGop;
  union {
    EFI_GRAPHICS_OUTPUT_PROTOCOL   *Gop;
    EFI_UGA_DRAW_PROTOCOL          *Uga;
  };
  UINT32                            DisplayHeight;
  UINT32                            DisplayWidth;
  UINT32                            ImageHeight;
  UINT32                            ImageWidth;
  union {
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *ImageGop;
    EFI_UGA_PIXEL                  *ImageUga;
    VOID                           *Image;
  };
  union {
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL   BackgroundGop;
    EFI_UGA_PIXEL                   BackgroundUga;
  };
} IMAGE_INFO;

/**
  Identify the type of graphics protocol to use.

  The system should be using the Graphics Output Protocol, but older systems
  may be using the UGA (Universal Graphics Adapter) Draw Protocol.

  Each protocol is tried in turn to determine which to use.  When one is found,
  the width and height of the display is extracted and stored for later.

  @param  Info  A pointer to an IMAGE_INFO structure.  On success, the
                IMAGE_INFO structure is updated to indicate which procotol is
                in use, the width and the height of the display.

  @retval EFI_SUCCESS       The protocol instance was located and the width and
                            height of the display were determined.
  @retval EFI_NOT_FOUND     An instance of the Graphics Output Protocol or the
                            UGA Draw Protocol could not be found.
  @retval EFI_NOT_STARTED   The UGA video display is not initialized.
 */
STATIC
EFI_STATUS
EFIAPI
IdentifyGraphics(IN OUT IMAGE_INFO *Info)
{
  EFI_STATUS  Status;
  UINT32      ColourDepth;
  UINT32      RefreshRate;

  Status = gBS->LocateProtocol(&gEfiGraphicsOutputProtocolGuid,
                               NULL,
                               (VOID**)&Info->Gop);
  if(!EFI_ERROR(Status)) {
    Info->UseGop = TRUE;
    Info->DisplayWidth  = Info->Gop->Mode->Info->HorizontalResolution;
    Info->DisplayHeight = Info->Gop->Mode->Info->VerticalResolution;
  }
  else {
    Status = gBS->LocateProtocol(&gEfiUgaDrawProtocolGuid,
                                 NULL,
                                 (VOID**)&Info->Uga);
    if(!EFI_ERROR(Status)) {
      Status = Info->Uga->GetMode(Info->Uga,
                                  &Info->DisplayWidth,
                                  &Info->DisplayHeight,
                                  &ColourDepth,
                                  &RefreshRate);
    }
  }
  return Status;
}

/**
  Clear the screen to the required background colour.
 
  If the PNG file contains a bKGD chunk, the colour specified there is used as
  the background colour.  If no bKGD chunk is available, the screen is cleared
  to black (this is decided in the Info callback).
 
  @param  Info  A pointer to the IMAGE_INFO structure.
 */
STATIC
VOID
ClearScreen(IN IMAGE_INFO *Info)
{
  if(Info) {
    if(Info->UseGop)
      (VOID) Info->Gop->Blt(Info->Gop,
                            &Info->BackgroundGop,
                            EfiBltVideoFill,
                            0, 0,
                            0, 0,
                            Info->DisplayWidth, Info->DisplayHeight,
                            0);
    else
      (VOID) Info->Uga->Blt(Info->Uga,
                            &Info->BackgroundUga,
                            EfiUgaVideoFill,
                            0, 0,
                            0, 0,
                            Info->DisplayWidth, Info->DisplayHeight,
                            0);
  }
}

/**
  Display the image.
 
  The image is centered on the screen (if smaller than the screen) or cropped
  (if larger than the screen)
 
  @param  Info  A pointer to the IMAGE_INFO structure containing the image.
 */
STATIC
VOID
DisplayImage(IN IMAGE_INFO *Info)
{
  UINT32 SrcX;
  UINT32 SrcY;
  UINT32 DestX;
  UINT32 DestY;
  UINT32 Height;
  UINT32 Width;
  UINT32 Delta;

  if(Info && Info->Image) {
    if(Info->ImageWidth <= Info->DisplayWidth) {
      SrcX  = 0;
      DestX = (Info->DisplayWidth - Info->ImageWidth) / 2;
      Width = Info->ImageWidth;
    }
    else {
      SrcX  = (Info->ImageWidth - Info->DisplayWidth) / 2;
      DestX = 0;
      Width = Info->DisplayWidth;
    }
    if(Info->ImageHeight <= Info->DisplayHeight) {
      SrcY   = 0;
      DestY  = (Info->DisplayHeight - Info->ImageHeight) / 2;
      Height = Info->ImageHeight;
    }
    else {
      SrcY   = (Info->ImageHeight - Info->DisplayHeight) / 2;
      DestY  = 0;
      Height = Info->DisplayHeight;
    }
    Delta = Info->ImageWidth * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL);

    if(Info->UseGop)
      (VOID) Info->Gop->Blt(Info->Gop,
                            Info->ImageGop,
                            EfiBltBufferToVideo,
                            SrcX, SrcY,
                            DestX, DestY,
                            Width, Height,
                            Delta);
    else
      (VOID) Info->Uga->Blt(Info->Uga,
                            Info->ImageUga,
                            EfiUgaBltBufferToVideo,
                            SrcX, SrcY,
                            DestX, DestY,
                            Width, Height,
                            Delta);
  }
}

/**
  Callback to handle warnings generated by libpng.
 */
STATIC
VOID
WarningHandler(png_structp      png_ptr,
               png_const_charp  msg)
{
  Print(L"libpng warning: %a\n", msg);
}

/**
  Callback to handle errors generated by libpng.
 
  This uses longjmp to abort processing so setjmp must be called before this
  can be called.
 */
STATIC
VOID
ErrorHandler(png_structp      png_ptr,
             png_const_charp  msg)
{
  Print(L"libpng error: %a\n", msg);
  png_longjmp(png_ptr, 1);
}

/**
  Callback called when libpng has finished processing all metadata chunks.
 
  Once all the metadata chunks have been processed, the IMAGE_INFO structure is
  updated with the size of the image, various transforms are enabled and memory
  for the transformed image is allocated.
 */
STATIC
VOID
InfoCallback(png_structp  png_ptr,
             png_infop    info_ptr)
{
  IMAGE_INFO   *Info;
  VOID         *Buffer;
  EFI_STATUS    Status;
  png_color_16  Background;
  png_color_16p BackgroundPtr;
  png_uint_32   ImageWidth;
  png_uint_32   ImageHeight;
  png_size_t    RowBytes;

  Info = png_get_progressive_ptr(png_ptr);
  if(!Info)
    return;
  png_get_IHDR(png_ptr,
               info_ptr,
               &ImageWidth,
               &ImageHeight,
               NULL,          // bit_depth
               NULL,          // color_type
               NULL,          // interlace_type
               NULL,          // compression_type
               NULL           // filter_type
               );
  /*
   * Transforms ... convert everthing to 8-bit BGRA (as used by GOP/UGA) and
   * enable interlace handling.
   */
  png_set_gray_to_rgb(png_ptr);
  png_set_palette_to_rgb(png_ptr);
  png_set_expand(png_ptr);
  png_set_scale_16(png_ptr);
  png_set_bgr(png_ptr);
  (VOID) png_set_interlace_handling(png_ptr);
  // Have to extract info about background here as
  // png_set_background_fixed must be called before png_read_update_info.
  if(!png_get_bKGD(png_ptr, info_ptr, &BackgroundPtr))
  {
    // Set default background to black
    Background.blue  = 0;
    Background.gray  = 0;
    Background.green = 0;
    Background.index = 0;
    Background.red   = 0;
    BackgroundPtr    = &Background;
  }
  Info->BackgroundGop.Blue     = BackgroundPtr->blue;
  Info->BackgroundGop.Green    = BackgroundPtr->green;
  Info->BackgroundGop.Red      = BackgroundPtr->red;
  Info->BackgroundGop.Reserved = 0;
  png_set_background_fixed(png_ptr,
                           BackgroundPtr,
                           PNG_BACKGROUND_GAMMA_SCREEN,
                           1,
                           10000 /* Gamma of 1.0 */);
  png_read_update_info(png_ptr, info_ptr);

  RowBytes = png_get_rowbytes(png_ptr, info_ptr);
  if(RowBytes == ImageWidth * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL)) {
    Status = gBS->AllocatePool(EfiBootServicesData,
                               ImageHeight * RowBytes,
                               &Buffer);
    if(!EFI_ERROR(Status)) {
      Info->ImageHeight = ImageHeight;
      Info->ImageWidth  = ImageWidth;
      Info->Image       = Buffer;
    }
  }
}

/**
  Callback called when libpng has got data for a row of the image.

  The row data is merged in with the image stored in the IMAGE_INFO struct.
 */
STATIC
VOID
RowCallback(png_structp   png_ptr,
            png_bytep     new_row,
            png_uint_32   row_num,
            int           pass)
{
  IMAGE_INFO *Info;
  if(!new_row)
    return;
  Print(L"Row Callback: %d\n", row_num);
  Info = png_get_progressive_ptr(png_ptr);
  if(Info && Info->Image) {
    png_bytep RowStart = (png_bytep)(Info->ImageGop +
                                     row_num * Info->ImageWidth);
    png_progressive_combine_row(png_ptr,
                                RowStart,
                                new_row);
  }
}

/**
  Callback called when libpng has finished processing all the chunks.

  This is called once all the data has been processed, so the the screen is
  cleared and the image is displayed.
 */
STATIC
VOID
EndCallback(png_structp png_ptr,
            png_infop info_ptr)
{
  IMAGE_INFO *Info;

  Info = png_get_progressive_ptr(png_ptr);
  if(Info && Info->Image) {
    ClearScreen(Info);
    DisplayImage(Info);
  }
}

/**
  Display a splash screen.

  The splash screen is passed in PNG format.  Any valid PNG file should be
  supported.

  @param  Buffer      A pointer to a buffer containing a PNG file.
  @param  BufferSize  Size of the buffer containing the PNG file.

  @retval EFI_SUCCESS           The splash screen was drawn successfully.
  @retval EFI_UNSUPPORTED       The PNG image is not valid/supported.
  @retval EFI_NOT_FOUND         The Graphics Output Protocol/UGA Draw Protocol/
                                Console Control Protocol was not found.
  @retval EFI_OUT_OF_RESOURCES  There was insufficient memory to decode the
                                image.
  @retval EFI_DEVCE_ERROR       The display device had an error and could not
                                display the image.
 */
EFI_STATUS
EFIAPI
ShowSplashScreen(IN VOID  *Buffer,
                 IN UINTN  BufferSize)
{
  EFI_CONSOLE_CONTROL_SCREEN_MODE   CurrentMode;
  EFI_CONSOLE_CONTROL_PROTOCOL     *ConsoleControl;
  EFI_STATUS                        Status;
  IMAGE_INFO                        PngInfo;
  BOOLEAN                           GopUgaExists;
  png_structp                       png_ptr;
  png_infop                         info_ptr;

  Status = gBS->LocateProtocol(&gEfiConsoleControlProtocolGuid,
                               NULL,
                               (VOID**)&ConsoleControl);
  if(EFI_ERROR(Status))
     return Status;
  (VOID) ConsoleControl->GetMode(ConsoleControl,
                                 &CurrentMode,
                                 &GopUgaExists,
                                 NULL);
  if(!GopUgaExists)
    return EFI_NOT_FOUND;

  (VOID) SetMem(&PngInfo, sizeof(PngInfo), 0);
  Status = IdentifyGraphics(&PngInfo);
  if(EFI_ERROR(Status))
    return Status;

  /* Allocate structs needed by libpng */
  if(!(png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
                                        &PngInfo,              // error_ptr
                                        ErrorHandler,          // error_fn
                                        WarningHandler         // warn_fn
                                        )) ||
     !(info_ptr = png_create_info_struct(png_ptr))) {
    png_destroy_read_struct(&png_ptr, NULL, NULL);
    return EFI_OUT_OF_RESOURCES;
  }
  /* Configure error handling! */
  if(setjmp(png_jmpbuf(png_ptr))) {
    if(PngInfo.Image)
      gBS->FreePool(PngInfo.Image);
    (VOID) ConsoleControl->SetMode(ConsoleControl, CurrentMode);
    return EFI_UNSUPPORTED;
  }
  /* Enable graphical console */
  if(EfiConsoleControlScreenGraphics != CurrentMode)
    (VOID) ConsoleControl->SetMode(ConsoleControl,
                                   EfiConsoleControlScreenGraphics);
  /* Parse the image - most of the work is done in the three callbacks */
  png_set_progressive_read_fn(png_ptr,
                              &PngInfo,       // progressive_ptr
                              InfoCallback,   // info_fn
                              RowCallback,    // row_fn
                              EndCallback     // end_fn
                              );
  png_process_data(png_ptr,
                   info_ptr,
                   Buffer,
                   BufferSize);
  /* Deallocate structs used by libpng */
  png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
  /* Free image buffer */
  if(PngInfo.Image)
    gBS->FreePool(PngInfo.Image);
  
  return EFI_SUCCESS;
}

/**
  Hide the splash screen.

  Attempt to hide the splash screen.  Currently, this is only possible if the
  Console Control Protocol is supported.

  @retval EFI_SUCCESS          The splash screen was hidden successfully
  @retval EFI_NOT_FOUND        The Console Control protocol was not found
 */
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
