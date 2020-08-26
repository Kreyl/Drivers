/*
 * TStating.h
 *
 *  Created on: 16 мая 2020 г.
 *      Author: layst
 */

#pragma once

#include <inttypes.h>

class tSns_t {
public:
    uint8_t i2cAddr;
    float t = 0;
    bool IsOk = true;
    const char* Name;
    tSns_t(uint8_t Addr, const char* AName) : i2cAddr(Addr), Name(AName) {}

    uint8_t GetT() {
        uint8_t PointerReg = 0; // read t reg
        uint8_t IBuf[2];
        if(ti2c.WriteRead(i2cAddr, &PointerReg, 1, IBuf, 2) == retvOk) {
            int16_t r = (((int16_t)IBuf[0]) << 8) | (int16_t)IBuf[1];
            r >>= 7;
            t = ((float)r) / 2.0;
            IsOk = true;
            return retvOk;
        }
        else {
            IsOk = false;
            return retvFail;
        }
    }
};

// Example:
//    tSns_t Sensors[3] = {
//            {0x48, "t1"},
//            {0x49, "t2"},
//            {0x4A, "t3"},
//    };
//
//. . .
//
//if(Sensors[i].GetT() == retvOk) {
//    Cnt++;
//    tAvg += Sensors[i].t;
//}


