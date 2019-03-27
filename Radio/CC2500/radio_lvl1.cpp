/*
 * radio_lvl1.cpp
 *
 *  Created on: Nov 17, 2013
 *      Author: kreyl
 */

#include "radio_lvl1.h"
#include "cc2500.h"
#include "MsgQ.h"
#include "shell.h"
#include "board.h"

cc2500_t CC(CC_Setup0);

//#define DBG_PINS

#ifdef DBG_PINS
#define DBG_GPIO1   GPIOB
#define DBG_PIN1    0
#define DBG1_SET()  PinSetHi(DBG_GPIO1, DBG_PIN1)
#define DBG1_CLR()  PinSetLo(DBG_GPIO1, DBG_PIN1)
//#define DBG_GPIO2   GPIOB
//#define DBG_PIN2    9
//#define DBG2_SET()  PinSet(DBG_GPIO2, DBG_PIN2)
//#define DBG2_CLR()  PinClear(DBG_GPIO2, DBG_PIN2)
#else
#define DBG1_SET()
#define DBG1_CLR()
#endif

rLevel1_t Radio;

#if 1 // ================================ Task =================================
static THD_WORKING_AREA(warLvl1Thread, 256);
__noreturn
static void rLvl1Thread(void *arg) {
    chRegSetThreadName("rLvl1");
    Radio.ITask();
}

__noreturn
void rLevel1_t::ITask() {
    while(true) {
        uint8_t RxRslt = CC.Receive(999, &Pkt, &Rssi);
        if(RxRslt == retvOk) {
            Printf("Rx Rssi=%d\r", Rssi);
            // Send message to main thd
//            EvtMsg_t Msg(evtIdRadioCmd, Pkt.Btn);
//            EvtQMain.SendNowOrExit(Msg);
            // Report reception
//            CC.Recalibrate();
//            CC.Transmit(&Pkt);
//            Printf("Tx\r");
        } // if RxRslt ok
//        else Printf("Rx\r");
//        chThdSleepMilliseconds(11);
    } // while
}
#endif // task

#if 1 // ============================
uint8_t rLevel1_t::Init() {
#ifdef DBG_PINS
    PinSetupOut(DBG_GPIO1, DBG_PIN1, omPushPull);
//    PinSetupOut(DBG_GPIO2, DBG_PIN2, omPushPull);
#endif    // Init radioIC

    if(CC.Init() == retvOk) {
        CC.SetTxPower(ccPwr0dBm);
        CC.SetPktSize(RPKT_LEN);
        CC.SetChannel(RCHNL);
        CC.Recalibrate();
        // Thread
        chThdCreateStatic(warLvl1Thread, sizeof(warLvl1Thread), HIGHPRIO, (tfunc_t)rLvl1Thread, NULL);
        return retvOk;
    }
    else return retvFail;
}
#endif
