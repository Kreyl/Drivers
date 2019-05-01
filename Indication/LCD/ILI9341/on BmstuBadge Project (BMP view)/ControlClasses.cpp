/*
 * ControlClasses.cpp
 *
 *  Created on: 23 θώνÿ 2016 γ.
 *      Author: Kreyl
 */

#include "kl_lib.h"
#include "uart.h"
#include "color.h"
#include "ImageBMP.h"
#include "ILI9488.h"
#include "ControlClasses.h"

void Button_t::Init(uint32_t aY_Top, uint32_t aY_Bottom, uint32_t aX_Left, uint32_t aX_Right){
		Y_Top 		= aY_Top;
		Y_Bottom 	= aY_Bottom;
		X_Left 		= aX_Left;
		X_Right 	= aX_Right;
//		Printf("new button: TY = %d, BY = %d, LX = %d, RX =  %d \n\r", Y_Top, Y_Bottom, X_Left, X_Right);
};

void Page_t::Init(uint32_t aTop, uint32_t aLeft, const char* aPageNameOnDisk){
	Top = aTop;
	Left = aLeft;
	PageNameOnDisk = aPageNameOnDisk;
	NumberOfButtonsOnPage = 0;
}

void Page_t::Show() const{
	Lcd.DrawRect(0, 0, Lcd.Width, Lcd.Height, clBlack);
	Image.ShowImage(Top, Left, PageNameOnDisk);
}

void Page_t::BindButton(uint32_t Y_Top, uint32_t Y_Bottom,	uint32_t X_Left, uint32_t X_Right){
	if(NumberOfButtonsOnPage == 10) return;
	Buttons[NumberOfButtonsOnPage].Init(Y_Top, Y_Bottom, X_Left, X_Right);
	NumberOfButtonsOnPage++;
}
