/*
 * pcm1865.cpp
 *
 *  Created on: 08 сент. 2015 г.
 *      Author: Kreyl
 */

#include "pcm1865.h"
#include "pcm1865_regs.h"
#include "uart.h"
#include "clocking.h"
#include "usb_audio.h"
#include "leds.h"

// ST I2S formats
#define I2SSTD_I2S      0x00
#define I2SSTD_MSB      SPI_I2SCFGR_I2SSTD_0
#define I2SSTD_LSB      SPI_I2SCFGR_I2SSTD_1
#define I2SSTD_PCM      (SPI_I2SCFGR_I2SSTD_0 | SPI_I2SCFGR_I2SSTD_1)

// Mic inputs
#define IN_L1           0x01
#define IN_L2           0x02
#define IN_L3           0x04
#define IN_L4           0x08
#define IN_R1           0x01
#define IN_R2           0x02
#define IN_R3           0x04
#define IN_R4           0x08

// ADC channels
#define REG_ADC1L       0x06
#define REG_ADC1R       0x07
#define REG_ADC2L       0x08
#define REG_ADC2R       0x09

PinOutputPWM_t<PCM_CLKTMR_TOP, invNotInverted, omPushPull> TmrClk {GPIOC, 6, TIM3, 1};

// Wrapper for DMA TX IRQ
extern "C" {
void PcmRxIrq(void *p, uint32_t flags) { Pcm.IRQDmaRxHandler(); }
}

// ==== TX DMA IRQ ====
void PCM1865_t::IRQDmaRxHandler() {
    PinSet(GPIOC, 13);
    App.ProcessValues(IRxBuf);
    PinClear(GPIOC, 13);
}

void PCM1865_t::Init() {
    // Variables
    // ==== Control SPI ====
    CS.Init();
    CS.SetHi();
    // SPI pins
    PinSetupAlterFunc(PCM_SPI_GPIO, PCM_SCK,  omPushPull, pudNone, PCM_SPI_AF);
    PinSetupAlterFunc(PCM_SPI_GPIO, PCM_MISO, omPushPull, pudNone, PCM_SPI_AF);
    PinSetupAlterFunc(PCM_SPI_GPIO, PCM_MOSI, omPushPull, pudNone, PCM_SPI_AF);
    // ==== Control SPI ==== MSB first, master, ClkLowIdle, FirstEdge, Baudrate=32/4=8MHz
    ICtrlSpi.Setup(PCM_CTRL_SPI, boMSB, cpolIdleLow, cphaSecondEdge, sbFdiv4);
    ICtrlSpi.Enable();

    EnterPowerdownMode();

    // ==== Audio data input ====
    // GPIO
    PinSetupAlterFunc(GPIOB, 12, omPushPull, pudPullUp, AF5);   // NSS
    PinSetupAlterFunc(GPIOB, 13, omPushPull, pudPullDown, AF5); // Clk
    PinSetupAlterFunc(GPIOB, 15, omPushPull, pudPullDown, AF5); // MOSI

    // SPI
    PCM_DATA_SPI_RccEnable();
    // Slave, 16 bit, RX only, HW NSS, MSB, Slave, CPOL=0 (Clk Idle Low),
    // CPHA=0 (First edge), baudrate=32/4=8MHz
    PCM_DATA_SPI->CR1 = SPI_CR1_DFF | SPI_CR1_RXONLY | ((uint16_t)sbFdiv2 << 3);
    PCM_DATA_SPI->CR2 = SPI_CR2_RXDMAEN;
    PCM_DATA_SPI->I2SCFGR = 0;  // Disable I2S
    PCM_DATA_SPI->CR1 |= SPI_CR1_SPE;

    // DMA
    dmaStreamAllocate     (PCM_DMA_STREAM, IRQ_PRIO_MEDIUM, PcmRxIrq, NULL);
    dmaStreamSetPeripheral(PCM_DMA_STREAM, &PCM_DATA_SPI->DR);
    dmaStreamSetMemory0   (PCM_DMA_STREAM, IRxBuf);
    dmaStreamSetTransactionSize(PCM_DMA_STREAM, 8);
    dmaStreamSetMode      (PCM_DMA_STREAM, PCM_DMA_RX_MODE);
    dmaStreamEnable       (PCM_DMA_STREAM);

    // ==== Master clock generator (8 MHz) ====
    TmrClk.Init();
    TmrClk.Set(PCM_CLKTMR_VALUE);

    chThdSleepMilliseconds(9);  // Let clocks to stabilize

    // ==== Setup PCM1865 regs ====
    ResetRegs();
    SelectPage(0);

#if 1 // === Clocks ===
    WriteReg(0x28, 0x00);   // PLL disabled
    WriteReg(0x20, 0b00111110); // Master, use PLL for BCK/LRCK, All use PLL, Clk detector disabled

    // PLL coeffs
    WriteReg(0x29, 0);      // PLL P = 1
    WriteReg(0x2A, 0);      // PLL R = 1
    WriteReg(0x2B, 8);      // PLL J-part }
    WriteReg(0x2D, 0x07);   // PLL D-MSB  } K = 8.192
    WriteReg(0x2C, 0x80);   // PLL D-LSB  }

    // DSP1, DSP2, ADC dividers
    WriteReg(0x21, 7);      // DSP1 Clock Divider = 8
    WriteReg(0x22, 15);     // DSP2 Clock Divider = 16
    WriteReg(0x23, 31);     // ADC Clock Divider = 32

    // BCK & LRCK dividers
    WriteReg(0x25, 7);      // PLL SCK Clock Divider = 8
    WriteReg(0x26, 1);      // Master Clock (SCK) Divider = 2
    WriteReg(0x27, 255);    // Master SCK Clock Divider = 256

    WriteReg(0x28, 0x01);   // PLL enabled, src is SCK
#endif // Clocks
    // Common settings
//    WriteReg(0x05, 0b00000110); // No Smooth, no Link, no ClippDet, def attenuation, no AGC
    WriteReg(0x05, 0b01000110); // No Smooth, Gain Link Enabled, no ClippDet, def attenuation, no AGC
//    WriteReg(0x05, 0b11000111);
    // ADC Channel selection
    SelectMicGrp(mg1);
    WriteReg(0x0A, 0x00);   // Secondary ADC not connected

    // == Data Format ==
    // RX_WLEN=16bit (not used), LRCK duty=50%, TX_WLEN=16bit, FMT=TDM
    WriteReg(0x0B, 0b11001111);
    WriteReg(0x0C, 0x01);   // TDM mode: 4 ch
//    WriteReg(0x0C, 0x00);   // TDM mode: 2 ch
    WriteReg(0x0D, 128);   // TX TDM Offset: 128 to move data to falling edge

    EnterRunMode();
    chThdSleepMilliseconds(450);
    PrintState();
}

// ADC Channel selection (everywhere signal is not inverted)
void PCM1865_t::SelectMicGrp(MicGroup_t Grp) {
    if(Grp == mg1) {    // 1, 4, 6, 7 == L1, R2, R3, L4 => L1,R2,L4,R3 => 1,4,7,6
        WriteReg(REG_ADC1L, IN_L1);
        WriteReg(REG_ADC1R, IN_R2);
        WriteReg(REG_ADC2L, IN_L4);
        WriteReg(REG_ADC2R, IN_R3);
    }
    else {  // 2, 3, 5, 8 == R1, L2, L3, R4 => L2,R1,L3,R4
        WriteReg(REG_ADC1L, IN_L2);
        WriteReg(REG_ADC1R, IN_R1);
        WriteReg(REG_ADC2L, IN_L3);
        WriteReg(REG_ADC2R, IN_R4);
    }
}

void PCM1865_t::WriteReg(uint8_t Addr, uint8_t Value) {
//    Uart.Printf("\rW: %02X %02X", Addr, Value);
    CS.SetLo();
    Addr = (Addr << 1) | 0x00;  // Write operation
    ICtrlSpi.ReadWriteByte(Addr);
    ICtrlSpi.ReadWriteByte(Value);
    CS.SetHi();
}

uint8_t PCM1865_t::ReadReg(uint8_t Addr) {
    CS.SetLo();
    Addr = (Addr << 1) | 0x01;  // Read operation
    ICtrlSpi.ReadWriteByte(Addr);
    uint8_t r = ICtrlSpi.ReadWriteByte(0);
    CS.SetHi();
    return r;
}

#if 1 // ============================== Commands ===============================
void PCM1865_t::PrintState() {
    Uart.Printf("\rPCM State");
    uint8_t b = ReadReg(0x72);
    switch(b & 0x0F) {
        case 0x00: Uart.Printf("\r  PwrDown"); break;
        case 0x01: Uart.Printf("\r  Wait Clk Stable"); break;
        case 0x02: Uart.Printf("\r  Release reset"); break;
        case 0x03: Uart.Printf("\r  Stand-by"); break;
        case 0x04: Uart.Printf("\r  Fade IN"); break;
        case 0x05: Uart.Printf("\r  Fade OUT"); break;
        case 0x09: Uart.Printf("\r  Sleep"); break;
        case 0x0F: Uart.Printf("\r  Run"); break;
        default: Uart.Printf("\r  ! Reserved"); break;
    }
    // Ratio
    b = ReadReg(0x73);
    switch(b & 0x07) {
        case 0x00: Uart.Printf("\r  Fs: Out of range (Low) or LRCK Halt"); break;
        case 0x01: Uart.Printf("\r  Fs: 8kHz"); break;
        case 0x02: Uart.Printf("\r  Fs: 16kHz"); break;
        case 0x03: Uart.Printf("\r  Fs: 32-48kHz"); break;
        case 0x04: Uart.Printf("\r  Fs: 88.2-96kHz"); break;
        case 0x05: Uart.Printf("\r  Fs: 176.4-192kHz"); break;
        case 0x06: Uart.Printf("\r  Fs: Out of range (High)"); break;
        case 0x07: Uart.Printf("\r  Invalid Fs"); break;
        default: break;
    }
    b = ReadReg(0x74);
    switch(b & 0x70) {
        case 0x00: Uart.Printf("\r  BCK Ratio: Out of range (L) or BCK Halt"); break;
        case 0x10: Uart.Printf("\r  BCK Ratio: 32"); break;
        case 0x20: Uart.Printf("\r  BCK Ratio: 48"); break;
        case 0x30: Uart.Printf("\r  BCK Ratio: 64"); break;
        case 0x40: Uart.Printf("\r  BCK Ratio: 256"); break;
        case 0x60: Uart.Printf("\r  BCK Ratio: Out of range (H)"); break;
        case 0x70: Uart.Printf("\r  Invalid BCK ratio or LRCK Halt"); break;
        default: break;
    }
    switch(b & 0x07) {
        case 0x00: Uart.Printf("\r  SCK Ratio: Out of range (L) or SCK Halt"); break;
        case 0x01: Uart.Printf("\r  SCK Ratio: 128"); break;
        case 0x02: Uart.Printf("\r  SCK Ratio: 256"); break;
        case 0x03: Uart.Printf("\r  SCK Ratio: 384"); break;
        case 0x04: Uart.Printf("\r  SCK Ratio: 512"); break;
        case 0x05: Uart.Printf("\r  SCK Ratio: 768"); break;
        case 0x06: Uart.Printf("\r  SCK Ratio: Out of range (H)"); break;
        case 0x07: Uart.Printf("\r  Invalid SCK ratio or LRCK Halt"); break;
        default: break;
    }
    // Clock
    b = ReadReg(0x75);
    if(b & 0x40) Uart.Printf("\r  LRCK Halt");
    else if(b & 0x04) Uart.Printf("\r  LRCK Error");
    else Uart.Printf("\r  LRCK Ok");
    if(b & 0x20) Uart.Printf("\r  BCK  Halt");
    else if(b & 0x02) Uart.Printf("\r  BCK  Error");
    else Uart.Printf("\r  BCK  Ok");
    if(b & 0x10) Uart.Printf("\r  SCK  Halt");
    else if(b & 0x01) Uart.Printf("\r  SCK  Error");
    else Uart.Printf("\r  SCK  Ok");
    // Power
    b = ReadReg(0x78);
    if(b & 0x04) Uart.Printf("\r  DVDD Ok");
    else Uart.Printf("\r  DVDD Bad/Missing");
    if(b & 0x02) Uart.Printf("\r  AVDD Ok");
    else Uart.Printf("\r  AVDD Bad/Missing");
    if(b & 0x01) Uart.Printf("\r  LDO  Ok");
    else Uart.Printf("\r  LDO  Failure");
}

void PCM1865_t::PrintClkRegs() {
    uint8_t b;
    b = ReadReg(0x20);
    Uart.Printf("\r 0x20: %X", b);
    b = ReadReg(0x21);
    Uart.Printf("\r 0x21: %u", b);
    b = ReadReg(0x22);
    Uart.Printf("\r 0x22: %u", b);
    b = ReadReg(0x23);
    Uart.Printf("\r 0x23: %u", b);
    b = ReadReg(0x25);
    Uart.Printf("\r 0x25: %u", b);
    b = ReadReg(0x26);
    Uart.Printf("\r 0x26: %u", b);
    b = ReadReg(0x27);
    Uart.Printf("\r 0x27: %u", b);
    b = ReadReg(0x28);
    Uart.Printf("\r 0x28: %X", b);
    b = ReadReg(0x29);
    Uart.Printf("\r 0x29: %u", b);
    b = ReadReg(0x2A);
    Uart.Printf("\r 0x2A: %u", b);
    b = ReadReg(0x2B);
    Uart.Printf("\r 0x2B: %u", b);
    b = ReadReg(0x2C);
    Uart.Printf("\r 0x2C: %u", b);
    b = ReadReg(0x2D);
    Uart.Printf("\r 0x2D: %u", b);
}

void PCM1865_t::SetGain(int8_t Gain_dB) {
    if(Gain_dB < -12 or Gain_dB > 40) return;
    Gain_dB *= 2;
    WriteReg(0x01, Gain_dB);
}

int8_t PCM1865_t::GetGain(uint8_t Ch) {
    int8_t g = ReadReg(Ch);
    return g / 2;
}


#endif
