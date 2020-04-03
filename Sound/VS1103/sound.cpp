#include "sound.h"
#include <string.h>

#if 1 // ==== VS defs ====
// Mode flags
#define VS_SM_DIFF      0x0001
#define VS_SM_RESET     0x0004
#define VS_SM_PDOWN     0x0010
#define VS_SM_TESTS     0x0020
#define VS_ICONF_ADSDI  0x0040  // SCI midi, SDI ADPCM
#define VS_SM_SDINEW    0x0800  // native SPI modes; must have

// Mixer volume
#define VS_SMV_ACTIVE   0x8000  // Mixer is active
#define VS_SMV_GAIN2    (25 << 5)     // Gain2 = 0 db, others muted

// VS_REG_RECCTRL
#define VS_SARC_DREQ512     0x0100  // DREQ needs 512 byte space to turn on
#define VS_SARC_OUTOFADPCM  0x0080  // current ADPCM playback is canceled
#endif

Sound_t Sound;
EvtMsgQ_t<VsMsg_t, MAIN_EVT_Q_LEN> EvtQVs;

static const uint8_t SZero = 0;

// ================================= IRQ =======================================
// Dreq IRQ
void Sound_t::IIrqHandler() {
//    PrintfI("vsIrq\r");
    IDreq.DisableIrq();
    EvtQVs.SendNowOrExitI(VsMsg_t(VSMSG_DREQ_IRQ));
}

// DMA irq
extern "C"
void SIrqDmaHandler(void *p, uint32_t flags) {
    chSysLockFromISR();
    dmaStreamDisable(VS_DMA);
    EvtQVs.SendNowOrExitI(VsMsg_t(VSMSG_DMA_DONE));
    chSysUnlockFromISR();
}

// =========================== Implementation ==================================
static THD_WORKING_AREA(waSoundThread, 512);
__noreturn
static void SoundThread(void *arg) {
    chRegSetThreadName("Sound");
    Sound.ITask();
}

__noreturn
void Sound_t::ITask() {
    while(true) {
        VsMsg_t Msg = EvtQVs.Fetch(TIME_INFINITE);
        switch(Msg.ID) {
            case VSMSG_DREQ_IRQ:
                ISendNextData();
                break;

            case VSMSG_DMA_DONE:
                ISpi.WaitBsyHi2Lo();                // Wait SPI transaction end
                XCS_Hi();                           // }
                XDCS_Hi();                          // } Stop SPI
                // Send next data if VS is ready
                if(IDreq.IsHi()) ISendNextData();   // More data allowed, send it now
                else IDreq.EnableIrq(IRQ_PRIO_MEDIUM); // Enable dreq irq
                break;

            case VSMSG_READ_NEXT: {
                FRESULT rslt = FR_OK;
                bool Eof = f_eof(&IFile);
                // Read next if not EOF
                if(!Eof) {
                    if     (Buf1.DataSz == 0) { /*Uart.Printf(" r1");*/ rslt = Buf1.ReadFromFile(&IFile); }
                    else if(Buf2.DataSz == 0) { /*Uart.Printf(" r2");*/ rslt = Buf2.ReadFromFile(&IFile); }
                }
                if(rslt != FR_OK) Printf("sndReadErr=%u\r", rslt);
                if(rslt == FR_OK and !Eof) StartTransmissionIfNotBusy();
            } break;

            default: break;
        } // switch
    } // while true
}

void Sound_t::Init() {
    // ==== GPIO init ====
    PinSetupOut(VS_RST, omPushPull);
    PinSetupOut(VS_XCS, omPushPull);
    PinSetupOut(VS_XDCS, omPushPull);
    Rst_Lo();
    XCS_Hi();
    XDCS_Hi();
    chThdSleepMilliseconds(45);
    IDreq.Init(ttRising);
    PinSetupAlterFunc(VS_XCLK, omPushPull, pudNone, AF1); // AF1 is dummy
    PinSetupAlterFunc(VS_SO,   omPushPull, pudNone, AF1);
    PinSetupAlterFunc(VS_SI,   omPushPull, pudNone, AF1);

    // ==== SPI init ====
    uint32_t div;
    if(ISpi.PSpi == SPI1) div = Clk.APB2FreqHz / VS_MAX_SPI_BAUDRATE_HZ;
    else div = Clk.APB1FreqHz / VS_MAX_SPI_BAUDRATE_HZ;
    SpiClkDivider_t ClkDiv = sclkDiv2;
    if     (div >= 128) ClkDiv = sclkDiv256;
    else if(div >= 64) ClkDiv = sclkDiv128;
    else if(div >= 32) ClkDiv = sclkDiv64;
    else if(div >= 16) ClkDiv = sclkDiv32;
    else if(div >= 8)  ClkDiv = sclkDiv16;
    else if(div >= 4)  ClkDiv = sclkDiv8;
    else if(div >= 2)  ClkDiv = sclkDiv4;
    ISpi.Setup(boMSB, cpolIdleLow, cphaFirstEdge, ClkDiv);
    ISpi.Enable();
    ISpi.EnableTxDma();

    // ==== DMA ====
    // Here only unchanged parameters of the DMA are configured.
    dmaStreamAllocate     (VS_DMA, IRQ_PRIO_MEDIUM, SIrqDmaHandler, NULL);
    dmaStreamSetPeripheral(VS_DMA, &VS_SPI->DR);
    dmaStreamSetMode      (VS_DMA, VS_DMA_MODE);

    // ==== Variables ====
    State = sndStopped;
    IDmaIdle = true;
    PBuf = &Buf1;
    EvtQVs.Init();

    // ==== Init VS ====
    Rst_Hi();
    chThdSleepMilliseconds(7);
    WriteCmd(VS_REG_MODE, (VS_SM_DIFF | VS_ICONF_ADSDI | VS_SM_SDINEW));  // Normal mode
    WriteCmd(VS_REG_MIXERVOL, (VS_SMV_ACTIVE | VS_SMV_GAIN2));
    WriteCmd(VS_REG_RECCTRL, VS_SARC_DREQ512);
    SetVolume(234);
    chThdSleepMilliseconds(270);
//    AmplifierOn();

    // ==== Thread ====
    chThdCreateStatic(waSoundThread, sizeof(waSoundThread), NORMALPRIO, (tfunc_t)SoundThread, NULL);
}

void Sound_t::Play(const char* AFilename, uint32_t StartPosition) {
    if(State == sndPlaying) Stop();
    if(StartPosition & 1) StartPosition--;
    FRESULT rslt;
    // Open new file
    Printf("Play %S at %u\r", AFilename, StartPosition);
    rslt = f_open(&IFile, AFilename, FA_READ+FA_OPEN_EXISTING);
    if (rslt != FR_OK) {
        if (rslt == FR_NO_FILE) Printf("%S: not found\r", AFilename);
        else Printf("OpenFile error: %u\r", rslt);
        Stop();
        return;
    }
    // Check if zero file
    if (IFile.obj.objsize == 0) {
        f_close(&IFile);
        Printf("Empty file\r");
        Stop();
        return;
    }
    // Fast forward to start position if not zero
    if(StartPosition != 0) {
        if(StartPosition < IFile.obj.objsize) f_lseek(&IFile, StartPosition);
    }

    // Initially, fill both buffers
    if(Buf1.ReadFromFile(&IFile) != retvOk) {
        Printf("Error reading Buf1\r");
        Stop();
        return;
    }
    // Fill second buffer if needed
    if(Buf1.DataSz == VS_DATA_BUF_SZ) {
        if(Buf2.ReadFromFile(&IFile) != retvOk) {
            Printf("Error reading Buf2\r");
            Stop();
            return;
        }
    }

    WriteCmd(VS_REG_MODE, (VS_SM_DIFF | VS_ICONF_ADSDI | VS_SM_SDINEW));  // Normal mode
    WriteCmd(VS_REG_MIXERVOL, (VS_SMV_ACTIVE | VS_SMV_GAIN2));
    WriteCmd(VS_REG_RECCTRL, VS_SARC_DREQ512);
    AmplifierOn();

    PBuf = &Buf1;
    State = sndPlaying;
    StartTransmissionIfNotBusy();
}

void Sound_t::Shutdown() {
    Rst_Lo();           // enter shutdown mode
}

#define VS_TIMEOUT              8000000
void Sound_t::Stop() {
    dmaWaitCompletion(VS_DMA);
    dmaStreamDisable(VS_DMA);
    AmplifierOff();
    WriteCmd(VS_REG_RECCTRL, VS_SARC_OUTOFADPCM | VS_SARC_DREQ512);
    f_close(&IFile);

    volatile uint32_t Timeout = VS_TIMEOUT;
    IDreq.DisableIrq();
    while(!IDreq.IsHi()) {
        Timeout--;
        if(Timeout == 0) break;
    }
    ISpi.WaitBsyHi2Lo();
    XDCS_Lo();
    for (uint32_t i=0; i<11; i++) ISpi.ReadWriteByte(0);
    XDCS_Hi();

    State = sndStopped;
    IDmaIdle = true;
}

// ================================ Inner use ==================================
void Sound_t::ISendNextData() {
    dmaStreamDisable(VS_DMA);
    IDmaIdle = false;
    switch(State) {
        case sndPlaying: {
            // Switch buffer if required
            if(PBuf->DataSz == 0) {
                PBuf = (PBuf == &Buf1)? &Buf2 : &Buf1;      // Switch to next buf
                if(PBuf->DataSz == 0) { // Previous attempt to read the file failed
                    IDmaIdle = true;
                    Stop();
                    EvtQMain.SendNowOrExit(EvtMsg_t(evtIdSoundEnd));
                    break;
                }
                else EvtQVs.SendNowOrExit(VsMsg_t(VSMSG_READ_NEXT));
            }
            // Send next piece of data
            XDCS_Lo();  // Start data transmission
            uint32_t FLength = (PBuf->DataSz > 512)? 512 : PBuf->DataSz;
            dmaStreamSetMemory0(VS_DMA, PBuf->PData);
            dmaStreamSetTransactionSize(VS_DMA, FLength);
            dmaStreamSetMode(VS_DMA, VS_DMA_MODE | STM32_DMA_CR_MINC);  // Memory pointer increase
            dmaStreamEnable(VS_DMA);
//            if(PBuf == &Buf1) Printf("*"); else Printf("#");
            // Process pointers and lengths
            PBuf->DataSz -= FLength;
            PBuf->PData += FLength;
        } break;

        case sndStopped:
            if(!IDreq.IsHi()) IDreq.EnableIrq(IRQ_PRIO_MEDIUM);
            else IDmaIdle = true;
    } // switch
}

// ==== Commands ====
uint8_t Sound_t::ReadCmd(uint8_t AAddr, uint16_t* AData) {
    uint16_t IData;
    XCS_Lo();   // Start transmission
    ISpi.ReadWriteByte(VS_READ_OPCODE);  // Send operation code
    ISpi.ReadWriteByte(AAddr);           // Send addr
    *AData = ISpi.ReadWriteByte(0);      // Read upper byte
    *AData <<= 8;
    IData = ISpi.ReadWriteByte(0);       // Read lower byte
    *AData += IData;
    XCS_Hi();   // End transmission
    return retvOk;
}
uint8_t Sound_t::WriteCmd(uint8_t AAddr, uint16_t AData) {
    XCS_Lo();                       // Start transmission
    ISpi.ReadWriteByte(VS_WRITE_OPCODE); // Send operation code
    ISpi.ReadWriteByte(AAddr);           // Send addr
    ISpi.ReadWriteByte(AData >> 8);      // Send upper byte
    ISpi.ReadWriteByte(0x00FF & AData);  // Send lower byte
    XCS_Hi();                       // End transmission
    return retvOk;
}

// ==== Amplifier ====
void Sound_t::AmplifierOn() {
    // Setup GPIO0 as output
    WriteCmd(VS_REG_WRAMADDR, 0xC017);
    WriteCmd(VS_REG_WRAM,     0x0001);
    // Set output high
    WriteCmd(VS_REG_WRAMADDR, 0xC019);
    WriteCmd(VS_REG_WRAM,     0x0001);
}
void Sound_t::AmplifierOff() {
    // Setup GPIO0 as output
    WriteCmd(VS_REG_WRAMADDR, 0xC017);
    WriteCmd(VS_REG_WRAM,     0x0001);
    // Set output low
    WriteCmd(VS_REG_WRAMADDR, 0xC019);
    WriteCmd(VS_REG_WRAM,     0x0000);
}
