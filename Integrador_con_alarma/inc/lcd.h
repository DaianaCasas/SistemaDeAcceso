/*
 * lcd.h
 *
 *  Created on: 5 jun. 2023
 *      Author: daian
 */

#ifndef LCD_H_
#define LCD_H_

#include "chip.h"


#define I2C0_SDA 0,27
#define I2C0_SCL 0,28

#define I2C1_SDA 0,0
#define I2C1_SCL 0,1

#define I2C2_SDA 0,10
#define I2C2_SCL 0,11


#define CLOCK_RATE_INTERFACE 100000
#define SPEED_100KHZ         100000
#define SPEED_400KHZ         400000
/*		Tiempo LCD		*/
#define tLCD 100


typedef enum {
	TITULO,
	ERRR,
	INFO,
	ACCESO,
	CONTINUAR,
	ESPERAR
} mensajeTipo;


typedef struct{
	mensajeTipo code;
	uint8_t renglon;
	//uint8_t fecha[23];
	char* texto;
} mensajeLCD;
void lcd_Inicio_Pins(uint8_t);
void lcd_Inicio(void);
#endif /* LCD_H_ */
