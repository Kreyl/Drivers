/*
 * ILI9341.cpp
 *
 *  Created on: 13 ??? 2016 ?.
 *      Author: Kreyl
 */

#include <Gui/ILI9488.h>
#include "board.h"
#include "uart.h"

ILI9488_t Lcd;
//#include "images.h"

#if 1 // ==== Pin driving functions ====
#define RstHi()  { PinSetHi(LCD_RESET); }
#define RstLo()  { PinSetLo(LCD_RESET); }
#define CsHi()   { PinSetHi(LCD_CS); }
#define CsLo()   { PinSetLo(LCD_CS); }
#define DcHi()   { PinSetHi(LCD_DC); }
#define DcLo()   { PinSetLo(LCD_DC); }
#define WrHi()   { PinSetHi(LCD_WR); }
#define WrLo()   { PinSetLo(LCD_WR); }
#define RdHi()   { PinSetHi(LCD_RD); }
#define RdLo()   { PinSetLo(LCD_RD); }
#define Write(Value) PortSetValue(LCD_DATA_GPIO, Value)
#endif

__always_inline
static inline void WriteData(uint16_t Data) {
    PortSetValue(LCD_DATA_GPIO, Data);
    WrLo();
    WrHi();
}

void ILI9488_t::Init() {
    // ==== GPIO ====
    PinSetupOut(LCD_RESET, omPushPull);
    PinSetupOut(LCD_CS,    omPushPull);
    PinSetupOut(LCD_DC,    omPushPull, psHigh);
    PinSetupOut(LCD_WR,    omPushPull, psHigh);
    PinSetupOut(LCD_RD,    omPushPull, psHigh);
    // Data port
    PortInit(LCD_DATA_GPIO, omPushPull, pudNone, psHigh);
    PortSetupOutput(LCD_DATA_GPIO);
    // ==== Init LCD ====
    // Initial signals
    RstHi();
    CsHi();
    RdHi();
    WrHi();
    chThdSleepMilliseconds(10);
    // Reset LCD
    RstLo();
    chThdSleepMilliseconds(50);
    RstHi();
    chThdSleepMilliseconds(120);
    CsLo(); // Stay selected forever
    chThdSleepMilliseconds(100);

    // Commands
    WriteCmd(0x11); // Sleep out
    chThdSleepMilliseconds(5);

    WriteCmd(0x29); // Display ON
    // Row order etc.
    WriteCmd(0x36);
    WriteData(0xAA);    //b8 MY, Row/Column exchange, BGR
    // Pixel format
    WriteCmd(0x3A);
    WriteData(0x55);    // 16 bit both RGB & MCU

    chThdSleepMilliseconds(4);

//    WriteCmd(0x51);
//    WriteData(0xAA);
//    chThdSleepMilliseconds(50);

    WriteCmd(0x54);
    PortSetupInput(LCD_DATA_GPIO);
//    for(uint8_t i=0; i<5; i++) {
//        RdLo();
//        RdHi();
//        uint16_t r = LCD_DATA_GPIO->IDR;
//        Printf("Lcd: %X\r", r);
//    }
    PortSetupOutput(LCD_DATA_GPIO);
}

void ILI9488_t::SwitchTo16BitsPerPixel(){
	WriteCmd(0x3A);
	WriteData(0x55);    // 16 bit both RGB & MCU
    chThdSleepMilliseconds(4);
}
void ILI9488_t::SwitchTo24BitsPerPixel(){
	WriteCmd(0x3A);
	WriteData(0x77);    // 24 bit both RGB & MCU
    chThdSleepMilliseconds(4);
}

void ILI9488_t::WriteCmd(uint8_t Cmd) {
    DcLo();
    PortSetValue(LCD_DATA_GPIO, Cmd);
    WrLo();
    WrHi();
    DcHi();
}

//uint16_t ILI9341_t::ReadData() {
//    PortSetupInput(LCD_DATA_GPIO);
//    RdLo();
//    uint16_t Rslt = PortGetValue(LCD_DATA_GPIO);
//    RdHi();
//    PortSetupOutput(LCD_DATA_GPIO);
//    return 0;//Rslt;
//}

void ILI9488_t::SetBounds(uint16_t Left, uint16_t Top, uint16_t Width, uint16_t Height) {
    uint16_t XEndAddr = Left + Width  - 1;
    uint16_t YEndAddr = Top  + Height - 1;
    // Write bounds
    WriteCmd(0x2A); // X
    WriteData(Left>>8);
    WriteData(Left);            // MSB will be ignored anyway
    WriteData(XEndAddr >> 8);
    WriteData(XEndAddr);
    WriteCmd(0x2B); // Y
    WriteData(Top >> 8);
    WriteData(Top);             // MSB will be ignored anyway
    WriteData(YEndAddr >> 8);
    WriteData(YEndAddr);
}

void ILI9488_t::FillWindow(uint32_t Left, uint32_t Top, uint32_t Width, uint32_t Height, uint16_t *Ptr) {
    SetBounds(Left, Top, Width, Height);
    uint32_t Cnt = Width * Height;
    PrepareToWriteGRAM();
//    uint32_t t = TIM5->CNT;
    while(Cnt--) WriteData(*Ptr++);
//    uint32_t delta = TIM5->CNT - t;
//    Uart.Printf("t=%u\r", delta);
}

void ILI9488_t::DrawRect(uint32_t Left, uint32_t Top, uint32_t Width, uint32_t Height, uint16_t Color565) {
    SetBounds(Left, Top, Width, Height);
    uint32_t Cnt = Width * Height;
    PrepareToWriteGRAM();
    while(Cnt--) WriteData(Color565);
    WriteCmd(0x00);
}

void ILI9488_t::DrawRect(uint32_t Left, uint32_t Top, uint32_t Width, uint32_t Height, Color_t Color) {
    SetBounds(Left, Top, Width, Height);
    uint32_t Cnt = Width * Height;
    uint16_t Clr565 = Color.RGBTo565();
    // Fill LCD
    PrepareToWriteGRAM();
    while(Cnt--) WriteData(Clr565);
    WriteCmd(0x00);
}

void ILI9488_t::DrawPoint (uint32_t x, uint32_t y, Color_t Color) {
    SetBounds(x, y, 1, 1);
    uint16_t Clr565 = Color.RGBTo565();
    PrepareToWriteGRAM();
    WriteData(Clr565);
}

void ILI9488_t::DrawLineHoriz(uint32_t x0, uint32_t y0, uint32_t Len, Color_t Color) {
    SetBounds(x0, y0, Len, 1);
    uint16_t Clr565 = Color.RGBTo565();
    PrepareToWriteGRAM();
    while(Len--) WriteData(Clr565);
}
void ILI9488_t::DrawLineVert (uint32_t x0, uint32_t y0, uint32_t Len, Color_t Color) {
    SetBounds(x0, y0, 1, Len);
    uint16_t Clr565 = Color.RGBTo565();
    PrepareToWriteGRAM();
    while(Len--) WriteData(Clr565);
}

//void ILI9488_t::DrawImage(uint16_t Left, uint16_t Top) {
//	uint16_t Width = (image[19] << 8) | image[18] + 1;
//	uint16_t Height = (image[23] << 8) | image[22];
//	SetBounds(Left, Top, Width, Height);
//	uint32_t Cnt = Width * Height;
//	uint16_t Position = image[10];
//
//	PrepareToWriteGRAM();
//	while (Cnt--) {
//		uint16_t data2Write = (image[Position+1] << 8) | image[Position];
//		WriteData(data2Write);
//		Position+=2;
//	}
//}


void ILI9488_t::DrawImage(uint16_t Left, uint16_t Top, uint16_t Width, uint16_t Height, uint8_t *image) {
	SetBounds(Left, Top, Width, Height);
	uint32_t Cnt = (Width * Height);

	uint16_t *ptr = (uint16_t*)image;

	PrepareToWriteGRAM();
	while (Cnt--) {
		WriteData(*ptr++);
	}
}
