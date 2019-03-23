/*
 * lis3dh.h
 *
 *  Created on: 4 февр. 2017 г.
 *      Author: Kreyl
 */

#pragma once

#include "kl_i2c.h"
#include "uart.h"

#define LIS_i2C             i2c1
#define LIS_I2C_ADDR        0x19

#define LIS_FIFO_LEVELS     10
#define LIS_IRQ_EN          TRUE

//#define LIS_DEBUG_PINS

#ifdef LIS_DEBUG_PINS
#define LIS_DBG_PIN         GPIOA, 3
#define DBG1_SET()          PinSetHi(LIS_DBG_PIN)
#define DBG1_CLR()          PinSetLo(LIS_DBG_PIN)
#else
#define DBG1_SET()
#define DBG1_CLR()
#endif

// Reg addresses (do not change)
#define LIS_RA_WHO_AM_I     0x0F
#define LIS_RA_CTRL_REG1    0x20
#define LIS_RA_CTRL_REG2    0x21
#define LIS_RA_CTRL_REG3    0x22
#define LIS_RA_CTRL_REG4    0x23
#define LIS_RA_CTRL_REG5    0x24
#define LIS_RA_CTRL_REG6    0x25
#define LIS_RA_FIFO_CTRL    0x2E
#define LIS_RA_OUT_X_L      0x28

// Reg values (do not change)
#define LIS_SCALE_2G        0x00
#define LIS_SCALE_4G        0x10
#define LIS_SCALE_8G        0x20
#define LIS_SCALE_16G       0x30

#define LIS_HIRES_EN        0x08


struct Accelerations_t {
    int16_t ax[3];
};

#if LIS_IRQ_EN
class Lis3d_t : public IrqHandler_t {
#else
class Lis3d_t {
#endif
private:
    uint8_t ReadReg(uint8_t RegAddr, uint8_t *Value) {
        return i2c1.WriteRead(LIS_I2C_ADDR, &RegAddr, 1, Value, 1);
    }
    uint8_t WriteReg(uint8_t RegAddr, uint8_t Value) {
        uint8_t arr[2];
        arr[0] = RegAddr;
        arr[1] = Value;
        return i2c1.Write(LIS_I2C_ADDR, arr, 2);
    }
public:
    Accelerations_t a[LIS_FIFO_LEVELS];
    bool IsOk = false;
    void Init() {
//        Uart.Printf("Lis3D Init\r");
#ifdef LIS_DEBUG_PINS
        PinSetupOut(LIS_DBG_PIN, omPushPull);
#endif
        IsOk = false;
        // Check if Lis connected
        uint8_t b = 0, rslt;
        rslt = ReadReg(LIS_RA_WHO_AM_I, &b);
        if(rslt != retvOk) {
            Uart.Printf("Lis3D ReadReg fail\r");
            return;
        }
        if(b != 0x33) {
            Uart.Printf("Lis3D fail: %02X\r", b);
            return;
        }
        // CFG1: Output data rate = 100Hz, normal mode, XYZ enable
//        if(WriteReg(LIS_RA_CTRL_REG1, 0b01010111) != retvOk) return;
        // CFG1: Output data rate = 200Hz, normal mode, XYZ enable
        if(WriteReg(LIS_RA_CTRL_REG1, 0b01100111) != retvOk) return;
        // CFG2: HPF normal mode, filter bypassed
        if(WriteReg(LIS_RA_CTRL_REG2, 0b10000000) != retvOk) return;
        // CFG3: Watermark interrupt on INT1
        if(WriteReg(LIS_RA_CTRL_REG3, 0b00000100) != retvOk) return;
        // CFG4: continuos update, LSB, FullScale=2g, HighRes, no selftest
        if(WriteReg(LIS_RA_CTRL_REG4, (LIS_SCALE_2G + LIS_HIRES_EN)) != retvOk) return;
        // CFG5: no reboot, FIFO en, no IRQ latch
        if(WriteReg(LIS_RA_CTRL_REG5, 0b01000000) != retvOk) return;
        // FIFO Ctrl: stream mode (replace older values), FIFO thr = 20
        if(WriteReg(LIS_RA_FIFO_CTRL, 0b10010100) != retvOk) return;
        IsOk = true;
    }
    void ReadSingle() {
        uint8_t RegAddr = LIS_RA_OUT_X_L | 0x80;
        if(i2c1.WriteRead(LIS_I2C_ADDR, &RegAddr, 1, (uint8_t*)a, 6) != retvOk) IsOk = false;
    }
    void ReadFIFO() {
        DBG1_SET();
        uint8_t RegAddr = LIS_RA_OUT_X_L | 0x80;
        if(i2c1.WriteRead(LIS_I2C_ADDR, &RegAddr, 1, (uint8_t*)a, (6 * LIS_FIFO_LEVELS)) != retvOk) IsOk = false;
        DBG1_CLR();
    }
#if LIS_IRQ_EN
    void IIrqHandler();
#endif
};

/*
 Put this in right place:
Lis3d_t Lis;
PinIrq_t LisIrq(ACC_INT_PIN, &Lis);
...
LisIrq.EnableIrq(IRQ_PRIO_MEDIUM);
*/
