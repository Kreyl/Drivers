/*
 * RotaryDial.h
 *
 *  Created on: 07 сент. 2017 г.
 *      Author: Elessar, Eldalim
 */

#pragma once

#include "uart.h"
#include "board.h"
#include "kl_lib.h"

// === EXTI IRQ Handler ===
#if Dial_Namber_PIN == 0
#define DIAL_IRQ_HANDLER  Vector58
#elif Dial_Namber_PIN == 1
#define DIAL_IRQ_HANDLER  Vector5C
#elif Dial_Namber_PIN == 2
#define DIAL_IRQ_HANDLER  Vector60
#elif Dial_Namber_PIN == 3
#define DIAL_IRQ_HANDLER  Vector64
#elif Dial_Namber_PIN == 4
#define DIAL_IRQ_HANDLER  Vector68
#else
#error "RotaryDial.h: not selected IRQ pin"
#endif

#define HexadecimalOut          TRUE    // Example: number "03" = 0xA3
/* else Decimal Output */

#define IRQ_En_Delay_MS         20
#define Disk_Poll_Period_MS     100
#define EndNumber_TimeOut_MS    1500

typedef enum {deUnlockIRQ, deDiskPoll, deSendEndEvt} DialerEvt_t;

uint64_t ConvertTextToNumber(const char *Str);

// VirtualTimers callback
void LockTmrCallback(void *p);
void DiskTmrCallback(void *p);
void SendEvtTmrCallback(void *p);


class Dial_t {
private:
    uint8_t Numeral = 0;
    uint64_t Number = 0;
    virtual_timer_t LockTmr, DiskTmr, SendEvtTmr;
    thread_t *IPAppThd;
    eventmask_t EvtArm, EvtEnd;
    bool DiskIsArmed() {
        return PinIsLo(Dial_Disk_GPIO, Dial_Disk_PIN);
    }
    void CalculateNumber() {
#ifdef HexadecimalOut
        Number = Number << 4 | Numeral;
#else
        if (Numeral == 10) Numeral = 0;
        Number = Number*10 + Numeral;
#endif
        Numeral = 0;
    }

public:
    void Init();
    void SetupSeqEvents(eventmask_t AArmEvt, eventmask_t AEndEvt) {
        IPAppThd = chThdGetSelfX();
        EvtEnd = AEndEvt;
        EvtArm = AArmEvt;
    }
    uint64_t GetNumber() {
        uint64_t temp = Number;
//        Uart.Printf("Namber %u\r", Number);
        Number = 0;
        return temp;
    }
    void ResetNumber() {
        Numeral = 0;
        Number = 0;
        chVTReset(&SendEvtTmr);
    }
    // Inner use
    void IProcessSequenceI(DialerEvt_t DialerEvt);
    inline void IIrqPinHandler();
};

extern Dial_t Dialer;
