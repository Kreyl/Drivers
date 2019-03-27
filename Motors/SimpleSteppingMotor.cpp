/*
 * SteppingMotor.cpp
 *
 *  Created on: 05 февр. 2017 г.
 *      Author: Elessar
 */

#include "SimpleSteppingMotor.h"

void StepperTmrCallback(void *p) {
    chSysLockFromISR();
    reinterpret_cast<SteppingMotor_t*>(p)->IStepperTmrCallbakHandlerI();
    chSysUnlockFromISR();
}


void SteppingMotor_t::SetSpeed(int32_t Speed, const StepMode_t Mode = smLikeBefore) {     // [об/мин * 0,1]
//    Start();

    if (Speed > 0) Rotation = srClockwise;
    else if (Speed < 0) { Rotation = srReverse; Speed *= -1; }

    MotorMode = Mode;
    if (Speed != 0){
        switch(MotorMode) {
            case smFullStep:
                Steps_CNT = 3;
                StepInterval_ms = (uint32_t)(1000*60*StepAngle*10)/(360 * GearRatio * Speed);
                break;
            case smHalfStep:
                Steps_CNT = 7;
                StepInterval_ms = (uint32_t)(1000*60*StepAngle*5)/(360 * GearRatio * Speed);
                break;
            default: break;
        }         // обороты в минуту /10 -> интервал между шагами [mS]
    }
    if (StepInterval_ms == 0) {
        StepInterval_ms = 1;
        Uart.Printf("StepInterval LIM MIN (1mS)\r");
    }
    else if (StepInterval_ms > StepInterval_MAX_MS) {
        StepInterval_ms = StepInterval_MAX_MS;
        Uart.Printf("StepInterval LIM MAX (%umS)\r", StepInterval_MAX_MS);
    }
    else Uart.Printf("Step Interval %u mS\r", StepInterval_ms);
}

void SteppingMotor_t::TaskI() {
//    uint16_t temp = PortGetValue(IMotor.PGpio);
//    temp = temp | (FullStepOperation[StepIndex] << PinA1);
//    PortSetValue(IMotor.PGpio, temp);
//    Uart.PrintfI("Motor Step\r");
    if (MotorMode == smFullStep) {
        switch(StepIndex) {
            case 0: // 0b1010
                PinSetHi(IMotor.PGpio, IMotor.PinA1);
                PinSetLo(IMotor.PGpio, IMotor.PinA2);
                PinSetHi(IMotor.PGpio, IMotor.PinB1);
                PinSetLo(IMotor.PGpio, IMotor.PinB2);
                break;
            case 1: // 0b0110
                PinSetLo(IMotor.PGpio, IMotor.PinA1);
                PinSetHi(IMotor.PGpio, IMotor.PinA2);
                PinSetHi(IMotor.PGpio, IMotor.PinB1);
                PinSetLo(IMotor.PGpio, IMotor.PinB2);
                break;
            case 2: // 0b0101
                PinSetLo(IMotor.PGpio, IMotor.PinA1);
                PinSetHi(IMotor.PGpio, IMotor.PinA2);
                PinSetLo(IMotor.PGpio, IMotor.PinB1);
                PinSetHi(IMotor.PGpio, IMotor.PinB2);
                break;
            case 3: // 0b1001
                PinSetHi(IMotor.PGpio, IMotor.PinA1);
                PinSetLo(IMotor.PGpio, IMotor.PinA2);
                PinSetLo(IMotor.PGpio, IMotor.PinB1);
                PinSetHi(IMotor.PGpio, IMotor.PinB2);
                break;
        }
    } else if (MotorMode == smHalfStep) {
        switch(StepIndex) {
            case 0: // 0b1010
                PinSetHi(IMotor.PGpio, IMotor.PinA1);
                PinSetLo(IMotor.PGpio, IMotor.PinA2);
                PinSetHi(IMotor.PGpio, IMotor.PinB1);
                PinSetLo(IMotor.PGpio, IMotor.PinB2);
                break;
            case 1: // 0b0010
                PinSetLo(IMotor.PGpio, IMotor.PinA1);
                PinSetLo(IMotor.PGpio, IMotor.PinA2);
                PinSetHi(IMotor.PGpio, IMotor.PinB1);
                PinSetLo(IMotor.PGpio, IMotor.PinB2);
                break;
            case 2: // 0b0110
                PinSetLo(IMotor.PGpio, IMotor.PinA1);
                PinSetHi(IMotor.PGpio, IMotor.PinA2);
                PinSetHi(IMotor.PGpio, IMotor.PinB1);
                PinSetLo(IMotor.PGpio, IMotor.PinB2);
                break;
            case 3: // 0b0100
                PinSetLo(IMotor.PGpio, IMotor.PinA1);
                PinSetHi(IMotor.PGpio, IMotor.PinA2);
                PinSetLo(IMotor.PGpio, IMotor.PinB1);
                PinSetLo(IMotor.PGpio, IMotor.PinB2);
                break;
            case 4: // 0b0101
                PinSetLo(IMotor.PGpio, IMotor.PinA1);
                PinSetHi(IMotor.PGpio, IMotor.PinA2);
                PinSetLo(IMotor.PGpio, IMotor.PinB1);
                PinSetHi(IMotor.PGpio, IMotor.PinB2);
                break;
            case 5: // 0b0001
                PinSetLo(IMotor.PGpio, IMotor.PinA1);
                PinSetLo(IMotor.PGpio, IMotor.PinA2);
                PinSetLo(IMotor.PGpio, IMotor.PinB1);
                PinSetHi(IMotor.PGpio, IMotor.PinB2);
                break;
            case 6: // 0b1001
                PinSetHi(IMotor.PGpio, IMotor.PinA1);
                PinSetLo(IMotor.PGpio, IMotor.PinA2);
                PinSetLo(IMotor.PGpio, IMotor.PinB1);
                PinSetHi(IMotor.PGpio, IMotor.PinB2);
                break;
            case 7: // 0b1000
                PinSetHi(IMotor.PGpio, IMotor.PinA1);
                PinSetLo(IMotor.PGpio, IMotor.PinA2);
                PinSetLo(IMotor.PGpio, IMotor.PinB1);
                PinSetLo(IMotor.PGpio, IMotor.PinB2);
                break;
        }
    }

//    Uart.PrintfI("StepIndex %u\r", StepIndex);

    switch(Rotation) {
        case srClockwise:
            if (StepIndex < Steps_CNT)
                StepIndex ++;
            else
                StepIndex = 0;
            break;
        case srReverse:
            if (StepIndex > 0)
                StepIndex --;
            else
                StepIndex = Steps_CNT;
            break;
    }
}




