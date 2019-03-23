/*
 * mcp3551.h
 *
 *  Created on: 16 ??? 2016 ?.
 *      Author: Kreyl
 */

#pragma once

#include "kl_lib.h"

struct MCP3551Setup_t {
    GPIO_TypeDef *PGpio;
    uint32_t Cs, Clk, Sdo;
    AlterFunc_t AF;
    Spi_t Spi;
};

// 5 MHz top SPI freq. t conv = 73ms

class MCP3551_t {
private:
    const MCP3551Setup_t ISetup;
    const PinIrq_t PinIrq;
    void CsLo() const { PinClear(ISetup.PGpio, ISetup.Cs); }
    void CsHi() const { PinSet  (ISetup.PGpio, ISetup.Cs); }
public:
    uint32_t LastData;
    void Init();

    void StartMeasurement() const {
        // Prepare IRQ
        PinIrq.CleanIrqFlag();
        PinIrq.EnableIrq(IRQ_PRIO_LOW);
        // Select chip to start measurement. SDO will fall to low when ready.
        CsLo();
    }
    void DisableIRQ() const { PinIrq.DisableIrq(); }

    // Call when ready
    void GetData() {
        Convert::DWordBytes_t dwb;
        dwb.DWord = 0;
        ISetup.Spi.WriteRead3Bytes(&dwb.b[1]);
//        Uart.Printf("%A\r", dwb.b, 4, ' ');
        dwb.DWord = __REV(dwb.DWord);
        CsHi();
        LastData = dwb.DWord;
    }

    MCP3551_t(MCP3551Setup_t ASetup) :
        ISetup(ASetup),
        PinIrq(ASetup.PGpio, ASetup.Sdo, pudPullUp),
        LastData(0) {}
};
