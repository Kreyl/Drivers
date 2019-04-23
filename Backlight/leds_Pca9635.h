/*
 * File:   leds_pca.h
 * Author: Kreyl
 *
 * Created on May 31, 2011, 8:44 PM
 */

#pragma once

#include "color.h"
#include "kl_lib.h"
#include "ChunkTypes.h"
#include "MsgQ.h"

#define LED_I2C_ADDR    0x1B

class PCA9635_t {
private:
    void SetPWM(uint8_t AChannel, uint8_t AValue);
public:
    void Init();
    void OutputEnable () { PinSetLo(LED_OE_PIN); }
    void OutputDisable() { PinSetHi(LED_OE_PIN);   }
    void SetColor(uint8_t Indx, Color_t Clr);
};

struct PcaMsg_t {
    uint8_t Indx;
    Color_t Color;
    PcaMsg_t& operator = (const PcaMsg_t &Right) {
        Indx = Right.Indx;
        Color = Right.Color;
        return *this;
    }
    PcaMsg_t() : Indx(0), Color(clBlack) {}
    PcaMsg_t(uint8_t AIndx, Color_t AColor) : Indx(AIndx), Color(AColor) {}
} __attribute__((__packed__));

extern EvtMsgQ_t<PcaMsg_t, MAIN_EVT_Q_LEN> EvtQPca;


class LedPcaBlinker_t : public BaseSequencer_t<LedRGBChunk_t> {
private:
    uint8_t Indx;
    void ISwitchOff() { SetColorI(clBlack); }
    SequencerLoopTask_t ISetup() {
        SetColorI(IPCurrentChunk->Color);
        IPCurrentChunk++;   // Always increase
        return sltProceed;  // Always proceed
    }
public:
    LedPcaBlinker_t(uint8_t AIndx) : BaseSequencer_t(), Indx(AIndx) {}
    void Init() {
        chSysLock();
        SetColorI(clBlack);
        chSysUnlock();
    }
    void SetColorI(Color_t AColor) {
        EvtQPca.SendNowOrExitI(PcaMsg_t(Indx, AColor));
    }
};

extern PCA9635_t Pca9635;
