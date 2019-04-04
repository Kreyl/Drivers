/*
 * pcm1865.h
 *
 *  Created on: 08 сент. 2015 г.
 *      Author: Kreyl
 */

#ifndef PCM1865_H_
#define PCM1865_H_

#include "kl_lib.h"
#include "board.h"
#include "main.h"

#define PCM_MAX_GAIN_DB     40
#define PCM_MIN_GAIN_DB     -12

#define PCM_DMA_RX_MODE     STM32_DMA_CR_CHSEL(PCM_DMA_CHNL) | \
                            DMA_PRIORITY_MEDIUM | \
                            STM32_DMA_CR_MSIZE_HWORD | \
                            STM32_DMA_CR_PSIZE_HWORD | \
                            STM32_DMA_CR_MINC |       /* Memory pointer increase */ \
                            STM32_DMA_CR_DIR_P2M |    /* Direction is peripheral to memory */ \
                            STM32_DMA_CR_TCIE    |    /* Enable Transmission Complete IRQ */ \
                            STM32_DMA_CR_CIRC         /* Circular buffer enable */

#define SAMPLE_SZ           2   // 16bit == 2bytes
#define PCM_RX_CH_CNT       8   // 4 meaningful + 4 dummy due to PCM1865 TDM mode realization

// Master clock generation
#define PCM_CLKTMR_TOP      7
#define PCM_CLKTMR_VALUE    3

enum PcmAdcChnls_t {pacADC1L = 0x01, pacADC1R = 0x02, pacADC2L = 0x03, pacADC2R = 0x04};
enum MicGroup_t {mg1, mg2};

class PCM1865_t {
private:
    Spi_t ICtrlSpi;
    PinOutput_t CS{PCM_CS_GPIO, PCM_CS_PIN, omPushPull};
    int16_t IRxBuf[PCM_RX_CH_CNT];
    void WriteReg(uint8_t Addr, uint8_t Value);
    uint8_t ReadReg(uint8_t Addr);
    // Commands
    void ResetRegs() { WriteReg(0x00, 0xFF); }
    void SelectPage(uint8_t Page) { WriteReg(0x00, Page); }
public:
    void Init();
    void SelectMicGrp(MicGroup_t Grp);
    void EnterRunMode() { WriteReg(0x70, 0x70); }
    void EnterPowerdownMode() { WriteReg(0x70, 0x74); }
    void SetGain(int8_t Gain_dB);
    int8_t GetGain(uint8_t Ch);
    // Debug
    void PrintState();
    void PrintClkRegs();
    // Inner use
    void IRQDmaRxHandler();
};

extern PCM1865_t Pcm;

#endif /* PCM1865_H_ */
