/*
 * STSPIN220.h
 *
 *  Created on: 16 мар. 2021 г.
 *      Author: Elessar
 */

#ifndef STSPIN220_H_
#define STSPIN220_H_

#include "board.h"
#include "uart.h"
#include "kl_lib.h"


#define Vref_DAC    TRUE    // or Vrer PWM
#ifndef STS_MODE1_PIN
#define MODE1_PIN_IS_HI 0
#endif
#ifndef STS_MODE2_PIN
#define MODE2_PIN_IS_HI 1
#endif


#if Vref_DAC
#define STS_DAC_LVL_MIN     0
#define STS_DAC_LVL_MAX     195 // for delnver 30k/7.5k (to Vref 0.5V)
static uint8_t ConvertPercentToLevel(uint8_t Percent) {    // nom level is 14%
    return STS_DAC_LVL_MIN+((int16_t)(STS_DAC_LVL_MAX-STS_DAC_LVL_MIN) * Percent)/100;
}
#endif


#define nop( )  asm volatile ("nop\n\t" ::)
#define STS_MODE3_PIN   STS_STCK_PIN
#define STS_MODE4_PIN   STS_DIR_PIN

struct MotorSetupPins_t {
    GPIO_TypeDef *PGpio;
    uint8_t STCK_PIN;
    uint8_t DIR_PIN;
    uint8_t MODE1_PIN;
    uint8_t MODE2_PIN;
};

#if defined STS_MODE1_PIN and defined STS_MODE1_PIN
enum StepMode_t { smFullStep = 1, smHalfStep = 2, sm1_4ndStep = 4, sm1_8ndStep = 8, sm1_16ndStep = 16, sm1_32ndStep = 32, sm1_64ndStep = 64, sm1_128ndStep = 128, sm1_256ndStep = 256 };
#elif MODE1_PIN_IS_HI == 0 and MODE2_PIN_IS_HI == 0
enum StepMode_t { smFullStep = 2, sm1_32ndStep = 8, sm1_128ndStep = 128, sm1_256ndStep = 256 };
#elif MODE1_PIN_IS_HI == 1 and MODE2_PIN_IS_HI == 0
enum StepMode_t { smHalfStep = 2, sm1_8ndStep = 8, sm1_128ndStep = 128, sm1_256ndStep = 256 };
#elif MODE1_PIN_IS_HI == 0 and MODE2_PIN_IS_HI == 1
enum StepMode_t { sm1_4ndStep = 4, sm1_32ndStep = 32, sm1_64ndStep = 64, sm1_256ndStep = 256 };
#elif MODE1_PIN_IS_HI == 1 and MODE2_PIN_IS_HI == 1
enum StepMode_t { sm1_8ndStep = 8, sm1_16ndStep = 16, sm1_64ndStep = 64, sm1_256ndStep = 256 };
#endif

enum Rotation_t { srClockwise, srReverse };


class STSPIN220_t {
private:
    PinOutputPWM_t STCK_Tim {STS_STCK_TIM};
#ifndef Vref_DAC // Vref PWM
    PinOutputPWM_t Vref_Tim {STS_Vref_PWM};
#endif
    int32_t CurrentSpeed = 0;
    uint8_t StepAngle = 18;
    uint16_t GearRatio = 100;
    StepMode_t FractionalStep = sm1_256ndStep;
    uint16_t CurrentVrefPercent = 0;
    bool isRun = false;

//    const GPIO_TypeDef *SpiGpio, *CSGpio;
//    const uint16_t Sck, Miso, Mosi, Cs;

//    const MotorSetupPins_t IMotor;
//    const uint8_t VREF_PIN, STBY_PIN, EN_PIN;
//    const uint8_t MODE1_PIN, MODE2_PIN, STCK_PIN, DIR_PIN;

    uint8_t SetupStepMode(StepMode_t AStepMode) {
        uint8_t Result = retvOk;
        PinSetLo(STS_GPIO, STS_STBY_PIN);
        chThdSleepMicroseconds(1);
        switch(AStepMode) {
#if defined STS_MODE1_PIN and defined STS_MODE1_PIN
            case smFullStep:
                PinSetLo(STS_GPIO, STS_MODE3_PIN);
                PinSetLo(STS_GPIO, STS_MODE4_PIN);
                PinSetLo(STS_GPIO, STS_MODE1_PIN);
                PinSetLo(STS_GPIO, STS_MODE2_PIN);
                break;
            case smHalfStep:
                PinSetHi(STS_GPIO, STS_MODE3_PIN);
                PinSetLo(STS_GPIO, STS_MODE4_PIN);
                PinSetHi(STS_GPIO, STS_MODE1_PIN);
                PinSetLo(STS_GPIO, STS_MODE2_PIN);
                break;
            case sm1_4ndStep:
                PinSetLo(STS_GPIO, STS_MODE3_PIN);
                PinSetHi(STS_GPIO, STS_MODE4_PIN);
                PinSetLo(STS_GPIO, STS_MODE1_PIN);
                PinSetHi(STS_GPIO, STS_MODE2_PIN);
                break;
            case sm1_8ndStep:
//                PinSetHi(STS_GPIO, STS_MODE3_PIN);
//                PinSetLo(STS_GPIO, STS_MODE4_PIN);
//                PinSetHi(STS_GPIO, STS_MODE1_PIN);
//                PinSetHi(STS_GPIO, STS_MODE2_PIN);
                // or
                PinSetHi(STS_GPIO, STS_MODE3_PIN);
                PinSetHi(STS_GPIO, STS_MODE4_PIN);
                PinSetHi(STS_GPIO, STS_MODE1_PIN);
                PinSetLo(STS_GPIO, STS_MODE2_PIN);
                break;
            case sm1_16ndStep:
                PinSetHi(STS_GPIO, STS_MODE3_PIN);
                PinSetHi(STS_GPIO, STS_MODE4_PIN);
                PinSetHi(STS_GPIO, STS_MODE1_PIN);
                PinSetHi(STS_GPIO, STS_MODE2_PIN);
                break;
            case sm1_32ndStep:
                PinSetLo(STS_GPIO, STS_MODE3_PIN);
                PinSetLo(STS_GPIO, STS_MODE4_PIN);
                PinSetLo(STS_GPIO, STS_MODE1_PIN);
                PinSetHi(STS_GPIO, STS_MODE2_PIN);
                break;
            case sm1_64ndStep:
//                PinSetLo(STS_GPIO, STS_MODE3_PIN);
//                PinSetHi(STS_GPIO, STS_MODE4_PIN);
//                PinSetHi(STS_GPIO, STS_MODE1_PIN);
//                PinSetHi(STS_GPIO, STS_MODE2_PIN);
                // or
                PinSetHi(STS_GPIO, STS_MODE3_PIN);
                PinSetHi(STS_GPIO, STS_MODE4_PIN);
                PinSetLo(STS_GPIO, STS_MODE1_PIN);
                PinSetHi(STS_GPIO, STS_MODE2_PIN);
                break;
            case sm1_128ndStep:
                PinSetLo(STS_GPIO, STS_MODE3_PIN);
                PinSetLo(STS_GPIO, STS_MODE4_PIN);
                PinSetHi(STS_GPIO, STS_MODE1_PIN);
                PinSetLo(STS_GPIO, STS_MODE2_PIN);
                break;
            case sm1_256ndStep:
                PinSetLo(STS_GPIO, STS_MODE3_PIN);
                PinSetLo(STS_GPIO, STS_MODE4_PIN);
                PinSetHi(STS_GPIO, STS_MODE1_PIN);
                PinSetHi(STS_GPIO, STS_MODE2_PIN);
                // or
//                PinSetLo(STS_GPIO, STS_MODE3_PIN);
//                PinSetHi(STS_GPIO, STS_MODE4_PIN);
//                PinSetHi(STS_GPIO, STS_MODE1_PIN);
//                PinSetLo(STS_GPIO, STS_MODE2_PIN);
                // or
//                PinSetHi(STS_GPIO, STS_MODE3_PIN);
//                PinSetLo(STS_GPIO, STS_MODE4_PIN);
//                PinSetLo(STS_GPIO, STS_MODE1_PIN);
//                PinSetHi(STS_GPIO, STS_MODE2_PIN);
                break;
#endif

#if MODE1_PIN_IS_HI == 0 and MODE2_PIN_IS_HI == 0
            case smFullStep:
                PinSetLo(STS_GPIO, STS_MODE3_PIN);
                PinSetLo(STS_GPIO, STS_MODE4_PIN);
                break;
#endif
#if MODE1_PIN_IS_HI == 1 and MODE2_PIN_IS_HI == 0
            case smHalfStep:
                PinSetHi(STS_GPIO, STS_MODE3_PIN);
                PinSetLo(STS_GPIO, STS_MODE4_PIN);
                break;
#endif
#if MODE1_PIN_IS_HI == 0 and MODE2_PIN_IS_HI == 1
            case sm1_4ndStep:
                PinSetLo(STS_GPIO, STS_MODE3_PIN);
                PinSetHi(STS_GPIO, STS_MODE4_PIN);
                break;
#endif
#if MODE1_PIN_IS_HI == 1 and MODE2_PIN_IS_HI == 0
            case sm1_8ndStep:
                PinSetHi(STS_GPIO, STS_MODE3_PIN);
                PinSetHi(STS_GPIO, STS_MODE4_PIN);
                break;
#elif MODE1_PIN_IS_HI == 1 and MODE2_PIN_IS_HI == 1
            case sm1_8ndStep:
                PinSetHi(STS_GPIO, STS_MODE3_PIN);
                PinSetLo(STS_GPIO, STS_MODE4_PIN);
                break;
#endif
#if MODE1_PIN_IS_HI == 1 and MODE2_PIN_IS_HI == 1
            case sm1_16ndStep:
                PinSetHi(STS_GPIO, STS_MODE3_PIN);
                PinSetHi(STS_GPIO, STS_MODE4_PIN);
                break;
#endif
#if MODE1_PIN_IS_HI == 0 and MODE2_PIN_IS_HI == 0
            case sm1_32ndStep:
                PinSetLo(STS_GPIO, STS_MODE3_PIN);
                PinSetHi(STS_GPIO, STS_MODE4_PIN);
                break;
#elif MODE1_PIN_IS_HI == 0 and MODE2_PIN_IS_HI == 1
            case sm1_32ndStep:
                PinSetLo(STS_GPIO, STS_MODE3_PIN);
                PinSetLo(STS_GPIO, STS_MODE4_PIN);
                break;
#endif
#if MODE1_PIN_IS_HI == 0 and MODE2_PIN_IS_HI == 1
            case sm1_64ndStep:
                PinSetHi(STS_GPIO, STS_MODE3_PIN);
                PinSetHi(STS_GPIO, STS_MODE4_PIN);
                break;
#elif MODE1_PIN_IS_HI == 1 and MODE2_PIN_IS_HI == 1
            case sm1_64ndStep:
                PinSetLo(STS_GPIO, STS_MODE3_PIN);
                PinSetHi(STS_GPIO, STS_MODE4_PIN);
                break;
#endif
#if MODE1_PIN_IS_HI == 0 and MODE2_PIN_IS_HI == 0
            case sm1_128ndStep:
                PinSetHi(STS_GPIO, STS_MODE3_PIN);
                PinSetLo(STS_GPIO, STS_MODE4_PIN);
                break;
#elif MODE1_PIN_IS_HI == 1 and MODE2_PIN_IS_HI == 0
            case sm1_128ndStep:
                PinSetLo(STS_GPIO, STS_MODE3_PIN);
                PinSetLo(STS_GPIO, STS_MODE4_PIN);
                break;
#endif
#if MODE1_PIN_IS_HI == 0 and MODE2_PIN_IS_HI == 0
            case sm1_256ndStep:
                PinSetHi(STS_GPIO, STS_MODE3_PIN);
                PinSetHi(STS_GPIO, STS_MODE4_PIN);
                break;
#elif MODE1_PIN_IS_HI == 1 and MODE2_PIN_IS_HI == 0
            case sm1_256ndStep:
                PinSetLo(STS_GPIO, STS_MODE3_PIN);
                PinSetHi(STS_GPIO, STS_MODE4_PIN);
                break;
#elif MODE1_PIN_IS_HI == 0 and MODE2_PIN_IS_HI == 1
            case sm1_256ndStep:
                PinSetHi(STS_GPIO, STS_MODE3_PIN);
                PinSetLo(STS_GPIO, STS_MODE4_PIN);
                break;
#elif MODE1_PIN_IS_HI == 1 and MODE2_PIN_IS_HI == 1
            case sm1_256ndStep:
                PinSetLo(STS_GPIO, STS_MODE3_PIN);
                PinSetLo(STS_GPIO, STS_MODE4_PIN);
                break;
#endif
            default:
                Result = retvBadValue;
                break;
        } // switch AStepMode
        if (Result == retvOk) FractionalStep = AStepMode;
        chThdSleepMilliseconds(1);
        PinSetHi(STS_GPIO, STS_STBY_PIN);
        chThdSleepMilliseconds(1);
        return Result;
    }
    void SetRotation(const Rotation_t ARotation) {
        if (ARotation == srClockwise)
            PinSetHi(STS_GPIO, STS_DIR_PIN);
        else
            PinSetLo(STS_GPIO, STS_DIR_PIN);
    }

public:
    void Init(uint8_t AStepAngle = 18, uint16_t AGearRatio = 100, const StepMode_t AStepMode = sm1_256ndStep, uint8_t AVrefPercent = 50) {
        PinSetupOut(STS_GPIO, STS_STBY_PIN, omPushPull);
        PinSetupOut(STS_GPIO, STS_STCK_PIN, omPushPull);
        PinSetupOut(STS_GPIO, STS_DIR_PIN, omPushPull);
#if defined STS_MODE1_PIN and defined STS_MODE1_PIN
        PinSetupOut(STS_GPIO, STS_MODE1_PIN, omPushPull);
        PinSetupOut(STS_GPIO, STS_MODE2_PIN, omPushPull);
#endif
//        PinSetupInput(STS_GPIO, FAULT_PIN, pudNone);
//        PinSetupOut(STS_GPIO, STS_EN_PIN, omPushPull);
//        PinSetHi(STS_GPIO, STS_EN_PIN);

#if Vref_DAC
        PinSetupAnalog(STS_Vref_DAC_PIN);
        rccEnableDAC1(FALSE);
        DAC->CR = DAC_CR_EN1;   // Enable Ch1
        DAC->DHR8R1 = 0;
#else // Vref PWM
        PinSetupOut(STS_GPIO, STS_Vref_PIN, omPushPull);
        Vref_Tim.Init();
        Vref_Tim.SetFrequencyHz(10000);
#endif
        SetVrefPercent(AVrefPercent);

        if (SetupStepMode(AStepMode) != retvOk)
            SetupStepMode(sm1_256ndStep);
        STCK_Tim.Init();
        StepAngle = AStepAngle;
        GearRatio = AGearRatio;
        Printf("MotorAngle=%u, MotorRatio=%u\r", StepAngle, GearRatio);
    }

    void SetSpeed(int32_t ASpeed_x10) {
        CurrentSpeed = ASpeed_x10;
        if (ASpeed_x10 > 0) SetRotation(srClockwise);
        else if (ASpeed_x10 < 0) { SetRotation(srReverse); ASpeed_x10 *= -1; }
        else { Stop(); return; } // if Speed == 0

        uint32_t StepFrequencyHz = (6U * GearRatio * ASpeed_x10 * FractionalStep)/(StepAngle*10);
//        Printf("Motor STCKtmr=%uHz\r", StepFrequencyHz);
        STCK_Tim.SetFrequencyHz(StepFrequencyHz);
        if (!isRun) {
            STCK_Tim.Set(1);
            isRun = true;
        }
    }
    uint8_t SetStepMode(const StepMode_t AStepMode) {
        uint8_t Result = retvOk;
        Stop();
        STCK_Tim.Deinit();
        PinSetupOut(STS_GPIO, STS_STCK_PIN, omPushPull);
        Result = SetupStepMode(AStepMode);
        STCK_Tim.Init();
        if (Result == retvOk)
            Start();
        return Result;
    }
    void SetVrefPercent(uint8_t AVrefPercent) {
        if (AVrefPercent > 100)
            AVrefPercent = 100;
        CurrentVrefPercent = AVrefPercent;
#if Vref_DAC
        DAC->DHR8R1 = ConvertPercentToLevel(AVrefPercent);
#else // Vref PWM
        Vref_Tim.Set(AVrefPercent);
#endif
    }

    void Stop() {
        isRun = false;
        STCK_Tim.Set(0);
    }
    void Start() {
        SetSpeed(CurrentSpeed);
    }

    void ShutDown() {
        Stop();
        STCK_Tim.Deinit();
        PinSetLo(STS_GPIO, STS_STBY_PIN);
#if Vref_DAC
        DAC->DHR8R1 = 0;
#else // Vref PWM
        Vref_Tim.Set(0);
#endif
    }
    void Resume() {
        Init(StepAngle, GearRatio, (StepMode_t)FractionalStep, CurrentVrefPercent);
        SetSpeed(CurrentSpeed);
    }

    // Inner use
//    STSPIN220_t(MotorSetupPins_t AMotor, uint8_t ASTBY_PIN) :
//        IMotor(AMotor), STBY_PIN(ASTBY_PIN) {}
};

#endif /* STSPIN220_H_ */
