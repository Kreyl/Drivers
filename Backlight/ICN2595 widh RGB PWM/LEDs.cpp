#include "LEDs.h"
#include "board.h"
#include "kl_lib.h"
#include "color.h"
//#include "DispBuf.h"

#if 1 // ============================= Variables ===============================
// color remapping: "RGB" or "GBR"
#define ColorOrder_RGB

#define LEDS_MAX_BAUDRATE_HZ    20000000
#define LEDS_TIM_SHIFT      4
#define LEDS_TIM_PULSE_LEN  1

#define LEDS_DMA_MODE(Chnl) \
                        (STM32_DMA_CR_CHSEL(Chnl) \
                        | DMA_PRIORITY_MEDIUM \
                        | STM32_DMA_CR_MSIZE_HWORD \
                        | STM32_DMA_CR_MSIZE_HWORD \
                        | STM32_DMA_CR_MINC     /* Memory pointer increase */ \
                        | STM32_DMA_CR_DIR_M2P)  /* Direction is memory to peripheral */ \
                        | STM32_DMA_CR_TCIE

static volatile bool NewPicIsReady = false;
#endif


#if 1 // ========================= RawBuf ======================================
#define LED_BYTE_CNT        (LED_IC_CNT * 2UL)
#define LED_WORD_CNT        (LED_IC_CNT)

#define LED_RAWBUF_SZ      (LED_TIMESLOT_TOP * LED_IC_CNT * 2UL) // Size in bytes
#define LED_RAWBUF_CNT16   (LED_RAWBUF_SZ / 2UL) // Count of Word16

struct RawBuf_t {
    uint16_t Buf1[LED_RAWBUF_CNT16], Buf2[LED_RAWBUF_CNT16], *ReadBuf = Buf1;
    void Reset() {
        ReadBuf = Buf1;
        for(uint32_t i=0; i<LED_RAWBUF_CNT16; i++) {
            Buf1[i] = 0xFFFF;
            Buf2[i] = 0xFFFF;
        }
    }
    uint16_t* GetWriteBuf() { return (ReadBuf == Buf1)? Buf2 : Buf1; }
    void SwitchReadBuf() { ReadBuf = (ReadBuf == Buf1)? Buf2 : Buf1; }
} RawBuf;
#endif

// Counting on both fronts => 400*2 pulses for display,
#define LEDS_TIM_TOP        ((LED_IC_CNT * 16UL) * 2UL - 1UL)

namespace Leds {
#if 1 // ==== Dimmer ====
uint8_t CurrentBrt = 255;
PinOutputPWM_t DimmerPic {LED_DIMMER_PIN};

static inline void PicEnableOutput()    { DimmerPic.Set(CurrentBrt); }
static inline void PicDisableOutput()   { DimmerPic.Set(0);          }
#endif

static const Timer_t ITmr {LED_LATCH_TMR}; // Latch generator
static const Spi_t LedsSpi {LED_SPI};
static const stm32_dma_stream_t *LedsDma;

__attribute__((aligned))
uint8_t PicBrtBuf[LED_CHNL_CNT] = { 0 };

void DmaIrqHandler(void *p, uint32_t flags) {
    chSysLockFromISR();
//    DBG_HI();
//    PrintfI("i");
    dmaStreamDisable(LedsDma);
    if(NewPicIsReady) {
        RawBuf.SwitchReadBuf();
        NewPicIsReady = false;
//        DBG_LO();
    }
    // Prepare DMA
    dmaStreamSetMemory0(LedsDma, RawBuf.ReadBuf);
    dmaStreamSetTransactionSize(LedsDma, LED_RAWBUF_CNT16);
    dmaStreamSetMode(LedsDma, LEDS_DMA_MODE(LED_DMA_CHNL));
    // Wait transmission to complete (4 words in FIFO after last DMA transaction)
    while(LED_SPI->SR & SPI_SR_FTLVL);
    while(LED_SPI->SR & SPI_SR_BSY);
    ITmr.SetCounter(0); // Reset counter to return it back to sync, in case it is lost somehow
    dmaStreamEnable(LedsDma); // Start transmission
//    DBG_LO();
    chSysUnlockFromISR();
}

void Init() {
#if 1 // ==== SPI & DMA ====
    RawBuf.Reset();
    // Dimmer
    DimmerPic.Init();
    PicDisableOutput();
    // SPI
    PinSetupAlterFunc(LED_CLK, psMedium);
    PinSetupAlterFunc(LED_DATA, psHigh);
    LedsSpi.Setup(boMSB, cpolIdleLow, cphaFirstEdge, LEDS_MAX_BAUDRATE_HZ, bitn16);
    LedsSpi.PrintFreq();
    // DMA
    LedsDma = dmaStreamAlloc(LED_DMA, IRQ_PRIO_MEDIUM, DmaIrqHandler, nullptr);
    dmaStreamSetMemory0(LedsDma, RawBuf.ReadBuf);
    dmaStreamSetPeripheral(LedsDma, &LED_SPI->DR);
    dmaStreamSetTransactionSize(LedsDma, LED_RAWBUF_CNT16);
    dmaStreamSetMode(LedsDma, LEDS_DMA_MODE(LED_DMA_CHNL));
    dmaStreamEnable(LedsDma);
    LedsSpi.EnableTxDma();
#endif

#if 1 // ==== Latch generator ====
    ITmr.Init();
    ITmr.SetTopValue(LEDS_TIM_TOP);

#if LED_LATCH_TMR_IN == 1 // Input1 as clock input
    PinSetupAlterFunc(LED_LATCH_IN_PIN, psHigh);
    // CC1 channel is configured as input, IC1 is mapped on TI1
    ITmr.SetupInput1(0b01, Timer_t::pscDiv1, rfBoth);
    ITmr.SelectSlaveMode(smExternal); // External clock mode 1
    ITmr.SetTriggerInput(tiTI1FP1);
#elif LED_LATCH_TMR_IN == 2 // Input2 as clock input
    PinSetupAlterFunc(LED_LATCH_IN_PIN, psHigh);
    // CC2 channel is configured as input, IC2 is mapped on TI2
    ITmr.SetupInput2(0b01, Timer_t::pscDiv1, rfBoth);
    ITmr.SelectSlaveMode(smExternal); // External clock mode 1
    ITmr.SetTriggerInput(tiTI2FP2);
#else
#error "For clock need input 1 or 2"
#endif

#if LED_LATCH_TMR_OUT == 3 // Output3 as latch output
    PinSetupAlterFunc(LED_LATCH_OUT_PIN, psHigh);
    // Combined PWM mode: OC3REFC is the logical AND between OC4REF and OC3REF
    ITmr.SetupOutput3(0b1101); // mode 2
    ITmr.SetupOutput4(0b1100); // mode 1
    ITmr.EnableCCOutput3();
    ITmr.SetCCR3(LEDS_TIM_TOP - LEDS_TIM_SHIFT); // When set Hi
    ITmr.SetCCR4(LEDS_TIM_TOP - LEDS_TIM_SHIFT + LEDS_TIM_PULSE_LEN); // When set Lo
#elif LED_LATCH_TMR_OUT == 4 // Output4 as latch output
    PinSetupAlterFunc(LED_LATCH_OUT_PIN, psHigh);
    // Combined PWM mode 2: OC4REFC is the logical AND between OC4REF and OC3REF
    ITmr.SetupOutput3(0b1100); // mode 1
    ITmr.SetupOutput4(0b1101); // mode 2
    ITmr.EnableCCOutput4();
    ITmr.SetCCR4(LEDS_TIM_TOP - LEDS_TIM_SHIFT); // When set Hi
    ITmr.SetCCR3(LEDS_TIM_TOP - LEDS_TIM_SHIFT + LEDS_TIM_PULSE_LEN); // When set Lo
#else
#error "For latch need output 3 or 4"
#endif

    ITmr.Enable();
#endif
    PicEnableOutput();
    LedsSpi.Enable();
}

void SetRingBrightness(uint8_t Brt) {
    CurrentBrt = Brt;
    DimmerPic.Set(Brt);
}

void Stop() {
    PicDisableOutput();
    ITmr.Disable();
    dmaStreamDisable(LedsDma);
    LedsSpi.Disable();
//    LedsSpi.Reset();
    NewPicIsReady = false;
}

void Resume() {
    NewPicIsReady = false;
    ITmr.SetCounter(0);
    ITmr.Enable();
    RawBuf.Reset();
    dmaStreamSetMemory0(LedsDma, RawBuf.ReadBuf);
    dmaStreamSetTransactionSize(LedsDma, LED_RAWBUF_CNT16);
    dmaStreamSetMode(LedsDma, LEDS_DMA_MODE(LED_DMA_CHNL));
    dmaStreamEnable(LedsDma); // Start transmission
    LedsSpi.Enable();
    PicEnableOutput();
}

void ShowPic(Color_t *PClr) {
    NewPicIsReady = false;
    uint8_t *p = PicBrtBuf;
    for(uint32_t i=0; i<LED_CNT; i++) {
        uint32_t RealN = i; //Real2NLedTbl[i];
#ifdef ColorOrder_RGB
        *p++ = PClr[RealN].R;
        *p++ = PClr[RealN].G;
        *p++ = PClr[RealN].B;
#elif defined ColorOrder_GBR
        *p++ = PClr[RealN].G;
        *p++ = PClr[RealN].B;
        *p++ = PClr[RealN].R;
#endif
    }
    // Where to write to and read from
    uint16_t *pDst = RawBuf.GetWriteBuf() + (LED_IC_CNT - 1);
    uint16_t w;
    // Process other brightness values
    for(uint32_t brt=1; brt<=LED_TIMESLOT_TOP; brt++) {
        uint8_t *PSrc = PicBrtBuf;
        for(uint32_t N=0; N<LED_IC_CNT; N++) { // Iterate ICs
            w = 0;
            if(*PSrc++ >= brt) w |= (1<< 0);
            if(*PSrc++ >= brt) w |= (1<< 1);
            if(*PSrc++ >= brt) w |= (1<< 2);
            if(*PSrc++ >= brt) w |= (1<< 3);
            if(*PSrc++ >= brt) w |= (1<< 4);
            if(*PSrc++ >= brt) w |= (1<< 5);
            if(*PSrc++ >= brt) w |= (1<< 6);
            if(*PSrc++ >= brt) w |= (1<< 7);
            if(*PSrc++ >= brt) w |= (1<< 8);
            if(*PSrc++ >= brt) w |= (1<< 9);
            if(*PSrc++ >= brt) w |= (1<<10);
            if(*PSrc++ >= brt) w |= (1<<11);
            if(*PSrc++ >= brt) w |= (1<<12);
            if(*PSrc++ >= brt) w |= (1<<13);
            if(*PSrc++ >= brt) w |= (1<<14);
            if(*PSrc++ >= brt) w |= (1<<15);
            w ^= 0xFFFF;
            *pDst-- = w;
        } // for N
        pDst += (1 + LED_WORD_CNT + (LED_WORD_CNT - 1)); // Eliminate last --, move to next part, move to last element of next part
    } // brt
    // Say it is ready
    NewPicIsReady = true;
}

//void ShowBuf() {
//    NewPicIsReady = false;
////    DBG_HI();
//    // Where to write to and read from
//    uint16_t *pDst = DispBuf.GetWriteBuf() + 24;
//    uint16_t w;
//    // Process other brightness values
//    for(uint32_t brt=1; brt<=TIMESLOT_TOP; brt++) {
//        uint8_t *PSrc = PicBuf;
//        for(uint32_t N=0; N<25; N++) { // Iterate ICs
//            w = 0;
//            if(*PSrc++ >= brt) w |= (1<< 0);
//            if(*PSrc++ >= brt) w |= (1<< 1);
//            if(*PSrc++ >= brt) w |= (1<< 2);
//            if(*PSrc++ >= brt) w |= (1<< 3);
//            if(*PSrc++ >= brt) w |= (1<< 4);
//            if(*PSrc++ >= brt) w |= (1<< 5);
//            if(*PSrc++ >= brt) w |= (1<< 6);
//            if(*PSrc++ >= brt) w |= (1<< 7);
//            if(*PSrc++ >= brt) w |= (1<< 8);
//            if(*PSrc++ >= brt) w |= (1<< 9);
//            if(*PSrc++ >= brt) w |= (1<<10);
//            if(*PSrc++ >= brt) w |= (1<<11);
//            if(*PSrc++ >= brt) w |= (1<<12);
//            if(*PSrc++ >= brt) w |= (1<<13);
//            if(*PSrc++ >= brt) w |= (1<<14);
//            if(*PSrc++ >= brt) w |= (1<<15);
//            w ^= 0xFFFF;
//            *pDst-- = w;
//        } // for N
//        pDst += (1 + DISP_WORD_CNT + (DISP_WORD_CNT - 1)); // Eliminate last --, move to next part, move to last element of next part
//    } // brt
//    // Say it is ready
//    NewPicIsReady = true;
////    DBG_LO();
//}

#if 0 // ========================== Pic Buffer =================================
void ClearBuf() {
    uint64_t *p = (uint64_t*)PicBuf, *PicBufEnd = p + (DISP_PIC_SZ / 8);
    while(p < PicBufEnd) *p++ = 0;
}
void FillBuf(uint8_t Value) {
    uint64_t v = Value;
    v = v | (v << 8) | (v << 16) | (v << 24) | (v << 32) | (v << 40) | (v << 48) | (v << 56);
    uint64_t *p = (uint64_t*)PicBuf, *PicBufEnd = p + (DISP_PIC_SZ / 8);
    while(p < PicBufEnd) *p++ = v;
}

void PutPixelToBuf(int32_t x, int32_t y, uint8_t Brt) {
    PicBuf[ITable[x + y * COL_CNT]] = Brt;
}

void PutPixelToBuf(int32_t Indx, uint8_t Brt) {
    PicBuf[ITable[Indx]] = Brt;
}

void PutPicToBuf(uint8_t* Pic, uint32_t Sz) {
    if(Sz < DISP_PIC_SZ) ClearBuf(); // Clear buf if input is small
    if(Sz > DISP_PIC_SZ) Sz = DISP_PIC_SZ; // Truncate Sz if too large
    for(uint32_t i=0; i<Sz; i++) {
        PicBuf[ITable[i]] = Pic[i];
    }
}
#endif

} // Namespace
