/*
 * gui_engine.cpp
 *
 *  Created on: 9 дек. 2017 г.
 *      Author: nikit
 */

#include "gui_engine.h"
#include "color.h"
#include "ImageBMP.h"
#include "Navigation.h"

Gui_t Gui;

static THD_WORKING_AREA(waGuiThread, 2048);
__noreturn
static THD_FUNCTION(GuiThread, arg) {
    chRegSetThreadName("Gui");
    Gui.ITask();
}

Page_t MainPage;
Page_t NaviPage;
Page_t BadgePage;

__always_inline
static inline void BindButtonsToPages(){
	const uint32_t mainPage_Btn1_TY = 20;
	const uint32_t mainPage_Btn1_BY = 52;
	const uint32_t mainPage_Btn1_LX = 130;
	const uint32_t mainPage_Btn1_RX = 346;
	MainPage.BindButton(mainPage_Btn1_TY, mainPage_Btn1_BY, mainPage_Btn1_LX, mainPage_Btn1_RX);

	const uint32_t mainPage_Btn2_TY = 70;
	const uint32_t mainPage_Btn2_BY = 108;
	const uint32_t mainPage_Btn2_LX = 130;
	const uint32_t mainPage_Btn2_RX = 346;
	MainPage.BindButton(mainPage_Btn2_TY, mainPage_Btn2_BY, mainPage_Btn2_LX, mainPage_Btn2_RX);


	const uint32_t naviPage_Btn1_TY = 244;
	const uint32_t naviPage_Btn1_BY = 295;
	const uint32_t naviPage_Btn1_LX = 5;
	const uint32_t naviPage_Btn1_RX = 79;
	NaviPage.BindButton(naviPage_Btn1_TY, naviPage_Btn1_BY, naviPage_Btn1_LX, naviPage_Btn1_RX);

	const uint32_t badgePage_Btn1_TY = 244;
	const uint32_t badgePage_Btn1_BY = 295;
	const uint32_t badgePage_Btn1_LX = 5;
	const uint32_t badgePage_Btn1_RX = 79;
	NaviPage.BindButton(badgePage_Btn1_TY, badgePage_Btn1_BY, badgePage_Btn1_LX, badgePage_Btn1_RX);
}

void Gui_t::Init(void){
    Lcd.Init();
	MainPage.Init(0, 0, "mainMenu.bmp");
	NaviPage.Init(0, 0, "navi.bmp");
	BadgePage.Init(0, 0,"ostranna.bmp");
	BindButtonsToPages();
	CurrPage = &MainPage;
	CurrPage->Show();

    chThdCreateStatic(waGuiThread, sizeof(waGuiThread), HIGHPRIO, GuiThread, NULL);
}

void Gui_t::DrawDigit(uint16_t Top, uint16_t Left, uint8_t Digit){
	switch (Digit){
	case 0:
		Image.ShowImage(Top, Left, "num0.bmp");
		break;
	case 1:
		Image.ShowImage(Top, Left, "num1.bmp");
		break;
	case 2:
		Image.ShowImage(Top, Left, "num2.bmp");
		break;
	case 3:
		Image.ShowImage(Top, Left, "num3.bmp");
		break;
	case 4:
		Image.ShowImage(Top, Left, "num4.bmp");
		break;
	case 5:
		Image.ShowImage(Top, Left, "num5.bmp");
		break;
	case 6:
		Image.ShowImage(Top, Left, "num6.bmp");
		break;
	case 7:
		Image.ShowImage(Top, Left, "num7.bmp");
		break;
	case 8:
		Image.ShowImage(Top, Left, "num8.bmp");
		break;
	case 9:
		Image.ShowImage(Top, Left, "num9.bmp");
		break;
	default: break;
	}
}

void Gui_t::DrawNumber(uint16_t Top, uint16_t Left, int16_t Number, uint8_t DecimalDigits, const char* someSuffixOnDisk){
	bool NegativeNumber = (Number < 0);

	if(NegativeNumber){
		Number = -Number;
		Image.ShowImage(Top, Left+=8, "sign_minus.bmp");
	}

	uint8_t ThousandsDigit = Number / 1000;
	uint8_t HundreedsDigit = (Number - ThousandsDigit * 1000) / 100;
	uint8_t TensDigit = (Number - ThousandsDigit * 1000 - HundreedsDigit * 100) / 10;
	uint8_t OnesDigit = Number - ThousandsDigit * 1000 - HundreedsDigit * 100 - TensDigit * 10;

	bool NumberDrawStarted = false;
	if((ThousandsDigit > 0) || (NumberDrawStarted))	{
		this->DrawDigit(Top, Left+=8, ThousandsDigit);
		NumberDrawStarted = true;
	}

	if((HundreedsDigit > 0) || (NumberDrawStarted) || (DecimalDigits > 1)) {
		this->DrawDigit(Top, Left+=8, HundreedsDigit);
		NumberDrawStarted = true;
	}

	if(DecimalDigits == 2){
		Image.ShowImage(Top, Left+=8, "sign_point.bmp");
	}

	if((TensDigit > 0) || (NumberDrawStarted) || (DecimalDigits > 0)){
		this->DrawDigit(Top, Left+=8, TensDigit);
		NumberDrawStarted = true;
	}

	if(DecimalDigits == 1){
		Image.ShowImage(Top, Left+=8, "sign_point.bmp");
	}

	this->DrawDigit(Top, Left+=8, OnesDigit);
	NumberDrawStarted = true;

	if(someSuffixOnDisk !="") Image.ShowImage(Top, Left+=8, someSuffixOnDisk);
}

__noreturn
void Gui_t::ITask() {
	while(true){
		if (Touch.IsTouched()) {
			Touch.ReadData();
			if((CurrPage = &MainPage) && (CurrPage->TouchProcess(Touch.X, Touch.Y) == 1)){
				CurrPage = &NaviPage;
				CurrPage -> Show();
				Navigation.NavigationVisible = true;
			}
			if((CurrPage = &MainPage) && (CurrPage->TouchProcess(Touch.X, Touch.Y) == 2)){
				CurrPage = &BadgePage;
				CurrPage -> Show();
			}
			if((CurrPage = &NaviPage) && (CurrPage->TouchProcess(Touch.X, Touch.Y) == 1)){
				CurrPage = &MainPage;
				CurrPage -> Show();
				Navigation.NavigationVisible = false;
			}
			if((CurrPage = &BadgePage) && (CurrPage->TouchProcess(Touch.X, Touch.Y) == 1)){
				CurrPage = &MainPage;
				CurrPage -> Show();
			}
//			Printf("touch result = %d", CurrPage->TouchProcess(Touch.X, Touch.Y));
//			Lcd.DrawRect(Touch.X, Touch.Y, 10, 10, clBlack);
		}
	    chThdSleepMilliseconds(450);
	}
}
