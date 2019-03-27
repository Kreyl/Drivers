/*
 * radio_lvl1.h
 *
 *  Created on: Nov 17, 2013
 *      Author: kreyl
 */

#pragma once

#include "kl_lib.h"
#include "ch.h"
#include "cc2500.h"
#include "kl_buf.h"

#if 1 // =========================== Pkt_t =====================================
struct rPkt_t  {
    int16_t DtctrID;
    int16_t FrflyID;
} __packed;

#define RPKT_LEN    sizeof(rPkt_t)

#define TEST_WORD   0xCa115ea1  // Call Seal
#endif

// ==== Sizes ====

#if 1 // ======================= Channels & cycles =============================
#define RCHNL_SRV       0
#define ID2RCHNL(ID)    (RCHNL_MIN + ID)
#define RCHNL           7

#define ID_FRFLY_MIN    10
#define ID_FRFLY_MAX    40
#endif

#if 1 // =========================== Timings ===================================
#define RX_T_MS                 180      // pkt duration at 10k is around 12 ms
#define RX_SLEEP_T_MS           810
#define MIN_SLEEP_DURATION_MS   18
#define RETRY_CNT               4

#endif

class rLevel1_t {
private:
    void TryToSleep(uint32_t SleepDuration) {
//        if(SleepDuration >= MIN_SLEEP_DURATION_MS) CC.EnterPwrDown();
        chThdSleepMilliseconds(SleepDuration); // XXX
    }
public:
    int8_t Rssi;
    uint8_t Init();
    rPkt_t Pkt;
    // Inner use
    void ITask();
};

extern rLevel1_t Radio;
