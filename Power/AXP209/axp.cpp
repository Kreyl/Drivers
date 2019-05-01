/*
 * axp.cpp
 *
 *  Created on: 11 но€б. 2017 г.
 *      Author: nikit
 */

#include "axp.h"
#include "shell.h"
#include "MsgQ.h"

axp_t axp;

static THD_WORKING_AREA(waAxpThread, 128);
__noreturn
static THD_FUNCTION(AxpThread, arg) {
    chRegSetThreadName("AXP");
    axp.ITask();
}

uint8_t axpAddress = 0x34;

uint8_t reg_powerStatus 				= 0x00;
uint8_t reg_powerMode 					= 0x01;
uint8_t reg_VBUSStatus 					= 0x02;
uint8_t reg_PwrOutCtrl				 	= 0x12;
uint8_t reg_DCDC2_voltage 				= 0x23;
uint8_t reg_DCDC2_LDO3_ramp 			= 0x25;
uint8_t reg_DCDC3_voltage 				= 0x27;
uint8_t reg_LDO24_voltage 				= 0x28;
uint8_t reg_LDO3_voltage 				= 0x29;
uint8_t reg_VBUS_IPSOUT_path 			= 0x30;
uint8_t reg_shutdownVolage 				= 0x31;
uint8_t reg_shutdown_batDetect_LEDCtrl	= 0x32;
uint8_t reg_chargeCtrl_1				= 0x33;
uint8_t reg_chargeCtrl_2 				= 0x34;
uint8_t reg_backUpBatChargeCtrl 		= 0x35;
uint8_t reg_PEK_paramSet 				= 0x36;
uint8_t reg_DCDC_freqSet 				= 0x37;
uint8_t reg_batChrgLowTempAlarmSet 		= 0x38;
uint8_t reg_batChrgTempAlarmSet 		= 0x39;
uint8_t reg_APS_lowPwrSetLevel_1 		= 0x3A;
uint8_t reg_APS_lowPwrSetLevel_2 		= 0x3B;
uint8_t reg_BatDischargeLowTempAlarmSet	= 0x3C;
uint8_t reg_BatDischargeTempAlarmSet 	= 0x3D;
uint8_t reg_DCDC_workModeSet 			= 0x80;
uint8_t reg_ADC_enableSet_1		 		= 0x82;
uint8_t reg_ADC_enableSet_2		 		= 0x83;
uint8_t reg_ADC_sampleRate_TSpinSet		= 0x84;
uint8_t reg_GPIO_inputRangeSet	 		= 0x85;
uint8_t reg_GPIO1_ADC_IRQ_risThreshold	= 0x86;
uint8_t reg_GPIO1_ADC_IRQ_fallThreshold	= 0x87;
uint8_t reg_timerCtrl		 			= 0x8A;
uint8_t reg_VBUSMonitorSet	 			= 0x8B;
uint8_t reg_OverTempShutdownCtrl		= 0x8F;

uint8_t reg_GPIO0_Ctrl		 			= 0x90;
uint8_t reg_LDO5_VoltageSet	 			= 0x91;
uint8_t reg_GPIO1_Ctrl		 			= 0x92;
uint8_t reg_GPIO2_Ctrl		 			= 0x93;
uint8_t reg_GPIOSignalStatus 			= 0x94;
uint8_t reg_GPIO3_Ctrl		 			= 0x95;

uint8_t reg_IRQ_enableCtrl_1 			= 0x40;
uint8_t reg_IRQ_enableCtrl_2 			= 0x41;
uint8_t reg_IRQ_enableCtrl_3 			= 0x42;
uint8_t reg_IRQ_enableCtrl_4 			= 0x43;
uint8_t reg_IRQ_enableCtrl_5 			= 0x44;
uint8_t reg_IRQ_status_1	 			= 0x48;
uint8_t reg_IRQ_status_2	 			= 0x49;
uint8_t reg_IRQ_status_3	 			= 0x4A;
uint8_t reg_IRQ_status_4	 			= 0x4B;
uint8_t reg_IRQ_status_5	 			= 0x4C;

uint8_t reg_ACIN_VoltageH	 			= 0x56;
uint8_t reg_ACIN_VoltageL	 			= 0x57;
uint8_t reg_ACIN_CurrentH	 			= 0x58;
uint8_t reg_ACIN_CurrentL	 			= 0x59;
uint8_t reg_VBUS_VoltageH	 			= 0x5A;
uint8_t reg_VBUS_VoltageL	 			= 0x5B;
uint8_t reg_VBUS_CurrentH	 			= 0x5C;
uint8_t reg_VBUS_CurrentL	 			= 0x5D;

uint8_t reg_intTempH 					= 0x5E;
uint8_t reg_intTempL 					= 0x5F;
uint8_t reg_TS_inputH	 				= 0x62;
uint8_t reg_TS_inputL 					= 0x63;

uint8_t reg_GPIO0VoltageH	 			= 0x64;
uint8_t reg_GPIO0VoltageL	 			= 0x65;
uint8_t reg_GPIO1VoltageH	 			= 0x66;
uint8_t reg_GPIO1VoltageL	 			= 0x67;
uint8_t reg_instantPwrBat	 			= 0x70;
uint8_t reg_instantPwrInBat	 			= 0x71;
uint8_t reg_instantBatPwrIsLow 			= 0x72;

uint8_t reg_BatVoltageH	 				= 0x78;
uint8_t reg_BatVoltageL	 				= 0x79;
uint8_t reg_BatChargeCurrentH			= 0x7A;
uint8_t reg_BatChargeCurrentL 			= 0x7B;
uint8_t reg_BatDischargeCurrentH		= 0x7C;
uint8_t reg_BatDischargeCurrentL 		= 0x7D;
uint8_t reg_IPSOutVoltageH	 			= 0x7E;
uint8_t reg_IPSOutVoltageL	 			= 0x7F;

void axp_t::init(i2c_t* pi2c){
	this->i2c=pi2c;
	uint8_t dataToWrite[]={reg_ADC_enableSet_1,0xFF};
	i2c->Write(axpAddress,dataToWrite,2);
    chThdCreateStatic(waAxpThread, sizeof(waAxpThread), NORMALPRIO, AxpThread, NULL);
}

uint8_t axp_t::readStatusRegister(){

	uint8_t returnData;
	i2c->WriteRead(axpAddress,&reg_powerStatus,1,&returnData,1);
	return returnData;
}

void axp_t::setDCDC3milliVoltage(uint16_t milliVoltage) {
    uint16_t dataVoltage = (milliVoltage - 700) * 40;
    uint8_t dataToWrite[] = { reg_DCDC3_voltage, (uint8_t)dataVoltage };
    i2c->Write(axpAddress, dataToWrite, 2);
}

void axp_t::setLDO2milliVoltage(uint16_t milliVoltage){
	uint8_t returnData;
	i2c->WriteRead(axpAddress, &reg_LDO24_voltage, 1, &returnData, 1);
	uint16_t dataVoltage = (milliVoltage - 1800) / 100;
	returnData &= 0x0F;
	returnData |= dataVoltage << 4;
	uint8_t dataToWrite[2] = {reg_LDO24_voltage, returnData};
	i2c->Write(axpAddress, dataToWrite, 2);
}

void axp_t::setLDO4To2500mV() {
    uint8_t regData;
    i2c->WriteRead(axpAddress, &reg_LDO24_voltage, 1, &regData, 1);
    regData &= 0xF0;
    regData |= 9;
    uint8_t dataToWrite[] = { reg_LDO24_voltage, regData };
    i2c->Write(axpAddress, dataToWrite, 2);
}

void axp_t::turnOnDCDC3() {
    uint8_t returnData;
    i2c->WriteRead(axpAddress, &reg_PwrOutCtrl, 1, &returnData, 1);
    returnData |= 1 << 4; //mask for DCDC3
    uint8_t dataToWrite[] = { reg_PwrOutCtrl, returnData };
    i2c->Write(axpAddress, dataToWrite, 2);
}

void axp_t::turnOnLDO2(){
	uint8_t returnData;
	i2c->WriteRead(axpAddress,&reg_PwrOutCtrl,1,&returnData,1);
	returnData|=1<<2; //mask for LDO2
	uint8_t dataToWrite[]={reg_PwrOutCtrl,0x0E};
	i2c->Write(axpAddress,dataToWrite,2);
}
void axp_t::turnOnLDO4(){
	uint8_t returnData;
	i2c->WriteRead(axpAddress,&reg_PwrOutCtrl,1,&returnData,1);
	returnData|=1<<3; //mask for LDO4
	uint8_t dataToWrite[]={reg_PwrOutCtrl,0x0E};
	i2c->Write(axpAddress,dataToWrite,2);
}

void axp_t::keyShortStartShortFinish(){
	uint8_t dataToWrite[]={reg_PEK_paramSet,0x0C};
	i2c->Write(axpAddress,dataToWrite,2);
}

uint16_t axp_t::readChargeStatus(){
	uint8_t returnDataH;
	uint8_t returnDataL;
	i2c->WriteRead(axpAddress,&reg_powerStatus,1,&returnDataH,1);
	i2c->WriteRead(axpAddress,&reg_powerMode,1,&returnDataL,1);
	uint16_t returnData=(returnDataH<<4)|returnDataL;
    Printf("Power Status= %u \n\r",returnDataH);
    Printf("Power Mode= %u \n\r",returnDataL);
	return returnData;

}

uint16_t axp_t::readVBUSVoltage(){
	uint8_t returnDataH;
	uint8_t returnDataL;
	i2c->WriteRead(axpAddress,&reg_VBUS_VoltageH,1,&returnDataH,1);
	i2c->WriteRead(axpAddress,&reg_VBUS_VoltageL,1,&returnDataL,1);
	uint16_t returnData=(returnDataH<<4)|returnDataL;
    Printf("VBUS voltage= %u*1.7mV\n\r ",returnData);
	return returnData;

}
uint16_t axp_t::readVBUSCurrent(){
	uint8_t returnDataH;
	uint8_t returnDataL;
	i2c->WriteRead(axpAddress,&reg_VBUS_CurrentH,1,&returnDataH,1);
	i2c->WriteRead(axpAddress,&reg_VBUS_CurrentL,1,&returnDataL,1);
	uint16_t returnData=(returnDataH<<4)|returnDataL;
    Printf("VBUS current= %u*0.375mA\n\r ",returnData);
	return returnData;

}
uint16_t axp_t::readACINVoltage(){
	uint8_t returnDataH;
	uint8_t returnDataL;
	i2c->WriteRead(axpAddress,&reg_ACIN_VoltageH,1,&returnDataH,1);
	i2c->WriteRead(axpAddress,&reg_ACIN_VoltageL,1,&returnDataL,1);
	uint16_t returnData=(returnDataH<<4)|returnDataL;
    Printf("ACIN voltage= %u*1.7mV\n\r",returnData);
	return returnData;

}
uint16_t axp_t::readACINCurrent(){
	uint8_t returnDataH;
	uint8_t returnDataL;
	i2c->WriteRead(axpAddress,&reg_ACIN_CurrentH,1,&returnDataH,1);
	i2c->WriteRead(axpAddress,&reg_ACIN_CurrentL,1,&returnDataL,1);
	uint16_t returnData=(returnDataH<<4)|returnDataL;
    Printf("ACIN current= %u*0.625mV\n\r",returnData);
	return returnData;

}
uint16_t axp_t::readTemperature(){
	uint8_t returnDataH;
	uint8_t returnDataL;
	i2c->WriteRead(axpAddress,&reg_intTempH,1,&returnDataH,1);
	i2c->WriteRead(axpAddress,&reg_intTempL,1,&returnDataL,1);
	uint16_t returnData=(returnDataH<<4)|returnDataL;
    Printf("internal temp= %u*0.1-144.7\n\r",returnData);
	return returnData;

}
uint16_t axp_t::readBatVoltage(bool sendInfoViaUart){
	uint8_t returnDataH;
	uint8_t returnDataL;
	i2c->WriteRead(axpAddress,&reg_BatVoltageH,1,&returnDataH,1);
	i2c->WriteRead(axpAddress,&reg_BatVoltageL,1,&returnDataL,1);
	uint16_t returnData=(returnDataH<<4)|returnDataL;
	if(sendInfoViaUart) Printf("Battery voltage= %u*1.1mV\n\r",returnData);
	return returnData;

}
uint16_t axp_t::readIPSOUTVoltage(){
	uint8_t returnDataH;
	uint8_t returnDataL;
	i2c->WriteRead(axpAddress,&reg_IPSOutVoltageH,1,&returnDataH,1);
	i2c->WriteRead(axpAddress,&reg_IPSOutVoltageL,1,&returnDataL,1);
	uint16_t returnData=(returnDataH<<4)|returnDataL;
    Printf("Unstable voltage= %u*1.4mV\n\r",returnData);
	return returnData;

}


__noreturn
void axp_t::ITask(){
	while(true){
		uint16_t batCurrentVoltage = axp.readBatVoltage();
		if( ((batCurrentVoltage - batSavedVoltage) > 100) || ((batSavedVoltage - batCurrentVoltage) > 100)){
			batSavedVoltage = batCurrentVoltage;

	        EvtMsg_t Msg(evtAxpBatVoltageChanged);
	        EvtQMain.SendNowOrExit(Msg);
		};
	    chThdSleepMilliseconds(1000);
	}
}






