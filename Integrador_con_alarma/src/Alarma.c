#include "alarma.h"

void Alarmas_Inicio(void){
	buzzer_Inicio();
	Chip_GPIO_SetPinDIROutput(LPC_GPIO,aLed );
	Chip_GPIO_SetPinState(LPC_GPIO,aLed, true);
}


void Alarmas_Control(uint32_t estado) {

	if(estado){
		Chip_GPIO_SetPinToggle(LPC_GPIO,aLed);
		//Chip_GPIO_SetPinToggle(LPC_GPIO,sBuzz); // Descomentar
	}
	else{
		Chip_GPIO_SetPinState(LPC_GPIO,aLed, true);
		Chip_GPIO_SetPinState(LPC_GPIO,sBuzz, false);
	}
	return ;
}


alarma Alarmas_Comparar(uint32_t valor){
	alarma estado;

	if(Amax_mm < valor) {
		estado = A_normal; // no hay nadie
	}
	else{
		estado = A_min; // hay alguien
	}
	return estado;
}

bool Alarmas_Presencia(uint32_t valor){
	bool estado;

	if(Amax_mm < valor) {
		estado = false;
	}
	else{
		estado = true;
	}
	return estado;
}
