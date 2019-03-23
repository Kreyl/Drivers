/*
 * motor.h
 *
 *  Created on: 25 июля 2015 г.
 *      Author: Kreyl
 */

#ifndef MOTOR_MOTOR_H_
#define MOTOR_MOTOR_H_

#include "L6470.h"

// Using default 128 microsteps value

#define MOTOR_SPEED     2000
#define MOTOR_ACC       60

class Motor_t : private L6470_t {
public:
    bool IsRunning = false;
    void Init() {
        L6470_t::Init();
        SetAcceleration(MOTOR_ACC);
        SetDeceleration(MOTOR_ACC);
        SetConfig(0x2EA8);  // Default 2E88; voltage compensation enabled
    }
    void Run() {
        L6470_t::Run(dirReverse, MOTOR_SPEED);
        IsRunning = true;
    }
    void Stop() {
        L6470_t::StopSoftAndHiZ();
        IsRunning = false;
    }
};



#endif /* MOTOR_MOTOR_H_ */
