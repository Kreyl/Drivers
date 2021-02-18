#pragma once

#include <inttypes.h>
#include "color.h"

namespace Leds {

void Init();
void ShowPic(Color_t *PClr);
void SetRingBrightness(uint8_t Brt);

void Stop();
void Resume();

//void PutPixelToBuf(int32_t x, int32_t y, uint8_t Brt);
//void PutPixelToBuf(int32_t Indx, uint8_t Brt);
//void PutPicToBuf(uint8_t* Pic, uint32_t Sz);
//void ClearBuf();
//void FillBuf(uint8_t Value);

}
