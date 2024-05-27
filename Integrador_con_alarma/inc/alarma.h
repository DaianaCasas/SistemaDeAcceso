#ifndef ALARMA_H_
#define ALARMA_H_

#include "chip.h"
#include "Buzzer.h"

#define Amin_mm 45
#define Amax_mm 150


//#define Amin 2,0 // P2[0]
//#define Amax 2,2 // P2[2]

#define aLed 2,2

typedef enum{
	A_min,
	A_max,
	A_normal
} alarma;


void Alarmas_Inicio(void);


void Alarmas_Control(uint32_t estado);


alarma Alarmas_Comparar(uint32_t);

bool Alarmas_Presencia(uint32_t valor);

#endif /* ALARMA_H_ */
