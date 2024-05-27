/*
 * lcd.c
 *
 *  Created on: 5 jun. 2023
 *      Author: daian
 *
 *

/***************************************************************************************************
                                   2x16 LCD internal structure
****************************************************************************************************

            |<--------------------16 chars on Each Line-------------------->|
           ____________________________________________________________________
          |\                                                                  /|
          | \ ___ ___ ___ ___ ___ ___ ___ ___ ___ ___ ___ ___ ___ ___ ___ ___/ |
          |  |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   | |
    Line0 |  | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B | C | D | E | F | |
          |  |___|___|___|___|___|___|___|___|___|___|___|___|___|___|___|___| |
          |  |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   | |
    Line1 |  | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B | C | D | E | F | |
          |  |___|___|___|___|___|___|___|___|___|___|___|___|___|___|___|___| |
          | /                                                                \ |
          |/__________________________________________________________________\|
                        | D7 D6 D5 D4 D3 D2 D1 D0 |     EN   RW   RS
                        |<------Lcd Data Bus----->|    Lcd control Lines


 La dirección I2C por defecto del módulo es 0x3F o 0x27.
 La dirección 0x3F en binario sería: 0|0|1|1|1|A2|A1|A0 y la dirección 0x27: 0|0|1|0|0|A2|A1|A0.

 */


#include "lcd.h"

void lcd_Inicio_Pins(uint8_t i2c_num){
	if (i2c_num){
	/* Configuro la funcion del pin */
		/* I2CO: ver en manual */
		if (i2c_num == 0 ){
			Chip_IOCON_PinMux(LPC_IOCON, I2C0_SDA, IOCON_MODE_INACT, IOCON_FUNC1);
			Chip_IOCON_PinMux(LPC_IOCON, I2C0_SCL, IOCON_MODE_INACT, IOCON_FUNC1);
			//Chip_IOCON_EnableOD(LPC_IOCON, I2C0_SDA);
			//Chip_IOCON_EnableOD(LPC_IOCON, I2C0_SCL);

			/*  Configuro el Fast mode */
			Chip_IOCON_SetI2CPad(LPC_IOCON, I2CPADCFG_STD_MODE);
		}
		/* I2C1 */
		else if (i2c_num == 1){
			Chip_IOCON_PinMux(LPC_IOCON, I2C1_SDA, IOCON_MODE_INACT, IOCON_FUNC3);
			Chip_IOCON_PinMux(LPC_IOCON, I2C1_SCL, IOCON_MODE_INACT, IOCON_FUNC3);
			Chip_IOCON_EnableOD(LPC_IOCON, I2C1_SDA);
			Chip_IOCON_EnableOD(LPC_IOCON, I2C1_SCL);
		}
		/* I2C2 */
		else if (i2c_num == 2){
			Chip_IOCON_PinMux(LPC_IOCON, I2C2_SDA, IOCON_MODE_INACT, IOCON_FUNC2);
			Chip_IOCON_PinMux(LPC_IOCON, I2C2_SCL, IOCON_MODE_INACT, IOCON_FUNC2);
			Chip_IOCON_EnableOD(LPC_IOCON, I2C2_SDA);
			Chip_IOCON_EnableOD(LPC_IOCON, I2C2_SCL);
		}

	}
}


void lcd_Inicio(void){
		/* I2C1 : inicializo los pines como funcion I2C */
		lcd_Inicio_Pins(1);
		/* I2C2: Configuracion */
		Chip_I2C_Init(I2C1);
		/* I2C2: configuro clock */
		Chip_I2C_SetClockRate(I2C1, CLOCK_RATE_INTERFACE);
		/* I2C2: Desactivo NVIC */
		NVIC_DisableIRQ(I2C1_IRQn);
		/* I2C2: Set the master event handler, Set function that must handle I2C events */
		Chip_I2C_SetMasterEventHandler(I2C1, Chip_I2C_EventHandlerPolling);

		Chip_IOCON_PinMuxSet(LPC_IOCON,I2C1_SDA,3);
		Chip_IOCON_PinMuxSet(LPC_IOCON,I2C1_SCL,3);
	}

