/*
 * ImageBMP.h
 *
 *  Created on: 26 но€бр€ 2017 г.
 *      Author: NS
 */

#pragma once

#include "kl_lib.h"
#include "ff.h"
#include "ILI9488.h"

#define FRAME_BUF_SZ        4096

class ImageBMP_t {
private:
    FIL IFile;
//    uint32_t Buf1[(FRAME_BUF_SZ/4)], Buf2[(FRAME_BUF_SZ/4)], *PCurBuf, BufSz;
    uint8_t OpenBMP(const char* AFileName);

public:
    bool ImageReady;
    void Init();

    uint8_t ShowImage(uint16_t Top, uint16_t Left, const char* AFileName);
    // Inner use
    void ITask();
    void IHandleIrq();
};

extern ImageBMP_t Image;
