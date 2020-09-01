/*
 * SteppingMotor.h
 *
 *  Created on: 05 февр. 2017 г.
 *      Author: Elessar
 */

#ifndef STEPPINGMOTOR_H_
#define STEPPINGMOTOR_H_

#include "board.h"
#include "uart.h"
#include "kl_lib.h"

//const uint8_t FullStepOperation[] = {
//        0b1010,
//        0b0110,
//        0b0101,
//        0b1001,
////        0b0000,
//};
//#define FullStep_CNT    countof(FullStepOperation)
//#define HalfStep_CNT    8

#define StepInterval_MAX_US 500000
#define StepInterval_MIN_US 250
#define InintTimeOut_MS     1500

struct MotorSetupPins_t {
    GPIO_TypeDef *PGpio;
    uint8_t PinA1;
    uint8_t PinA2;
    uint8_t PinB1;
    uint8_t PinB2;
};

enum PowerDelay_t {pdDelay = true, pdNoDelay = false};
enum StepMode_t {smLikeBefore, smFullStep, smHalfStep};
enum Rotation_t {srClockwise, srReverse};

void StepperTmrCallback(void *p);    // VirtualTimer callback


class SteppingMotor_t {
private:
    virtual_timer_t StepTMR;
    uint8_t StepIndex = 0;
    systime_t StepInterval_us = 0;
    uint8_t Steps_CNT = 3;
    StepMode_t MotorMode = smFullStep;
    Rotation_t Rotation = srClockwise;

    const MotorSetupPins_t IMotor;
    uint8_t PinSHDN;
    uint8_t StepAngle = 18;
    uint16_t GearRatio = 100;

    void StepperTmrStsrtI(){
        if (StepInterval_us) {
            if(chVTIsArmedI(&StepTMR)) chVTResetI(&StepTMR);
            chVTSetI(&StepTMR, US2ST(StepInterval_us), StepperTmrCallback, this);
        }
    }
    void TaskI();

public:
    SteppingMotor_t(MotorSetupPins_t AMotor, uint8_t APinSHDN) :
        IMotor(AMotor), PinSHDN(APinSHDN) {}

    void Init(uint8_t AStepAngle = 18, uint16_t AGearRatio = 100, const PowerDelay_t Delay = pdNoDelay) {
        PinSetupOut(IMotor.PGpio, IMotor.PinA1, omPushPull);
        PinSetupOut(IMotor.PGpio, IMotor.PinA2, omPushPull);
        PinSetupOut(IMotor.PGpio, IMotor.PinB1, omPushPull);
        PinSetupOut(IMotor.PGpio, IMotor.PinB2, omPushPull);
        PinSetupOut(IMotor.PGpio, PinSHDN, omPushPull);
        PinSetHi(IMotor.PGpio, PinSHDN);
        StepAngle = AStepAngle;
        GearRatio = AGearRatio;
        Uart.Printf("MotorAngle=%u, MotorRatio=%u\r", MotorAngle, MotorRatio);
        if (Delay) chThdSleepMilliseconds(InintTimeOut_MS);
    }
    void Start() {
        chSysLock();
        StepperTmrStsrtI();
        chSysUnlock();
    }
    void SetSpeed(int32_t Speed, const StepMode_t Mode);
    int32_t GetSpeed() {
        int32_t Result = 0;
        if (StepInterval_us)
            switch(MotorMode) {
                case smFullStep:
                    Result = (1000000UL*StepAngle*10)/(6U * GearRatio * StepInterval_us);
                    break;
                case smHalfStep:
                    Result = (1000000UL*StepAngle*5)/(6U * GearRatio * StepInterval_us);
                    break;
                default: break;
            }
        if (Rotation == srReverse) return -Result;
        else return Result;
    }
    void Stop() {
        chVTReset(&StepTMR);
        StepIndex = 0;
        PinSetLo(IMotor.PGpio, IMotor.PinA1);
        PinSetLo(IMotor.PGpio, IMotor.PinA2);
        PinSetLo(IMotor.PGpio, IMotor.PinB1);
        PinSetLo(IMotor.PGpio, IMotor.PinB2);
    }
    void Sleep() {
        Stop();
        PinSetLo(IMotor.PGpio, PinSHDN);
    }

    // Inner use
    void IStepperTmrCallbakHandlerI() {
        StepperTmrStsrtI();
        TaskI();
    }
};

#endif /* STEPPINGMOTOR_H_ */
