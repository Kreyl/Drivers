/*
 * mcp3551.cpp
 *
 *  Created on: 16 θώνÿ 2016 γ.
 *      Author: Kreyl
 */

#include "mcp3551.h"
#include "main.h"

MCP3551_t AdcHeater(ADC_H_SETUP);
MCP3551_t AdcPCB   (ADC_P_SETUP);

void MCP3551_t::Init() {
    // GPIO
    PinSetupOut(ISetup.PGpio, ISetup.Cs, omPushPull);
    CsHi();
    PinIrq.Init(ttFalling);
    // SPI
    PinSetupAlterFunc(ISetup.PGpio, ISetup.Clk, omPushPull, pudNone, ISetup.AF);
    PinSetupAlterFunc(ISetup.PGpio, ISetup.Sdo, omPushPull, pudPullUp, ISetup.AF); // setup all registers
    // SPI
    ISetup.Spi.Setup(boMSB, cpolIdleLow, cphaFirstEdge, sbFdiv16, bitn8);
    ISetup.Spi.Enable();
}



#if 1 // ============================= IRQs ====================================
extern "C" {
// ==== ADC Heater ====
CH_IRQ_HANDLER(Vector9C) {  // EXTI Line[9:5] interrupts
    CH_IRQ_PROLOGUE();
    AdcHeater.DisableIRQ();
    AdcHeater.GetData();
    chSysLockFromISR();
    App.SignalEvtI(EVTMSK_ADC_HEATER_DONE);
    chSysUnlockFromISR();
    CH_IRQ_EPILOGUE();
}
// ==== ADC PCB ====
CH_IRQ_HANDLER(VectorE0) {  // EXTI Line[15:10] interrupts
    CH_IRQ_PROLOGUE();
    AdcPCB.DisableIRQ();
    AdcPCB.GetData();
    chSysLockFromISR();
    App.SignalEvtI(EVTMSK_ADC_PCB_DONE);
    chSysUnlockFromISR();
    CH_IRQ_EPILOGUE();
}
} // extern c
#endif


