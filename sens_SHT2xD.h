/*
 * sens_SHT2xD.h
 *
 *  Created on: 18 апр. 2017 г.
 *      Author: Elessar
 */

#pragma once

#include "kl_i2c.h"
#include "evt_mask.h"
#include "uart.h"

#define SHT_i2C             i2c1
#define SHT_I2C_ADDR        0x40 // Address specified in the Datasheet: 0x80
//#define HoldMasterMODE      // SCL pull GND on Measurement


// Commands
#define SHT_Meas_T_HM       0xE3 // Temperature measurement, hold master (SCL pull GND on Measurment)
#define SHT_Meas_RH_HM      0xE5 // Humidity measurement,  hold master (SCL pull GND on Measurment)
#define SHT_Meas_T_POLL     0xF3 // Temperature measurement, no hold master, currently not used
#define SHT_Meas_RH_POLL    0xF5 // Humidity measurement, no hold master, currently not used
#define SHT_UserReg_W       0xE6 // write User Register
#define SHT_UserReg_R       0xE7 // read User Register
#define SHT_SoftReset       0xFE // soft reset
#if defined HoldMasterMODE
#define SHT_MeasMode_T      SHT_Meas_T_HM
#define SHT_MeasMode_RH     SHT_Meas_RH_HM
#else
#define SHT_MeasMode_T      SHT_Meas_T_POLL
#define SHT_MeasMode_RH     SHT_Meas_RH_POLL
#endif

// Options (for User Register)
// sensor resolutions
typedef enum {
    RES_12_14BIT          = 0x00, // RH=12bit, T=14bit
    RES_8_12BIT           = 0x01, // RH= 8bit, T=12bit
    RES_10_13BIT          = 0x80, // RH=10bit, T=13bit
    RES_11_11BIT          = 0x81, // RH=11bit, T=11bit
} SHT_Resolution_t;
#define Resolution_MASK    ~0x81  // Mask for res. bits (7,0) in user reg.
// Heater control
typedef enum {
    Heater_ON             = 0x04, // heater on
    Heater_OFF            = 0x00, // heater off
} SHT_Heater_t;
#define Heater_MASK        ~0x04  // Mask for Heater bit(2) in user reg.


// Measurement signal selection
typedef enum {
    mtTemperature,
    mtHumidity
} MeasureType_t;

struct MeasData_t {
    uint8_t MSB, LSB, dCRC;
} __packed;
#define MeasData_SIZE     sizeof(MeasData_t)

#define MeasTimeOut  100       // Measurement Time-Out [mS]

struct ReadData_t {
    uint8_t Data, dCRC;
} __packed;
#define ReadData_SIZE     sizeof(ReadData_t)

// CRC
#define POLYNOMIAL      (uint16_t)0x131 // P(x)=x^8+x^5+x^4+1 = 100110001
#define CHECKSUM_ERROR  0x10


class SHT2xD_t {
private:
    uint8_t UserRegister;
    virtual_timer_t SHT_TMR;
    MeasureType_t MeasType;
    TmrKL_t TmrReadMeas { MS2ST(MeasTimeOut), EVT_SHT_READY, tktOneShot };

    uint8_t ReadReg(uint8_t RegAddr, uint8_t *Value) {
        uint8_t Result = retvOk;
        ReadData_t ReadData;
        Result |= i2c1.WriteRead(SHT_I2C_ADDR, &RegAddr, 1, (uint8_t*)&ReadData, ReadData_SIZE);
        Result |= CheckCRC(&ReadData.Data, 1, ReadData.dCRC);
        if (Result == retvOk)
            *Value = ReadData.Data;
        return Result;
    }
    uint8_t WriteReg(uint8_t RegAddr, uint8_t Value) {
        uint8_t arr[2];
        arr[0] = RegAddr;
        arr[1] = Value;
        return i2c1.Write(SHT_I2C_ADDR, arr, 2);
    }
    uint8_t WriteCommand(uint8_t Command) {
        return i2c1.Write(SHT_I2C_ADDR, &Command, 1);
    }
    uint8_t StartMeasurement(MeasureType_t AMeasType) {
        uint8_t Result;
        switch (AMeasType) {
            case mtTemperature:
                Result = WriteCommand(SHT_MeasMode_T);
                break;
            case mtHumidity:
                Result = WriteCommand(SHT_MeasMode_RH);
                break;
        }
        return Result;
    }
    uint8_t ReadMeasurement(int32_t *value) {
        MeasData_t MeasData;
        uint8_t Result = retvOk;
        Result |= i2c1.Read(SHT_I2C_ADDR, (uint8_t*)&MeasData, MeasData_SIZE);
        Result |= CheckCRC(&MeasData.MSB, 2, MeasData.dCRC);
//        Uart.Printf("Result %u\r", Result);
//        Uart.Printf("MSB %u  LSB %u\r", MeasData.MSB, MeasData.LSB);
        *value = (MeasData.MSB << 8) | (MeasData.LSB & ~0x03); // clear bits [1..0] (status bits)
        return Result;
    }
    uint8_t CheckCRC(uint8_t *Data, uint8_t NbrOfBytes, uint8_t CheckSum) {
        uint8_t crc = 0;
        // calculates 8-Bit checksum with given polynomial
        for (uint8_t byteCtr = 0; byteCtr < NbrOfBytes; byteCtr++) {
            crc ^= (Data[byteCtr]);
            for (uint8_t bit = 8; bit > 0; bit--) {
                if (crc & 0x80) crc = (crc << 1) ^ POLYNOMIAL;
                else crc = (crc << 1);
            }
        }
        if (crc != CheckSum) return CHECKSUM_ERROR; // Checksum error
        else return retvOk;
    }

public:
    uint8_t Init(thread_t *APThread, SHT_Resolution_t Resolutions) {
        uint8_t Result = retvOk;
//        i2c1.ScanBus();
        Result |= ReadReg(SHT_UserReg_R, &UserRegister);
        UserRegister = (UserRegister & Resolution_MASK) | Resolutions;
        Result |= WriteReg(SHT_UserReg_W, UserRegister);
        TmrReadMeas.Init(APThread);
        return Result;
    }

    uint8_t Start(MeasureType_t AMeasType) {
        uint8_t Result = StartMeasurement(AMeasType);
        if (Result == retvOk) {
            MeasType = AMeasType;
            TmrReadMeas.StartOrRestart();
        }
        return Result;
    }
    uint8_t Read(int32_t *PVaule) {
        int32_t Data;
        uint8_t Result = ReadMeasurement(&Data);
        if (Result == retvOk)
            switch (MeasType) {
                case mtTemperature:
                    *PVaule = 17572 * Data/65536 - 4685; // float: 175.72 * Data/65536.00 -46.85
                    break;
                case mtHumidity:
                    *PVaule = 12500 * Data/65536 - 600; // float: 125.0 * Data/65536.00 -6.0
                    break;
            }
        return Result;
    }
    uint8_t Start_AND_Read(MeasureType_t MeasType, int32_t *PVaule) {
        uint8_t Result = retvOk;
        Result |= StartMeasurement(MeasType);
        chThdSleepMilliseconds(MeasTimeOut);
        Result |= Read(PVaule);
        return Result;
    }

    uint8_t SetHeater(SHT_Heater_t AHeater) {
        UserRegister = (UserRegister & Heater_MASK) | AHeater;
        return WriteReg(SHT_UserReg_W, UserRegister);
    }

    uint8_t Reset() { return WriteCommand(SHT_SoftReset); }
};
