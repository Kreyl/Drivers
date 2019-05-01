/*
 * axp.h
 *
 *  Created on: 11 но€б. 2017 г.
 *      Author: nikit
 */

#include "kl_i2c.h"

#ifndef AXP_H_
#define AXP_H_
#include "board.h"
#include "uart.h"
#include "kl_i2c.h"


// ==== Base class ====
class axp_t {
private:
	i2c_t *i2c;

public:
    axp_t() {};
	void init(i2c_t* pi2c);
	uint8_t readStatusRegister();
	void setDCDC3milliVoltage(uint16_t milliVoltage);
	void setLDO2milliVoltage(uint16_t milliVoltage);
	void setLDO4To2500mV();
	void turnOnLDO2();
	void turnOnLDO4();
	void turnOnDCDC3();
	void keyShortStartShortFinish();
	uint16_t readChargeStatus();
	uint16_t readVBUSVoltage();
	uint16_t readVBUSCurrent();
	uint16_t readACINVoltage();
	uint16_t readACINCurrent();
	uint16_t readTemperature();
	uint16_t readBatVoltage(bool sendInfoViaUart = false);
	uint16_t readIPSOUTVoltage();

	uint16_t batSavedVoltage;
    void ITask();

};

extern axp_t axp;

#endif /* AXP_H_ */
