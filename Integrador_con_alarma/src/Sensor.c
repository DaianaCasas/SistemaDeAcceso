#include "Sensor.h"


void sMedicion_Inicio(void){
	//####### Pin de Trigger #########
	Chip_IOCON_PinMux(LPC_IOCON, sTrigg, IOCON_MODE_INACT, IOCON_FUNC3);
	Chip_IOCON_EnableOD(LPC_IOCON, sTrigg);


	//####### Timer3 para el Trigger #########
	Chip_TIMER_Init(LPC_TIMER3); // Inicio el Timer 3
	Chip_TIMER_MatchEnableInt(LPC_TIMER3, TIM3CH0); // Lo configuro para que utilice el MR0.
	Chip_TIMER_SetMatch(LPC_TIMER3, 0, sTrigg_TimePulse); // Configuro el pulso
	Chip_TIMER_ExtMatchControlSet(LPC_TIMER3, sSTATUS_LOW,TIMER_EXTMATCH_CLEAR , TIM3CH0); //Seteo el control del pin, lo dejo en LOW

	Chip_TIMER_Disable(LPC_TIMER3); // Deshabilito el timer para que aun no cuente
	Chip_TIMER_MatchDisableInt(LPC_TIMER3, TIM3CH0); // Deshabilito la interrupcion por MATCH

	//####### Pin de Echo #########
	Chip_IOCON_PinMux(LPC_IOCON, sEcho, IOCON_MODE_INACT, IOCON_FUNC3);

	//####### Timer2 para el Echo #########
	Chip_TIMER_Init(LPC_TIMER2); // Inicio el Timer 2
	Chip_TIMER_CaptureRisingEdgeEnable(LPC_TIMER2,CAP2CH0);
	Chip_TIMER_CaptureFallingEdgeEnable(LPC_TIMER2,CAP2CH0);

	NVIC_DisableIRQ(TIMER2_IRQn);//Habilita la interrupción de TIMER2
	Chip_TIMER_CaptureDisableInt(LPC_TIMER2,CAP2CH0);
	Chip_TIMER_Disable(LPC_TIMER2); // Deshabilito el timer para que aun no cuente

}


void sMedicion_CrearPulso(void){

	NVIC_EnableIRQ(TIMER2_IRQn);//Habilita la interrupción de TIMER2
	Chip_TIMER_Enable(LPC_TIMER2); // Habilito el timer para que comience a ver el CAPTURE
	Chip_TIMER_CaptureEnableInt(LPC_TIMER2,CAP2CH0);

	// Crear el pulso
	Chip_TIMER_Reset(LPC_TIMER3); // Reseteo a 0 Timer y Prescale y los Sincroninzo.
	Chip_TIMER_ExtMatchControlSet(LPC_TIMER3,sSTATUS_HIGH,TIMER_EXTMATCH_TOGGLE, TIM3CH0);// Seteo el pin en HIGH para cuando termine de contar lo cambie a LOW (toggle)
	Chip_TIMER_MatchEnableInt(LPC_TIMER3, TIM3CH0);
	Chip_TIMER_Enable(LPC_TIMER3); // Habilito el timer para que comience a CONTAR

}


uint32_t sMedicion_medir(uint32_t ticks){
	/*
	 * Resolucion de 3mm
	 *
	 * Distancia minima: 20mm
	 * Distancia maxima: 4000mm
	 * */
	float VELOCIDAD_mm_useg=0.3432; // 343,2 m/1 s

	uint32_t aux;
	float frec_second;
	float frec_usecond;
	float time_usecond;

	if (Chip_TIMER_ReadPrescale(LPC_TIMER3)){
		frec_second = Chip_Clock_GetPeripheralClockRate(SYSCTL_PCLK_TIMER3)/Chip_TIMER_ReadPrescale(LPC_TIMER3);
	}
	else{
		frec_second = Chip_Clock_GetPeripheralClockRate(SYSCTL_PCLK_TIMER3);
	}
	frec_usecond =  (frec_second) / 1E6;

	time_usecond = (ticks/frec_usecond) + 5; // ticks * Tusecond, sumo 5useg de error.

	aux = ( time_usecond * VELOCIDAD_mm_useg )/2;

	return aux; // milimetros

}
