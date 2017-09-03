/*
 * disp_MAX7219.h
 *
 *  Created on: 02 апр. 2017 г.
 *      Author: Elessar
 */

#pragma once

#include "uart.h"
#include "kl_lib.h"
#include "board.h"

// Pins
#define MAX_GPIO            GPIOB
#define MAX_CS              2
#define MAX_SCK             3
#define MAX_MOSI            5

// SPI
#define MAX_SPI             SPI1
#if defined STM32F030
#define MAX_SPI_AF          AF0
#elif defined STM32L1XX
#define MAX_SPI_AF          AF5
#endif

// Decode Mode
//#define dmBCDcode
#define dmNoDecode
#if (defined dmBCDcode) and (defined dmNoDecode)
#error "MAX7219.h: Only one Decode mode"
#endif
#define BlinkInterval_ms    200
#define MaxSegments         8

// Registers
#define MAX_DecodeMode      0x09    // Data mask ~0b11111111
#define MAX_Intensity       0x0A    // Data mask ~0b00001111
#define MAX_ScanLimit       0x0B    // Data mask ~0b00000111
#define MAX_Shutdown        0x0C    // Data mask ~0b00000001
#define MAX_DisplayTest     0x0F    // Data mask ~0b00000001

// Commands
#define MAX_Test        0x01
#define MAX_ShutDown    0x00
#define MAX_StayAwake   0x01
#if defined dmBCDcode
#define MAX_DispMode    0xFF
#elif defined dmNoDecode
#define MAX_DispMode    0x00
#endif


typedef enum { Bright100 = 0x0F, Bright75 = 0x0B, Bright50 = 0x07, Bright25 = 0x03, Bright5 = 0x00 } Intensity_t;

#if defined dmBCDcode
enum SymCode_t {
    symMinus = 0x0A,    // -
    symPoint = 0x80,    // . ,
    symEmpty = 0x0F,    //
    sym_E = 0x0B,       // E
    sym_H = 0x0C,       // H
    sym_L = 0x0D,       // L
    sym_P = 0x0E,       // P
    sym_0 = 0x00,       // 0 O D
    sym_1 = 0x01,       // 1
    sym_2 = 0x02,       // 2
    sym_3 = 0x03,       // 3
    sym_4 = 0x04,       // 4
    sym_5 = 0x05,       // 5 S
    sym_6 = 0x06,       // 6
    sym_7 = 0x07,       // 7
    sym_8 = 0x08,       // 8 B
    sym_9 = 0x09,       // 9
};
#elif defined dmNoDecode
enum SymCode_t {
    symMinus = 0b00000001,   // -
    symPoint = 0b10000000,   // . ,
    symSpace = 0b00001000,   // _
    symEmpty = 0b00000000,   //
    sym_A = 0b01110111,   // A R
    sym_b = 0b00011111,   // b
    sym_C = 0b01001110,   // C
    sym_c = 0b00001101,   // с
    sym_d = 0b00111101,   // d
    sym_E = 0b01001111,   // E
    sym_F = 0b01000111,   // F
    sym_G = 0b01011110,   // G
    sym_H = 0b00110111,   // H
    sym_h = 0b00010111,   // h
    sym_I = 0b00000110,   // I
    sym_J = 0b00111000,   // J
    sym_L = 0b00001110,   // L
    sym_n = 0b00010101,   // n
    sym_o = 0b00011101,   // o
    sym_P = 0b01100111,   // P
    sym_r = 0b00000101,   // r
    sym_t = 0b00001111,   // t
    sym_U = 0b00111110,   // U
    sym_u = 0b00011100,   // u
    sym_0 = 0b01111110,   // 0 O D
    sym_1 = 0b00110000,   // 1
    sym_2 = 0b01101101,   // 2
    sym_3 = 0b01111001,   // 3
    sym_4 = 0b00110011,   // 4
    sym_5 = 0b01011011,   // 5 S
    sym_6 = 0b01011111,   // 6
    sym_7 = 0b01110000,   // 7
    sym_8 = 0b01111111,   // 8 B
    sym_9 = 0b01111011,   // 9
};
const uint8_t SymCodeNam[] = {
        0b01111110,   // 0
        0b00110000,   // 1
        0b01101101,   // 2
        0b01111001,   // 3
        0b00110011,   // 4
        0b01011011,   // 5
        0b01011111,   // 6
        0b01110000,   // 7
        0b01111111,   // 8
        0b01111011,   // 9
};
#endif

#define CsHi()  PinSetHi(MAX_GPIO, MAX_CS)
#define CsLo()  PinSetLo(MAX_GPIO, MAX_CS)

void BlinkTmrCallback(void *p);    // VirtualTimer callback


class Disp_MAX7219_t {
private:
    Spi_t ISpi;
    uint8_t SegCount = MaxSegments;
    uint8_t DispBuffer[MaxSegments] = {symEmpty};
    uint8_t SegBlinking = 1;
    virtual_timer_t BlinkTMR;
    void WriteRegister (uint8_t ARegAddr, uint8_t AData) {
        CsLo();                         // Start transmission
        ISpi.ReadWriteByte(ARegAddr);   // Transmit header byte
        ISpi.ReadWriteByte(AData);      // Write data
        CsHi();                         // End transmission
    }
    void WriteBuffer (uint8_t AIndex, uint8_t AData) {
        DispBuffer[AIndex-1] = AData;
    }
    void BlinkTmrStsrtI(){
        if(chVTIsArmedI(&BlinkTMR)) chVTResetI(&BlinkTMR);
        chVTSetI(&BlinkTMR, MS2ST(BlinkInterval_ms), BlinkTmrCallback, this);
    }
    void BlinkTaskI();
public:
    void Init(const uint8_t ASegCount, const Intensity_t Intensity) {
        // ==== GPIO ====
        PinSetupOut      (MAX_GPIO, MAX_CS,   omPushPull);
        PinSetupAlterFunc(MAX_GPIO, MAX_SCK,  omPushPull, pudNone, MAX_SPI_AF);
        PinSetupAlterFunc(MAX_GPIO, MAX_MOSI, omPushPull, pudNone, MAX_SPI_AF);
        // ==== SPI ====    MSB first, master, ClkLowIdle, FirstEdge, Baudrate=f/2
        ISpi.Setup(boMSB, cpolIdleLow, cphaFirstEdge, sclkDiv2);
        ISpi.Enable();
        // Proceed with init
        SegCount = ASegCount;
        WriteRegister(MAX_DecodeMode, MAX_DispMode);
        WriteRegister(MAX_Intensity, Intensity);
        WriteRegister(MAX_ScanLimit, SegCount-1);
        Clear();
        WriteRegister(MAX_Shutdown, MAX_StayAwake);
    }

    void Clear() {
        for (uint8_t Seg=1; Seg<=SegCount; Seg++) {
            WriteRegister(Seg, symEmpty);
            DispBuffer[Seg-1] = symEmpty;
        }
    }
    void Print(const char *Str, int32_t Ddta = INT32_MAX, uint8_t Decade = 0);
    void SetBlinking(const uint8_t ASegment) {
        chSysLock();
        SegBlinking = ASegment;
        if(chVTIsArmedI(&BlinkTMR)) {
            chVTResetI(&BlinkTMR);
            WriteRegister(SegBlinking, DispBuffer[SegBlinking-1]);
        }
        chVTSetI(&BlinkTMR, MS2ST(BlinkInterval_ms), BlinkTmrCallback, this);
        chSysUnlock();
    }
    void StopBlinking() {
        chVTReset(&BlinkTMR);
        WriteRegister(SegBlinking, DispBuffer[SegBlinking-1]);
    }
    void Test() { WriteRegister(MAX_DisplayTest, MAX_Test); }      // Display-test mode turns all LEDs ON

    void ON() { WriteRegister(MAX_Shutdown, MAX_StayAwake); }
    void OFF() { WriteRegister(MAX_Shutdown, MAX_ShutDown); }

    Disp_MAX7219_t(): ISpi(MAX_SPI) {}
    // Inner use
    void BlinkTmrCallbakHandler() {
        chSysLockFromISR();
        BlinkTmrStsrtI();
        BlinkTaskI();
        chSysUnlockFromISR();
    }
};




