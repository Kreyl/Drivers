/*
 * gui.h
 *
 *  Created on: 13 ??? 2016 ?.
 *      Author: Kreyl
 */

#pragma once

#include "ILI9488.h"
#include "kl_lib.h"
#include "kl_buf.h"
#include "stmpe811.h"
#include "ControlClasses.h"

#define FRAMEBUFFER_LEN             20000   // 200x100
#define TOUCH_POLLING_PERIOD_MS     18

class Gui_t {
private:
    const Page_t *CurrPage;
    void DrawDigit(uint16_t Top, uint16_t Left, uint8_t Digit);
public:
    void Init();
    void DrawPage(uint8_t APage);
    // Inner use
    void ITask();
    void DrawNumber(uint16_t Top, uint16_t Left, int16_t Number, uint8_t DecimalDigits, const char* someSuffixOnDisk);
};

extern Gui_t Gui;
