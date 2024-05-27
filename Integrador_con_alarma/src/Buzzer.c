#include "Buzzer.h"



void buzzer_Inicio(void){
	Chip_IOCON_PinMux(LPC_IOCON, sBuzz, IOCON_MODE_INACT, IOCON_FUNC0);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO,sBuzz );
	Chip_GPIO_SetPinState(LPC_GPIO,sBuzz, FALSE);
	return;
}

void buzzer_setEstado(uint8_t estado){
	if(estado){
		Chip_GPIO_SetPinState(LPC_GPIO,sBuzz, 1);
	}
	else{
		Chip_GPIO_SetPinState(LPC_GPIO,sBuzz, 0);
	}
}
