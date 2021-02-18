/*
 * board.h
 *
 *  Created on: 05 08 2018
 *      Author: Kreyl
 */

#pragma once

// ==== General ====
#define APP_NAME            "Ehtele Top"

// MCU type as defined in the ST header.
#define STM32L476xx

// Freq of external crystal if any. Leave it here even if not used.
#define CRYSTAL_FREQ_HZ         12000000

// OS timer settings
#define STM32_ST_IRQ_PRIORITY   2
#define STM32_ST_USE_TIMER      5
#define SYS_TIM_CLK             (Clk.APB1FreqHz)    // Timer 5 is clocked by APB1

//  Periphery
#define I2C1_ENABLED            TRUE
#define I2C2_ENABLED            FALSE
#define I2C3_ENABLED            FALSE
#define SIMPLESENSORS_ENABLED   TRUE
#define BUTTONS_ENABLED         TRUE

#define ADC_REQUIRED            TRUE
#define STM32_DMA_REQUIRED      TRUE    // Leave this macro name for OS

// LEDs config: BandCnt; {LedCnt1, LedCnt2, LedCnt3...}
#define BAND_CNT        1
#define LEDs_CNT        32
#define BAND_SETUPS     ((const BandSetup_t[]){\
    {LEDs_CNT, dirForward}, \
    })

#if 1 // ========================== GPIO =======================================
// EXTI
#define INDIVIDUAL_EXTI_IRQ_REQUIRED    FALSE


// LEDs PWM with ICN2595, Counts
#define LED_CNT         3UL // Max
#define LED_CHNL_CNT    (LED_CNT * 3UL)
#define LED_IC_CNT      1UL
// LEDs PWM with ICN2595, Pins
#define LED_DIMMER_PIN      { GPIOA, 6, TIM16, 1, invInverted, omPushPull, 254 }
#define LED_TIMESLOT_TOP    255UL
#define LED_LATCH_TMR       TIM2
#define LED_LATCH_IN_PIN    GPIOA, 15, omPushPull, pudPullDown, AF1 // T2C1/T2ETR (input)
#define LED_LATCH_TMR_IN    1
#define LED_LATCH_OUT_PIN   GPIOB, 11, omPushPull, pudNone, AF1      // T2C4 (ouput)
#define LED_LATCH_TMR_OUT   4
#define LED_SPI             SPI3
#define LED_DATA            GPIOC, 12, omPushPull, pudNone, AF6     // MOSI3
#define LED_CLK             GPIOC, 10, omPushPull, pudNone, AF6     // SCK3

// DBG UART
#define UART_GPIO       GPIOA
#define UART_TX_PIN     2
#define UART_RX_PIN     3
#define CMD_UART        USART2

#endif // GPIO

#if 1 // =========================== I2C =======================================
// i2cclkPCLK1, i2cclkSYSCLK, i2cclkHSI
#define I2C_CLK_SRC     i2cclkHSI
#define I2C_BAUDRATE_HZ 400000
#endif

#if ADC_REQUIRED // ======================= Inner ADC ==========================
// Clock divider: clock is generated from the APB2
#define ADC_CLK_DIVIDER		adcDiv2

// ADC channels pins (GPIO, PIN, ADC channel)
#define ADC_LumMeas5V_PIN       GPIOA, 4, 9
#define ADC_VrefVirtual_PIN     GPIOA, 4, ADC_VREFINT_CHNL

#define ADC_SAMPLE_TIME         ast47d5Cycles   // ast24d5Cycles
#define ADC_OVERSAMPLING_RATIO  64   // 1 (no oversampling), 2, 4, 8, 16, 32, 64, 128, 256
#define MEAS_SampleFrequency_Hz 40

#endif

#if 1 // =========================== Timers ====================================
#endif

#if 1 // =========================== DMA =======================================
// ==== Uart ====
// Remap is made automatically if required
#define UART_DMA_TX     STM32_DMA_STREAM_ID(1, 7)
#define UART_DMA_RX     STM32_DMA_STREAM_ID(1, 6)
#define UART_DMA_CHNL   2

#define UART_DMA_TX_MODE(Chnl) (STM32_DMA_CR_CHSEL(Chnl) | DMA_PRIORITY_LOW | STM32_DMA_CR_MSIZE_BYTE | STM32_DMA_CR_PSIZE_BYTE | STM32_DMA_CR_MINC | STM32_DMA_CR_DIR_M2P | STM32_DMA_CR_TCIE)
#define UART_DMA_RX_MODE(Chnl) (STM32_DMA_CR_CHSEL(Chnl) | DMA_PRIORITY_MEDIUM | STM32_DMA_CR_MSIZE_BYTE | STM32_DMA_CR_PSIZE_BYTE | STM32_DMA_CR_MINC | STM32_DMA_CR_DIR_P2M | STM32_DMA_CR_CIRC)

// ==== SDMMC ====
//#define STM32_SDC_SDMMC1_DMA_STREAM   STM32_DMA_STREAM_ID(2, 4)

// ==== I2C ====
#define I2C_USE_DMA     TRUE
#define I2C1_DMA_TX     STM32_DMA_STREAM_ID(1, 6)
#define I2C1_DMA_RX     STM32_DMA_STREAM_ID(1, 7)
#define I2C1_DMA_CHNL   3
//#define I2C2_DMA_TX     STM32_DMA_STREAM_ID(1, 4)
//#define I2C2_DMA_RX     STM32_DMA_STREAM_ID(1, 5)
//#define I2C2_DMA_CHNL   3
//#define I2C3_DMA_TX     STM32_DMA_STREAM_ID(1, 2)
//#define I2C3_DMA_RX     STM32_DMA_STREAM_ID(1, 3)
//#define I2C3_DMA_CHNL   3

// ==== SPI3 ====
#define LED_DMA         STM32_DMA_STREAM_ID(2, 2)  // SPI3 TX (MOSI)
#define LED_DMA_CHNL    3

// ==== DAC ====
//#define DAC_DMA         STM32_DMA_STREAM_ID(2, 4)
//#define DAC_DMA_CHNL    3

#if ADC_REQUIRED
#define ADC_DMA         STM32_DMA_STREAM_ID(1, 1)
#define ADC_DMA_MODE    STM32_DMA_CR_CHSEL(0) |   /* DMA1 Stream1 Channel0 */ \
                        DMA_PRIORITY_LOW | \
                        STM32_DMA_CR_MSIZE_HWORD | \
                        STM32_DMA_CR_PSIZE_HWORD | \
                        STM32_DMA_CR_MINC |       /* Memory pointer increase */ \
                        STM32_DMA_CR_DIR_P2M |    /* Direction is peripheral to memory */ \
                        STM32_DMA_CR_TCIE         /* Enable Transmission Complete IRQ */
#endif // ADC

#endif // DMA

#if 1 // ========================== USART ======================================
#define PRINTF_FLOAT_EN FALSE
#define UART_TXBUF_SZ   4096
#define UART_RXBUF_SZ   1024
#define CMD_BUF_SZ      1024

#define CMD_UART_PARAMS \
    CMD_UART, UART_GPIO, UART_TX_PIN, UART_GPIO, UART_RX_PIN, \
    UART_DMA_TX, UART_DMA_RX, UART_DMA_TX_MODE(UART_DMA_CHNL), UART_DMA_RX_MODE(UART_DMA_CHNL), \
    uartclkHSI // Use independent clock

#endif
