/*
 * motorL6470.cpp
 *
 *  Created on: 25 июля 2015 г.
 *      Author: Kreyl
 */

#include "L6470.h"
#include "uart.h"

void L6470_t::Init() {
    // Aux pins
    PinSetupOut(M_AUX_GPIO, M_STBY_RST, omPushPull, pudNone);
    PinSetupOut(M_AUX_GPIO, M_SW1, omPushPull, pudNone);
    PinSet(M_AUX_GPIO, M_SW1);  // SW1 is not used
    PinSetupIn(M_AUX_GPIO, M_BUSY_SYNC1, pudPullUp);
    // SPI pins
    PinSetupOut      (M_SPI_GPIO, M_CS,   omPushPull, pudNone);
    PinSetupAlterFunc(M_SPI_GPIO, M_SCK,  omPushPull, pudNone, M_SPI_AF);
    PinSetupAlterFunc(M_SPI_GPIO, M_MISO, omPushPull, pudNone, M_SPI_AF);
    PinSetupAlterFunc(M_SPI_GPIO, M_MOSI, omPushPull, pudNone, M_SPI_AF);
    CsHi();
    // ==== SPI ==== MSB first, master, ClkLowIdle, FirstEdge, Baudrate=...
    ISpi.Setup(M_SPI, boMSB, cpolIdleLow, cphaFirstEdge, sbFdiv8);
    ISpi.Enable();
    // ==== Reset IC ====
    ResetOn();
    chThdSleepMilliseconds(7);
    ResetOff();
    chThdSleepMilliseconds(27);
    GetStatus();
}

#if 1 // ============================ Motion ===================================
void L6470_t::Run(Dir_t Dir, uint32_t Speed) {
    Convert::DWordBytes_t dwb;
    dwb.DWord = Speed;
    CsLo();
    ISpi.ReadWriteByte(0b01010000 | ((uint8_t)Dir));
    CSHiLo();
    ISpi.ReadWriteByte(dwb.b[2]);
    CSHiLo();
    ISpi.ReadWriteByte(dwb.b[1]);
    CSHiLo();
    ISpi.ReadWriteByte(dwb.b[0]);
    CsHi();
}

#endif


#if 1 // ======================= Params, Read/Write ============================
void L6470_t::Cmd(uint8_t ACmd) {
    CsLo();
    ISpi.ReadWriteByte(ACmd);
    CsHi();
}

void L6470_t::GetParam(uint8_t Addr, uint8_t *PParam1) {
    CsLo();
    ISpi.ReadWriteByte(0b00100000 | Addr);
    CSHiLo();
    *PParam1 = ISpi.ReadWriteByte(0);
    CsHi();
}

void L6470_t::SetParam8(uint8_t Addr, uint8_t Value) {
    CsLo();
    ISpi.ReadWriteByte(Addr);
    CSHiLo();
    ISpi.ReadWriteByte(Value);
    CsHi();
}
void L6470_t::SetParam16(uint8_t Addr, uint16_t Value) {
    Convert::WordBytes_t wb;
    wb.Word = Value;
    CsLo();
    ISpi.ReadWriteByte(Addr);
    CSHiLo();
    ISpi.ReadWriteByte(wb.b[1]);
    CSHiLo();
    ISpi.ReadWriteByte(wb.b[0]);
    CsHi();
}

void L6470_t::GetParam(uint8_t Addr, uint8_t *PParam1, uint8_t *PParam2) {
    CsLo();
    ISpi.ReadWriteByte(0b00100000 | Addr);
    CSHiLo();
    *PParam1 = ISpi.ReadWriteByte(0);
    CSHiLo();
    *PParam2 = ISpi.ReadWriteByte(0);
    CsHi();
}

uint16_t L6470_t::GetStatus() {
    uint16_t rslt;
    CsLo();
    ISpi.ReadWriteByte(0b11010000);
    CSHiLo();
    rslt = ISpi.ReadWriteByte(0);
    CSHiLo();
    rslt <<= 8;
    rslt |= ISpi.ReadWriteByte(0);
    CsHi();
    return rslt;
}
#endif
