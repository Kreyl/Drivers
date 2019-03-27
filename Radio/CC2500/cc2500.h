/*
 * File:   cc2500.h
 * Author: Laurelindo
 *
 * Created on 11 Декабрь 2009 г., 2:08
 */

#pragma once

/*
 * Low-level operations are here.
 * Pkt level at top side, and SPI at bottom.
 */

#include <inttypes.h>
#include "kl_lib.h"
#include "cc2500defins.h"
#include "cc2500_rf_settings.h"

#define CC_BUSYWAIT_TIMEOUT     99000   // tics, not ms

class cc2500_t : public IrqHandler_t {
private:
    const Spi_t ISpi;
    const GPIO_TypeDef *PGpio;
    const uint16_t Sck, Miso, Mosi, Cs;
    const PinIrq_t IGdo0;
    uint8_t IState; // Inner CC state, returned as first byte
    uint8_t IPktSz;
    thread_reference_t ThdRef;
    // Pins
    uint8_t BusyWait() {
        for(uint32_t i=0; i<CC_BUSYWAIT_TIMEOUT; i++) {
            if(PinIsLo(PGpio, Miso)) return retvOk;
        }
        return retvFail;
    }
    void CsHi() { PinSetHi((GPIO_TypeDef*)PGpio, Cs); }
    void CsLo() { PinSetLo((GPIO_TypeDef*)PGpio, Cs); }
    // General
    void RfConfig(void);
    int8_t RSSI_dBm(uint8_t ARawRSSI);
    // Registers and buffers
    uint8_t WriteRegister (const uint8_t Addr, const uint8_t AData);
    uint8_t ReadRegister(const uint8_t Addr, uint8_t *PData);
    uint8_t WriteStrobe(uint8_t AStrobe);
    uint8_t WriteTX(uint8_t* Ptr, uint8_t Length);
    // Strobes
    uint8_t Reset()       { return WriteStrobe(CC_SRES); }
    uint8_t EnterTX()     { return WriteStrobe(CC_STX);  }
    uint8_t EnterRX()     { return WriteStrobe(CC_SRX);  }
    uint8_t FlushRxFIFO() { return WriteStrobe(CC_SFRX); }
public:
    uint8_t Init();
    void SetChannel(uint8_t AChannel);
    void SetTxPower(uint8_t APwr)  { WriteRegister(CC_PATABLE, APwr); }
    void SetPktSize(uint8_t ASize) { WriteRegister(CC_PKTLEN, ASize); IPktSz = ASize; }
    // State change
    void Transmit(void *Ptr);
    uint8_t Receive(uint32_t Timeout_ms, void *Ptr, int8_t *PRssi=nullptr);
    uint8_t EnterIdle()    { return WriteStrobe(CC_SIDLE); }
    uint8_t EnterPwrDown() { return WriteStrobe(CC_SPWD);  }
    uint8_t Recalibrate() {
        while(IState != CC_STB_IDLE) {
            if(EnterIdle() != retvOk) return retvFail;
        }
        if(WriteStrobe(CC_SCAL) != retvOk) return retvFail;
        return BusyWait();
    }
    uint8_t ReadFIFO(void *Ptr, int8_t *PRssi);

    void IIrqHandler() { chThdResumeI(&ThdRef, MSG_OK); }   // NotNull check perfprmed inside chThdResumeI
    cc2500_t(
            SPI_TypeDef *ASpi, GPIO_TypeDef *APGpio,
            uint16_t ASck, uint16_t AMiso, uint16_t AMosi, uint16_t ACs, uint16_t AGdo0):
        ISpi(ASpi), PGpio(APGpio),
        Sck(ASck), Miso(AMiso), Mosi(AMosi), Cs(ACs),
        IGdo0(APGpio, AGdo0, pudNone, this),
        IState(0), IPktSz(0), ThdRef(nullptr) {}
};
