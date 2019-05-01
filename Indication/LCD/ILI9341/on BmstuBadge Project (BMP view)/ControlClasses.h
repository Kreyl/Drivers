/*
 * ControlClasses.h
 *
 *  Created on: 15 ??? 2016 ?.
 *      Author: Kreyl
 */

#pragma once

#include "color.h"
#include "font.h"
// ====Button====

class Button_t{
private:
	uint32_t Y_Top;
	uint32_t Y_Bottom;
	uint32_t X_Left;
	uint32_t X_Right;
public:
	void Init(uint32_t aY_Top, uint32_t aY_Bottom, uint32_t aX_Left, uint32_t aX_Right);
	bool isTouched(uint32_t X, uint32_t Y) const{
//		Printf("check button: TY = %d, BY = %d, LX = %d, RX =  %d, X = %d, Y = %d \n\r", Y_Top, Y_Bottom, X_Left, X_Right, X, Y);
		return ((X > X_Left) && (X < X_Right) && (Y > Y_Top) && (Y < Y_Bottom));
	}

};

// ==== Page ====
class Page_t {
private:
	const char* PageNameOnDisk;
	Button_t Buttons[10];
	uint8_t NumberOfButtonsOnPage;
	uint32_t Top;
	uint32_t Left;
public:
    void Init(uint32_t aTop, uint32_t aLeft, const char* aPageNameOnDisk);
    void BindButton(uint32_t Y_Top, uint32_t Y_Bottom,	uint32_t X_Left, uint32_t X_Right);
    void Show() const;
    uint8_t TouchProcess(uint32_t X, uint32_t Y) const{
    	for (int i = 0; i < NumberOfButtonsOnPage; i++) if(Buttons[i].isTouched(X, Y))  return i+1;
    	return 0;
    }


};
