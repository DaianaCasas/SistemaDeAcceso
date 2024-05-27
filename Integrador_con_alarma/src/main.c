#include <Sensor.h>
#include <alarma.h>
#include "lcd.h"
#include "lcd1602_i2c.h"

#include "chip.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/*****************************************************************************
 * Enums, Structs, Defines
 ****************************************************************************/
/* --- Maquina de Estado MAIN ----------------------*/
typedef enum ESTADO_MAQUINA{
	INICIO,
	ESCANEANDO,
	PRESENCIA,
	APERTURA,
	DENEGADO
} ESTADO_MAQ_e;



#define tESTADO 1000 // 1s
#define tPRESENCIA 2 // 2 segundos

/* --- Maquina de Estado Presencia ----------------------*/

#define INTENTO_MAX 3 // Cantidad maxima de intentos
#define USER_LARGO 10 // Cantidad de letras del nombre de usuario
#define userVALIDO 1
#define userINVALIDO 0

typedef enum PRESENCIA_MAQUINA{
	P_ESPERA,
	P_INGRESANDO,
	P_VERIFICANDO,
	P_FIN
} PRESENCIA_MAQ_e;


typedef struct{
	char user[USER_LARGO];
	uint8_t fValido;
}accesoDATA;
/* ------------- Sensor  ----------------------*/
#define tSensor 1000 // 1 seg
#define tSensorEscaneando 2000 //2 segundos
/* ------------- TARJETA SD ----------------------*/
#define tSD 50
/* ------------- Teclado ----------------------*/
#define LEDSTICK 0,22

#define tTeclado 100
#define tTecladoRebote 10
#define LARGO_PALABRA 6
#define PALABRA_RESET "######"
#define qFilas 4
#define qColumnas 4
#define columnaPort 2
uint8_t columnaPins[qColumnas] = {10,11,12,13};
#define ColumnaPin(x) columnaPins[x]
uint8_t filasPorts[qFilas] = {1,1,3,4};
uint8_t filasPins[qFilas] = {21,23,25,28};
#define FilaPort(x) filasPorts[x]
#define FilaPin(x) filasPins[x]
#define FilaPinStatus(x) 1<<FilaPin(x)
char teclas[qFilas][qColumnas] = {
  { 'D','#','0', '*' },
  { 'C','9','8', '7' },
  { 'B','6','5', '4' },
  { 'A','3','2', '1' }
};

/*****************************************************************************
 * Queue Handler
 ****************************************************************************/
xQueueHandle colaMediciones;

xQueueHandle qTecla;
xQueueHandle qTeclado;

xQueueHandle qLCD;

xQueueHandle qAcceso;
/*****************************************************************************
 * Semaphore Handler
 ****************************************************************************/
xSemaphoreHandle sPresencia;
xSemaphoreHandle sControlPresencia;
/*****************************************************************************
 * Interrupt Handler
 ****************************************************************************/
/* Interrupción de TIMER2 */
void TIMER2_IRQHandler(void){

	uint32_t contadorPulso=0;
	static portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
	// Interrupcion por flanco ascendente
	if (Chip_TIMER_CapturePending(LPC_TIMER2, CAP2CH0) & Chip_GPIO_GetPinState(LPC_GPIO,sEcho)){
		Chip_TIMER_Reset(LPC_TIMER2); // Reseteo a 0 Timer y Prescale y los Sincroninzo.
		Chip_TIMER_Enable(LPC_TIMER2); // Habilito el timer para que comience a CONTAR
	}
	// Interrupcion por flanco descendente
	else{
		Chip_TIMER_Reset(LPC_TIMER2); // Reseteo a 0 Timer y Prescale y los Sincroninzo.
		Chip_TIMER_Disable(LPC_TIMER2); // Deshabilito el timer para que se pare

		/* Lee el valor del ADC y lo guarda en la variable dataADC */
		contadorPulso = Chip_TIMER_ReadCapture(LPC_TIMER2, CAP2CH0);
		NVIC_DisableIRQ(TIMER2_IRQn);//Deshabilita la interrupción de TIMER2

		/* Pone el dato en una cola para su procesamiento */
		xQueueSendToBackFromISR(colaMediciones,&contadorPulso,&xHigherPriorityTaskWoken);

		/* Fuerza la ejecución del scheduler para así retornar a una tarea más prioritaria de ser necesario */
		portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
	}
	Chip_TIMER_ClearCapture(LPC_TIMER2,CAP2CH0);

}



/*****************************************************************************
 * Periodic Task Handlers
 ****************************************************************************/

/* -------------Sensor Distancia ----------------------
static void Task_MedirDistancia( void *pvParameters )
{
	portTickType xLastWakeTime;
	xLastWakeTime = xTaskGetTickCount();
	while(1){
		sMedicion_CrearPulso();
		vTaskDelayUntil(&xLastWakeTime,tSensor/portTICK_RATE_MS);
		}
}
*/
/* ------------- Teclado ----------------------*/
static void Task_BarridoTeclado(void *pvParameters){
	uint32_t iFila=0;
	uint32_t iColumna=0;
	char bPress;
	bool teclaPress =FALSE;

	teclado_Inicio();
	portTickType vTecladoTiempo;
	vTecladoTiempo = xTaskGetTickCount();

	while(1){
		// +3V3
		Chip_GPIO_SetPinState(LPC_GPIO,FilaPort(0),FilaPin(0), TRUE);
		Chip_GPIO_SetPinState(LPC_GPIO,FilaPort(1),FilaPin(1), TRUE);
		Chip_GPIO_SetPinState(LPC_GPIO,FilaPort(2),FilaPin(2), TRUE);
		Chip_GPIO_SetPinState(LPC_GPIO,FilaPort(3),FilaPin(3), TRUE);
		for(iFila=0; iFila < qFilas; iFila++){
			// GND
			Chip_GPIO_SetPinState(LPC_GPIO,FilaPort(iFila),FilaPin(iFila), FALSE);
			/* Perioricidad */
			vTaskDelayUntil(&vTecladoTiempo,tTeclado/portTICK_RATE_MS);
			for(iColumna=0; iColumna < qColumnas; iColumna++){
				if (!Chip_GPIO_GetPinState(LPC_GPIO, columnaPort, ColumnaPin(iColumna)) ){
					 /* Antirebote */
					vTaskDelayUntil(&vTecladoTiempo,tTecladoRebote/portTICK_RATE_MS);
					if (!Chip_GPIO_GetPinState(LPC_GPIO, columnaPort, ColumnaPin(iColumna)) ){
									 teclaPress = TRUE;
									 break;
					}
				}
			}
			if(teclaPress){
				break;
			}
			//+3V3
			Chip_GPIO_SetPinState(LPC_GPIO,FilaPort(iFila),FilaPin(iFila), TRUE);
		}
		if(teclaPress){
			teclaPress = FALSE;
			bPress = teclas[iFila][iColumna];
			//+3V3
			Chip_GPIO_SetPinState(LPC_GPIO,FilaPort(iFila),FilaPin(iFila), TRUE);
			Chip_GPIO_SetPinToggle(LPC_GPIO,LEDSTICK);
			xQueueSend(qTecla,&bPress,portMAX_DELAY);

		}
	}
}

/* ------------- LCD ----------------------*/
static void Task_LCD_Mensaje( void *pvParameters){
	lcd_Inicio();
	lcd_init(I2C1);
	lcd_clear();

	portTickType vLCDTime;
	vLCDTime = xTaskGetTickCount();
	mensajeLCD data_INICIO={
			TITULO,
			0,
			"TP TD3"
	};
	lcd_set_cursor(data_INICIO.renglon,5);
	lcd_string(data_INICIO.texto);
	mensajeLCD data_Titulo={
			TITULO,
			1,
			"Sist De Acceso"
	};
	mensajeLCD data;
	lcd_set_cursor(data_Titulo.renglon,0);
	lcd_string(data_Titulo.texto);
	portBASE_TYPE res;

	while(1){
		res = xQueueReceive(qLCD, &data, portMAX_DELAY);
		if (res){

			lcd_clear();
			switch(data.code){
				case TITULO: {
					lcd_set_cursor(data.renglon,0);
					lcd_string(data.texto);
					break;
				}
				case ERRR: {
					lcd_set_cursor(0,0);
					lcd_string("Error");

					lcd_set_cursor(2,0);
					lcd_string(data.texto);
					break;
				}
				case INFO: {
					lcd_set_cursor(0,0);
					lcd_string("Atencion");

					lcd_set_cursor(2,0);
					lcd_string(data.texto);
					break;
				}
				case CONTINUAR:{
					lcd_set_cursor(2,0);
					lcd_string("..presione #..");
					break;
				}
				case ESPERAR:{
					lcd_set_cursor(0,0);
					lcd_string(data.texto);
					lcd_set_cursor(2,0);
					lcd_string("....esperar....");
					break;
				}
				case ACCESO: {
					lcd_set_cursor(0,0);
					lcd_string("Acceso para:");

					lcd_set_cursor(2,0);
					lcd_string(data.texto);
					break;
				}
				default:{
					lcd_set_cursor(data.renglon,0);
					lcd_string(data.texto);
					break;
				}
			}

		}
		// Perioricidad
		vTaskDelayUntil(&vLCDTime,tLCD/portTICK_RATE_MS);
	}
}

/* ------------- TARJETA SD ----------------------*/
static void vTimeSDTask(void *pvParameters)
{
	while (1)
	{
		//memory_clock(); // Esta función es necesaria para la comunicación con la SD
		//vTaskDelay(tSD/portTICK_RATE_MS); //  Se llama a la función periódicamente cada 50mseg
	}
}


/*****************************************************************************
 * Control Task Handlers
 ****************************************************************************/
/* ------------- Teclado ----------------------*/
// arma la palabra completa del usuario
static void Task_ControlTeclado(void *pvParameters){
	uint8_t iPalabra;
	portBASE_TYPE estadoLectura;
	char bPress; // Almacena 1 caracter
	char inUsuario[LARGO_PALABRA]={0};
	bool fSend=FALSE;
	while(1){
		estadoLectura = xQueueReceive(qTecla, &bPress,portMAX_DELAY);
		if(estadoLectura){
			if( bPress == '#'){
				memcpy(inUsuario,PALABRA_RESET, sizeof(char)*LARGO_PALABRA);
				fSend = TRUE;
			}
			if(iPalabra < LARGO_PALABRA) {
				memcpy(inUsuario + iPalabra,&bPress, sizeof(char));
				//inUsuario[iPalabra] = bPress;
				iPalabra ++;
			}
			if(iPalabra == LARGO_PALABRA){
				fSend = TRUE;
			}

			if(fSend){
				// Insercion completa
				fSend = FALSE;
				xQueueSend(qTeclado,inUsuario,portMAX_DELAY);
				iPalabra = 0;
			}
		}// estadoLectura
	}// while
}

static void Task_ControlPresencia(void* pvParameters){
	// ESCANEANDO:
	uint32_t aux_valorDistancia=0;
	uint32_t valorDistancia=0;
	uint32_t controlAlarma=0;
	portBASE_TYPE estadoLectura;

	portTickType vTiempoControl;
	vTiempoControl = xTaskGetTickCount();
	// continuidad
	uint8_t qIntento = 0; // cant de intentos

	// usuario
	accesoDATA usuario = {{0},userINVALIDO};
	memcpy(usuario.user,"Ausente", sizeof(char)* 7);

	uint8_t fFin = 0;
	while(1){
		xSemaphoreTake(sControlPresencia, portMAX_DELAY); // tomo el semaforo
		qIntento = 0;
		fFin = 0;

		while(!fFin){
			sMedicion_CrearPulso();
			vTaskDelayUntil(&vTiempoControl,tSensorEscaneando/portTICK_RATE_MS);

			estadoLectura = xQueueReceive(colaMediciones, &aux_valorDistancia,0);
			if (estadoLectura ==  pdPASS){
				valorDistancia = sMedicion_medir(aux_valorDistancia);
				Alarmas_Control(TRUE);
				if ( Alarmas_Presencia(valorDistancia) ) {
					qIntento = 0;
				}
				else{
					qIntento ++;
				}
			} //estadoLectura
			if(qIntento == INTENTO_MAX){
				fFin=1;
				Alarmas_Control(FALSE);
				xQueueSendToBack(qAcceso, &usuario, 0); // envio
			}
		}// while(~Fin)
	}//while(1)
}

/*****************************************************************************
 * Maquina de estados
 ****************************************************************************/

static void Task_Main(void *pvParameters){
	// inicio
	ESTADO_MAQ_e estado = INICIO;
	mensajeLCD data1={TITULO,0,"Bienvenido"};
	// para el incio:

	// ESCANEANDO:
	uint32_t aux_valorDistancia=0;
	uint32_t valorDistancia=0;
	uint32_t controlAlarma=0;
	portBASE_TYPE estadoLectura;

	// PRESENCIA
	accesoDATA usuario;

	// APERTURA
	mensajeLCD data2={ACCESO,0,""};

	mensajeLCD data3={ERRR,0,"ID INVALIDO"};

	// Tiempo estado
	portTickType vTiempoESTADO;
	vTiempoESTADO = xTaskGetTickCount();

	// Tarjeta SD
	//disk_init(); // inicio
	while(1){
		switch(estado){
			case INICIO:{
				vTaskDelay(tESTADO/portTICK_RATE_MS);
				estado = ESCANEANDO;
				xQueueSendToBack(qLCD, &data1, portMAX_DELAY);
				break;
			}
			case ESCANEANDO:{
				sMedicion_CrearPulso();
				vTaskDelayUntil(&vTiempoESTADO,tSensor/portTICK_RATE_MS);
				estadoLectura = xQueueReceive(colaMediciones, &aux_valorDistancia,0);
				if (estadoLectura ==  pdPASS){
					valorDistancia = sMedicion_medir(aux_valorDistancia);
					if ( Alarmas_Presencia(valorDistancia) ) {
						if( controlAlarma == tPRESENCIA) {
							controlAlarma = 0; // reinicio
							estado = PRESENCIA;
						}
						controlAlarma ++;
					}
					else{
						controlAlarma = 0; // reinicio
					}

				}
				break;
			}
			case PRESENCIA:{
				xSemaphoreGive(sPresencia);
				xSemaphoreGive(sControlPresencia);
				estadoLectura = xQueueReceive(qAcceso, &usuario, portMAX_DELAY); // recibo
				if (estadoLectura ==  pdPASS){

					if(usuario.fValido == userVALIDO){
						estado = APERTURA;
						//memcpy(data2.texto, "Daiana C",sizeof(char)*8);
						xQueueSendToBack(qLCD, &data2, portMAX_DELAY);

					}
					else{
						//memcpy(data3.texto, "ID invalido",sizeof(char) * 11);
						xQueueSendToBack(qLCD, &data3, portMAX_DELAY);
						estado = DENEGADO;
					}
				}
				break;
			}
			case APERTURA:{

				break;
			}
			case DENEGADO:{

				break;
			}

		}


	}
}


static void Task_Presencia(void* pvParameters){
	PRESENCIA_MAQ_e estado=P_ESPERA;
	accesoDATA usuario = {{0},userINVALIDO};

	portBASE_TYPE estadoLectura;

	// P_ESPERA
	mensajeLCD data1={ESPERAR,0,"Presencia activa"};

	// P_INGRESANDO
	mensajeLCD data2={INFO,0,"Ingrese ID: ..."};
	mensajeLCD data3={INFO,0,"Reingrese ID: ..."};
	portBASE_TYPE estadoTeclado;
	char userIn[LARGO_PALABRA]={0};

	// P_VERIFICANDO
	mensajeLCD data4={ESPERAR,0," Verificando ID "};
	mensajeLCD data5={ ERRR,0," Max intentos "};

	// continuidad
	uint8_t qIntento = 0; // cant de intentos
	uint8_t fFin = 0;


	while(1){
		xSemaphoreTake(sPresencia, portMAX_DELAY); // tomo el semaforo

		xQueueSendToBack(qLCD, &data1, 0);
		vTaskDelay(tESTADO/portTICK_RATE_MS);
		fFin = 0;
		qIntento = 0;

		while(!fFin){
			switch(estado){
				case P_ESPERA:{
					estado = P_INGRESANDO;
					xQueueSendToBack(qLCD, &data2, 0);
					vTaskDelay(tESTADO/portTICK_RATE_MS);
					xQueueReset(qTeclado); // limpio la cola de cualquier ingreso
					break;
				}
				case P_INGRESANDO:{
					estadoTeclado = xQueueReceive(qTeclado,userIn,portMAX_DELAY);
					if(estadoTeclado){
						if(userIn[0] == "#"){
							xQueueSendToBack(qLCD, &data3, 0);
							qIntento++;
						}
						else{
							xQueueSendToBack(qLCD, &data4, 0);
							estado = P_VERIFICANDO;
						}
					}
					if(qIntento == INTENTO_MAX){
						xQueueSendToBack(qLCD, &data5, 0);
						estado = P_FIN;
					}
					break;
				}
				case P_VERIFICANDO:{
					// buscar userIn vs data en SD:
					/*readFileUser(&myUser);
					 * if( disk_ValidacionUsuario(myUser, userIn)){
					 * }
					 * else{
					 * 	// TO DO : llenar estructura usuario
					 * }
					 * estado = P_FIN;
					*/
					vTaskDelay(tESTADO/portTICK_RATE_MS);
					estado = P_FIN;
					break;
				}
				case P_FIN:{
					usuario.fValido = userVALIDO;
					memcpy(usuario.user, "Daiana", sizeof(char) * 6);
					qIntento = 0;
					fFin = 1;
					break;
				}
			} //switch
		} // while(~fFin)
		if(fFin){
			xQueueSendToBack(qAcceso, &usuario, 0); // envio
		}
	} // while(1)
}

/*****************************************************************************
 * Initialization
 ****************************************************************************/
void teclado_Inicio(void){
	/*  ------------------ PULSADOR FILA ---------------- */
	// Salida
	Chip_IOCON_PinMux(LPC_IOCON,FilaPort(0),FilaPin(0),IOCON_MODE_INACT, IOCON_FUNC0);
	Chip_IOCON_PinMux(LPC_IOCON,FilaPort(1),FilaPin(1),IOCON_MODE_INACT, IOCON_FUNC0);
	Chip_IOCON_PinMux(LPC_IOCON,FilaPort(2),FilaPin(2),IOCON_MODE_INACT, IOCON_FUNC0);
	Chip_IOCON_PinMux(LPC_IOCON,FilaPort(3),FilaPin(3),IOCON_MODE_INACT, IOCON_FUNC0);

	Chip_GPIO_SetPinDIROutput(LPC_GPIO,FilaPort(0),FilaPin(0));
	Chip_GPIO_SetPinDIROutput(LPC_GPIO,FilaPort(1),FilaPin(1));
	Chip_GPIO_SetPinDIROutput(LPC_GPIO,FilaPort(2),FilaPin(2));
	Chip_GPIO_SetPinDIROutput(LPC_GPIO,FilaPort(3),FilaPin(3));
	// +3V3
	Chip_GPIO_SetPinState(LPC_GPIO,FilaPort(0),FilaPin(0), TRUE);
	Chip_GPIO_SetPinState(LPC_GPIO,FilaPort(1),FilaPin(1), TRUE);
	Chip_GPIO_SetPinState(LPC_GPIO,FilaPort(2),FilaPin(2), TRUE);
	Chip_GPIO_SetPinState(LPC_GPIO,FilaPort(3),FilaPin(3), TRUE);

	/*  ------------------ PULSADOR COLUMNA ---------------- */
	/* Defino el pin como entrada */
	// +3V3
	Chip_IOCON_PinMux(LPC_IOCON,columnaPort,ColumnaPin(0),IOCON_MODE_PULLUP, IOCON_FUNC0);
	Chip_IOCON_PinMux(LPC_IOCON,columnaPort,ColumnaPin(1),IOCON_MODE_PULLUP, IOCON_FUNC0);
	Chip_IOCON_PinMux(LPC_IOCON,columnaPort,ColumnaPin(2),IOCON_MODE_PULLUP, IOCON_FUNC0);
	Chip_IOCON_PinMux(LPC_IOCON,columnaPort,ColumnaPin(3),IOCON_MODE_PULLUP, IOCON_FUNC0);

	Chip_GPIO_SetPinDIRInput(LPC_GPIO,columnaPort,ColumnaPin(0));
	Chip_GPIO_SetPinDIRInput(LPC_GPIO,columnaPort,ColumnaPin(1));
	Chip_GPIO_SetPinDIRInput(LPC_GPIO,columnaPort,ColumnaPin(2));
	Chip_GPIO_SetPinDIRInput(LPC_GPIO,columnaPort,ColumnaPin(3));
/*  ------------------ LED ---------------- */
	/* LED: Defino el pin como salida */
	Chip_GPIO_SetPinDIROutput(LPC_GPIO,LEDSTICK);
	/* LED: Defino el estado del LED en OFF */
	Chip_GPIO_SetPinState(LPC_GPIO,LEDSTICK, false);
	return ;
}

void Configuracion_inicio_uC(void){

	/* -------------pines GPIO ----------------------*/
	Chip_GPIO_Init(LPC_GPIO);

	/* ------------- Alarma ----------------------*/
	Alarmas_Inicio();

	/* ------------- Sensor ----------------------*/
	sMedicion_Inicio();
}

/****************************************************************************************************/
/**************************************** MAIN ******************************************************/
/****************************************************************************************************/

int main(void){

	/* Levanta la frecuencia del micro */
	SystemCoreClockUpdate();

	/* -------------Queues ----------------------*/

	/* --- Sensor Distancia ---------*/
	colaMediciones = xQueueCreate(3,sizeof(uint32_t));

	/* --- Teclado ----------------------*/
	qTecla = xQueueCreate(5,sizeof(char));
	qTeclado = xQueueCreate(1,sizeof(char)*LARGO_PALABRA);

	/* --- Display LCD ----------------------*/
	qLCD = xQueueCreate(5,sizeof(mensajeLCD));


	/* --- MAIN ---------*/
	qAcceso = xQueueCreate(1,sizeof(accesoDATA));
	/* -------------Semaphores ----------------------*/


	vSemaphoreCreateBinary(sPresencia);
	vSemaphoreCreateBinary(sControlPresencia);
	xSemaphoreTake(sPresencia, 0);
	xSemaphoreTake(sControlPresencia, 0);
	/* ------------- Initialization ----------------------*/
	Configuracion_inicio_uC();

	/*****************************************************************************
	 * Periodic Task Handlers
	 ****************************************************************************/

	/* --- Sensor Distancia ----------------------
	xTaskCreate(Task_MedirDistancia, (char *) "vMedirDistancia",
    			configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 3UL),
    			(xTaskHandle *) NULL);*/

	/* --- Teclado ----------------------*/
	xTaskCreate(Task_BarridoTeclado, (char *) "vBarridoTeclado",
	    			configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 2UL),
	    			(xTaskHandle *) NULL);

	xTaskCreate(Task_LCD_Mensaje, (char *) "vDisplayRefresco",configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 2UL),(xTaskHandle *) NULL);

	/*****************************************************************************
	 * Control Task Handlers
	 ****************************************************************************/
	xTaskCreate(Task_ControlTeclado, (char *) "vProcesarTeclado",
							configMINIMAL_STACK_SIZE+100, NULL, (tskIDLE_PRIORITY + 1UL),
							(xTaskHandle *) NULL);

	xTaskCreate(Task_ControlPresencia, (char *) "vProcesarPresencia",
								configMINIMAL_STACK_SIZE+100, NULL, (tskIDLE_PRIORITY + 1UL),
								(xTaskHandle *) NULL);
	/*****************************************************************************
	 * Maquina de estados
	 ****************************************************************************/

	xTaskCreate(Task_Main, (char *) "vControlMain",
						configMINIMAL_STACK_SIZE+100, NULL, (tskIDLE_PRIORITY + 1UL),
						(xTaskHandle *) NULL);

	xTaskCreate(Task_Presencia, (char *) "vControlPresencia",
						configMINIMAL_STACK_SIZE+100, NULL, (tskIDLE_PRIORITY + 1UL),
						(xTaskHandle *) NULL);
     /*Inicia el scheduler */
	vTaskStartScheduler();

	/* Nunca debería arribar aquí */

    return 0;
}

