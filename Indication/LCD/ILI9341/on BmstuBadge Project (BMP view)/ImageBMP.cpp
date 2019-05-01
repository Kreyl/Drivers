/*
 * ImageBMP.cpp
 *
 *  Created on: 26 но€бр€ 2017 г.
 *      Author: NS
 */

#include <ImageBMP.h>
#include "kl_fs_utils.h"
#include "kl_lib.h"
#include "shell.h"

//#define DBG_PINS

#ifdef DBG_PINS
#define DBG_GPIO1   GPIOB
#define DBG_PIN1    13
#define DBG1_SET()  PinSetHi(DBG_GPIO1, DBG_PIN1)
#define DBG1_CLR()  PinSetLo(DBG_GPIO1, DBG_PIN1)
#else
#define DBG1_SET()
#define DBG1_CLR()
#endif


struct BMPFileInfo_t {
    uint32_t FileSize;
    uint32_t DataBitsOffset;
    uint32_t ImageWidth;
    uint32_t ImageHeight;
    uint16_t BitsPerPixel;
};
static BMPFileInfo_t Info;


void ImageBMP_t::Init() {
	ImageReady = true;
}

static uint8_t IBuf[4800];

uint8_t ImageBMP_t::ShowImage(uint16_t Top, uint16_t Left, const char* AFileName) {
	if(!ImageReady) return retvFail;
	ImageReady = false;

	// Try to open file
    if(OpenBMP(AFileName) != retvOk) return retvFail;

    // Fill buffer
    char Chunk[2] = {0, 0};
    const uint8_t BytesPerPixel = Info.BitsPerPixel / 8;
    const uint32_t ImageSize = BytesPerPixel * Info.ImageWidth * Info.ImageHeight;
    uint32_t BufferSize = 4800;
    uint16_t TotalNumberOfPacks = ImageSize / BufferSize + 1;
    uint32_t HeightPerPack = Info.ImageHeight / TotalNumberOfPacks;
    TotalNumberOfPacks = Info.ImageHeight / HeightPerPack;



    uint16_t CurrentPack = 0;

    uint8_t *ImageBuffer = IBuf;

//    Printf("Info.ImageWidth = %d\n", Info.ImageWidth);

    if(f_lseek(&IFile, (Info.DataBitsOffset)) != FR_OK) goto end;
    if(TryRead(&IFile, Chunk, 2) != retvOk) goto end;

//    Printf("Info.BitsPerPixel = %d \n\r", Info.BitsPerPixel);
//    Printf("HeightPerPack = %d \n\r", HeightPerPack);
//    Printf("TotalNumberOfPacks = %d \n\r", TotalNumberOfPacks);
//    Printf("ImageSize = %d \n\r", ImageSize);

    while(CurrentPack < TotalNumberOfPacks){
    	Printf("CurrentPack = %d \n\r", CurrentPack);
        if(TryRead(&IFile, ImageBuffer, BytesPerPixel*HeightPerPack*(Info.ImageWidth - 1)) != retvOk);// goto end;
    	Printf("CurrentPack = %d \n\r", CurrentPack);
        Lcd.DrawImage((uint32_t)Left, (uint32_t)(Top + HeightPerPack * CurrentPack), (Info.ImageWidth - 1), HeightPerPack, ImageBuffer);
    	Printf("CurrentPack = %d \n\r", CurrentPack);
    	CurrentPack++;
    }


    f_close(&IFile);
    ImageReady = true;
    return retvOk;

    end:
    f_close(&IFile);
    Printf("error reading file \n\r");
    ImageReady = true;
    return retvFail;
}

uint8_t ImageBMP_t::OpenBMP(const char* AFileName) {
    // Open file
    if(TryOpenFileRead(AFileName, &IFile) != retvOk) return retvFail;

    uint32_t HeaderSize;

    // Check if starts with RIFF
    char ChunkID[2] = {0, 0};
    if(TryRead(&IFile, ChunkID, 2) != retvOk or (memcmp(ChunkID, "BM", 2) != 0)) goto end;
    // Get file size
    if(TryRead<uint32_t>(&IFile, &Info.FileSize) != retvOk) goto end;
    // Read dummy bytes
    if(TryRead(&IFile, ChunkID, 2) != retvOk ) goto end;
    if(TryRead(&IFile, ChunkID, 2) != retvOk ) goto end;
    // Get data offset
    if(TryRead<uint32_t>(&IFile, &Info.DataBitsOffset) != retvOk) goto end;
    // Get Header Size
    if(TryRead<uint32_t>(&IFile, &HeaderSize) != retvOk) goto end;
    // Get image width
    if(TryRead<uint32_t>(&IFile, &Info.ImageWidth) != retvOk) goto end;
    // Get image height
    if(TryRead<uint32_t>(&IFile, &Info.ImageHeight) != retvOk) goto end;
    // Read dummy bytes: image planes
    if(TryRead(&IFile, ChunkID, 2) != retvOk ) goto end;
    // Get bit count
    if(TryRead<uint16_t>(&IFile, &Info.BitsPerPixel) != retvOk) goto end;

    Info.ImageWidth++;

//    Printf("File Size: %X\r", Info.FileSize);
//    Printf("Offset: %X\r", Info.DataBitsOffset);
//    Printf("Pits/pixel: %X\r", Info.BitsPerPixel);
//    Printf("Height: %X\r", Info.ImageHeight);
//    Printf("Width: %X\r", Info.ImageWidth);
//    Printf("Header Size: %X\r", HeaderSize);


    // Go to data chunk
    // Current Point is 30. Data position is Info.DataBitsOffset
    if(f_lseek(&IFile, Info.DataBitsOffset) != FR_OK) goto end;

    return retvOk;

    end:
    f_close(&IFile);
    return retvFail;
}

