/*
 * lis3dh.h
 *
 *  Created on: 4 ����. 2017 �.
 *      Author: Kreyl
 */

#ifndef LIS3DH_H_
#define LIS3DH_H_

#include "kl_i2c.h"
#include "uart.h"
#include "board.h"

#ifndef LIS3D_I2C_ADDR
#define LIS3D_I2C_ADDR      0x19
#endif
#ifndef LIS3D_I2C
#define LIS3D_I2C           i2c1
#endif

#define LIS3D_FIFO_LEVELS   30
//#define LIS3D_IRQ_EN        TRUE

//#define LIS3D_DEBUG_PINS

#ifdef LIS3D_DEBUG_PINS
#define LIS3D_DBG_PIN         GPIOA, 3
#define DBG1_SET()          PinSetHi(LIS3D_DBG_PIN)
#define DBG1_CLR()          PinSetLo(LIS3D_DBG_PIN)
#else
#define DBG1_SET()
#define DBG1_CLR()
#endif

// Reg addresses (do not change)
#define LIS3D_RA_WHO_AM_I   0x0F
#define LIS3D_RA_CTRL_REG1  0x20
#define LIS3D_RA_CTRL_REG2  0x21
#define LIS3D_RA_CTRL_REG3  0x22
#define LIS3D_RA_CTRL_REG4  0x23
#define LIS3D_RA_CTRL_REG5  0x24
#define LIS3D_RA_CTRL_REG6  0x25
#define LIS3D_RA_REFERENCE  0x26
#define LIS3D_RA_STATUS     0x27
#define LIS3D_RA_FIFO_CTRL  0x2E
#define LIS3D_RA_OUT_X_L    0x28
#define LIS3D_INT1_CFG      0x30
#define LIS3D_INT1_SRC      0x31
#define LIS3D_INT1_THS      0x32
#define LIS3D_INT1_DURATION 0x33

// Reg values (do not change)
#define LIS3D_I_AM          0x33

#define LIS3D_SCALE_2G      0x00
#define LIS3D_SCALE_4G      0x10
#define LIS3D_SCALE_8G      0x20
#define LIS3D_SCALE_16G     0x30

#define LIS3D_DRATE_1Hz     0x10
#define LIS3D_DRATE_10Hz    0x20
#define LIS3D_DRATE_25Hz    0x30
#define LIS3D_DRATE_50Hz    0x40
#define LIS3D_DRATE_100Hz   0x50
#define LIS3D_DRATE_200Hz   0x60
#define LIS3D_DRATE_300Hz   0x70
#define LIS3D_DRATE_1600Hz  0x80    // Only low power mode
#define LIS3D_DRATE_5000Hz  0x90    // Normal mode (1.25 kHz) / low power mode (5 KHz)

#define LIS3D_HIRES_EN      0x08

#define LIS3D_LPOWER_EN     0x08

typedef struct {
    int16_t X, Y, Z;
} Accelerations_t;

#if LIS3D_IRQ_EN
class Lis3d_t : public IrqHandler_t {
#else
class Lis3d_t {
#endif
private:
    uint8_t ReadReg(uint8_t RegAddr, uint8_t *Value) {
        return LIS3D_I2C.WriteRead(LIS3D_I2C_ADDR, &RegAddr, 1, Value, 1);
    }
    uint8_t WriteReg(uint8_t RegAddr, uint8_t Value) {
        uint8_t arr[2];
        arr[0] = RegAddr;
        arr[1] = Value;
        return LIS3D_I2C.Write(LIS3D_I2C_ADDR, arr, 2);
    }
public:
    Accelerations_t a[LIS3D_FIFO_LEVELS];

    uint8_t Init() {
#ifdef LIS3D_DEBUG_PINS
        PinSetupOut(LIS3D_DBG_PIN, omPushPull);
#endif
        // Check if Lis connected
        uint8_t b = 0, rslt;
        rslt = ReadReg(LIS3D_RA_WHO_AM_I, &b);
        if(rslt != retvOk) {
            Printf("Lis3D ReadReg fail\r");
            return retvFail;
        }
        if(b != LIS3D_I_AM) {
            Printf("Lis3D fail: %02X\r", b);
            return retvFail;
        }
        // ==== Data rate ====
        // CFG1: Output data rate = 300Hz, normal mode, XYZ enable
        rslt |= WriteReg(LIS3D_RA_CTRL_REG1, LIS3D_DRATE_300Hz | 0b00000111);

        // CFG2: HPF normal mode, filter bypassed
        rslt |= WriteReg(LIS3D_RA_CTRL_REG2, 0b10000000);
        // CFG3 (irqs): DRDY irq on INT1
        rslt |= WriteReg(LIS3D_RA_CTRL_REG3, 0b00010000);
        // CFG4: Block data update (output registers not updated until MSB and LSB read), LSB, FullScale=8g, HighRes, no selftest
        rslt |= WriteReg(LIS3D_RA_CTRL_REG4, (0x80 | LIS3D_SCALE_8G | LIS3D_HIRES_EN));

        // ==== FIFO ====
        // CFG5: no reboot, FIFO dis, no IRQ latch
//        rslt |= WriteReg(LIS3D_RA_CTRL_REG5, 0);
        // CFG5: no reboot, FIFO en, no IRQ latch
        rslt |= WriteReg(LIS3D_RA_CTRL_REG5, 0b01000000);
        // FIFO CTRL REG: Stream mode (overwrite oldest), watermark = LIS3D_FIFO_LEVELS
        rslt |= WriteReg(LIS3D_RA_FIFO_CTRL, ((0b10 << 6) | LIS3D_FIFO_LEVELS));
        // CFG3 (irqs): WTM (watermark) irq on INT1
        rslt |= WriteReg(LIS3D_RA_CTRL_REG3, 1 << 2);

        // CFG6: Click and irqs dis
        rslt |= WriteReg(LIS3D_RA_CTRL_REG6, 0x0);
        // Get status and read data if available
        rslt |= ReadReg(LIS3D_RA_STATUS, &b);
        if(rslt != retvOk) return retvFail;
        if(b) rslt |= ReadFIFO();
        return rslt;
    }
    uint8_t ReadSingle() {
        uint8_t RegAddr = LIS3D_RA_OUT_X_L | 0x80;
        return LIS3D_I2C.WriteRead(LIS3D_I2C_ADDR, &RegAddr, 1, (uint8_t*)a, 6);
    }
    uint8_t ReadFIFO() {
        DBG1_SET();
        uint8_t RegAddr = LIS3D_RA_OUT_X_L | 0x80;
        uint8_t rslt = LIS3D_I2C.WriteRead(LIS3D_I2C_ADDR, &RegAddr, 1, (uint8_t*)a, (6 * LIS3D_FIFO_LEVELS));
        DBG1_CLR();
        return rslt;
    }
    uint8_t EnableWkupIntAndLowRwr(uint8_t AThreshold, uint8_t ADuration) {
        uint8_t rslt = retvOk, b = 0;
        // CFG1: Output data rate = 100Hz, low power mode, XYZ enable
        rslt |= WriteReg(LIS3D_RA_CTRL_REG1, LIS3D_DRATE_10Hz | LIS3D_LPOWER_EN | 0b00000111);
        rslt |= WriteReg(LIS3D_RA_CTRL_REG2, 0x0);
        // CFG3 (irqs): AOI1 irq on INT1
        rslt |= WriteReg(LIS3D_RA_CTRL_REG3, 0b01000000);
        rslt |= WriteReg(LIS3D_RA_CTRL_REG4, LIS3D_SCALE_2G);
        rslt |= WriteReg(LIS3D_RA_CTRL_REG5, 0x0);
        rslt |= WriteReg(LIS3D_RA_FIFO_CTRL, 0x0);
        // Set interrupt 1 threshold (if FullScale=2g theh +/-128 == 2g => 1 lbs = 2000/128 = 15.625 mG)
        rslt |= WriteReg(LIS3D_INT1_THS, AThreshold);
        // Set the minimum Duration of the Interrupt1 event to be recognized (depend on the FullScale chosen, see Table 18 of AN3308)
        rslt |= WriteReg(LIS3D_INT1_DURATION, ADuration);
        // Dummy read REFERENCE to force set reference acceleration/tilt value
        rslt |= ReadReg(LIS3D_RA_REFERENCE, &b);
        // INT1_CFG: 6D detection, +/-XYZ interupt enable
        rslt |= WriteReg(LIS3D_INT1_CFG, 0b01111111);
        return rslt;
    }
    uint8_t Standby() {
        uint8_t rslt = retvOk;
        // Disable interrupts
        rslt |= WriteReg(LIS3D_RA_CTRL_REG3, 0x0);
        // CFG1: Power-down mode
        rslt |= WriteReg(LIS3D_RA_CTRL_REG1, 0x0);
        return rslt;
    }
#if LIS3D_IRQ_EN
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

#if 0 // =========================== Vector ====================================
#include "math.h"
class Acc_t {
public:
    int16_t x[3] = { 0 };
    Acc_t() {}
    Acc_t(int16_t a0, int16_t a1, int16_t a2) { x[0] = a0; x[1] = a1; x[2] = a2; }
    void Print() { Printf("%d\t%d\t%d\r\n", x[0],x[1],x[2]); }

    Acc_t& operator = (const Acc_t& R) {
        x[0] = R.x[0];
        x[1] = R.x[1];
        x[2] = R.x[2];
        return *this;
    }

    int16_t operator[](const uint8_t Indx) { return x[Indx]; }

    friend Acc_t operator - (const Acc_t &L, const Acc_t &R) {
        return Acc_t(L.x[0] - R.x[0], L.x[1] - R.x[1], L.x[2] - R.x[2]);
    }

    bool AllLessThan(const int32_t R) {
        if(x[0] >= R) return false;
        if(x[1] >= R) return false;
        if(x[2] >= R) return false;
        return true;
    }

    bool AnyBiggerThan(const Acc_t &R) {
        if(x[0] > R.x[0]) return true;
        if(x[1] > R.x[1]) return true;
        if(x[2] > R.x[2]) return true;
        return false;
    }

    Acc_t Percent(int32_t Percent) {
        return Acc_t(((int32_t)x[0] * Percent) / 100L, ((int32_t)x[1] * Percent) / 100L, ((int32_t)x[2] * Percent) / 100L);
    }

    int32_t LengthPow2() { return x[0]*x[0] + x[1]*x[1] + x[2]*x[2]; }
} __attribute__((packed));

int32_t DiffLen(Acc_t &v1, Acc_t &v2) {
    int32_t dif, sum = 0;
    dif = v1[0] - v2[0];
    sum = dif * dif;
    dif = v1[1] - v2[1];
    sum += dif * dif;
    dif = v1[2] - v2[2];
    sum += dif * dif;
    return (int32_t)sqrtf(sum);
}

Acc_t Module(Acc_t Acc) {
    if(Acc.x[0] < 0) Acc.x[0] = - Acc.x[0];
    if(Acc.x[1] < 0) Acc.x[1] = - Acc.x[1];
    if(Acc.x[2] < 0) Acc.x[2] = - Acc.x[2];
    return Acc;
}
#endif

#endif /* LIS3DH_H_ */
