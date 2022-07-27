Humidity and Temperature sensor IC
Supports SHT20D, SHT21D, SHT25D
I2C interface


Add to "kl_i2c" function:

uint8_t i2c_t::Read(uint32_t Addr, uint8_t *RPtr, uint32_t RLength) {
#if I2C_USE_SEMAPHORE
    if(chBSemWait(&BSemaphore) != MSG_OK) return FAILURE;
#endif
    uint8_t Rslt;
    msg_t r;
    I2C_TypeDef *pi2c = PParams->pi2c;  // To make things shorter
    if(RLength == 0 or RPtr == nullptr) { Rslt = retvCmdError; goto ReadEnd; }
    if(IBusyWait() != retvOk) {
        Rslt = retvBusy;
        Uart.Printf("i2cR Busy\r");
        goto ReadEnd;
    }
    IReset(); // Reset I2C
    if(RLength != 0 and RPtr != nullptr) {
        // Prepare RX DMA
        dmaStreamSetMode(PParams->PDmaRx, PParams->DmaModeRx);
        dmaStreamSetMemory0(PParams->PDmaRx, RPtr);
        dmaStreamSetTransactionSize(PParams->PDmaRx, RLength);
        ILen = RLength;
        IState = istWriteRead;
    }
    else IState = istWrite;  // Nothing to read
    pi2c->CR2 = (Addr << 1);
    // Enable IRQs: TX completed, error, NAck
    pi2c->CR1 |= (I2C_CR1_TCIE | I2C_CR1_ERRIE | I2C_CR1_NACKIE);
    pi2c->CR2 |= I2C_CR2_START;         // Start transmission
    // Wait completion
    chSysLock();
    r = chThdSuspendTimeoutS(&PThd, MS2ST(I2C_TIMEOUT_MS));
    chSysUnlock();
    // Disable IRQs
    pi2c->CR1 &= ~(I2C_CR1_TCIE | I2C_CR1_ERRIE | I2C_CR1_NACKIE);
    if(r == MSG_TIMEOUT) {
        pi2c->CR2 |= I2C_CR2_STOP;
        Rslt = retvTimeout;
    }
    else Rslt = (IState == istFailure)? retvFail : retvOk;
    ReadEnd:
#if I2C_USE_SEMAPHORE
    chBSemSignal(&BSemaphore);
#endif
    return Rslt;
}


Example:

SHT2xD_t Sens1;
...
Sens1.SetupSeqEndEvt(EVT_SHT_READY);
Sens1.Init(RES_12_14BIT);
...
Sens1.Start(mtTemperature);
...
if(Evt & EVT_SHT_READY) {
    int32_t Temperature = 0;
    Sens1.Read(&Temperature);
    Uart.Printf("Temperature: %i\r", Temperature); // t "2315" = 23,15 C
    }


