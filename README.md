# SistemaDeAcceso

_aclaración_ Este proyecto es una porción de un proyecto de seguridad más amplio, que ha sido acotado para su entrega en el ámbito académico.
## FUNDAMENTACIÓN
_PROBLEMÁTICA_
Hoy en día la seguridad es un tema principal del día a día, dentro del mercado se encuentran soluciones generales. Estas soluciones engloban distintas aplicaciones que conlleva un cambio de las estructuras de los hogares y  con altos precios de adquisición.
_SOLUCIÓN_
El Sistema de Acceso, ‘SDA’, tiene por objetivo el anuncio identificado de accesos a la vivienda de una familia, aquellos accesos no identificados se consideran involuntarios por lo cual es un disparador de tareas de aviso. Este sistema está enfocado en instalarse en las entradas de los hogares donde se encuentra la puerta de acceso, ver  Fig. 1.

Se plantea un diseño simple y de fácil instalación con compatibilidad a la estructura de los hogares de la mayoría de la población. Enfocado en bajo costo y con piezas de repuestos accesibles. 


El sistema se compone de las siguientes funcionalidades descritas a continuación.

* Detector de presencia, permite identificar la presencia de una persona en la puerta. 
* Identificación de persona, a través de un teclado matricial se insertará la contraseña de apagado de alarma.La contraseña se compone de una clave (una serie de números aleatorios) y número identificador( número asignado a una persona específica). Se utiliza un teclado para insertar la contraseña correspondiente a un usuario, el mismo se registrará en la memoria Se implementa un LCD para visualizar la información.


## SOFTWARE
El proyecto utiliza el sistema operativo FreeRTOS,el cual nos permite realizar priorización de tareas, sincronización de las mismas y nos brinda administración de recursos de forma tal que accedemos en tiempo real a la adquisición de datos.

La adquisición de datos se centra en la medición de distancias. Para detalles ver la sección  7. Sobre el hardware. A su vez toda la información de los usuarios posibles para el acceso exitoso se disponibiliza en una  memoria externa por lo cual dicho acceso debe ser controlado. Para lo que es el acceso se define 3 intentos de inserción de contraseña. Por otra parte, se maneja todo el flujo de visualización de información como el del  sonido como alarma sonora.

Se utilizan semáforos binarios y colas propios del sistema operativos para disparar la ejecución de tareas de control.
Se definen distintas estructuras de datos para el intercambio de información e identificación del usuario, como así se implementa una máquina de estado principal para el flujo del acceso, la cual se complementa con otra máquina de estado la cual se focaliza en el ingreso de contraseña del usuario y su verificación.


El SDA se desarrolla con un microcontrolador LPC1769 la cual posee las entradas necesarias para realizar la aplicación de este proyecto. Se alimenta con 5V y se agrega un botón de reset para reiniciar el sistema de acceso.

 Del microcontrolador se utiliza el TIMER1 y TIMER2 para el manejo de la medición de la distancia, mediante un sensor ultrasónico.  Para el manejo del LCD se utiliza un adaptador que utiliza I2C permitiendo que sea de forma serie el envío de datos hacia el LCD, se le asigna el uso de I2C1 (SDA1, SCL1) con una dirección de periférico 0x27 y se alimenta con 5V.

Para el teclado matricial y la identificación de las teclas seleccionadas se utilizan distintos pines y  puertos GPIO del microcontrolador. Se realiza mediante un barrido de estado de pines para identificar la columna y fila correspondiente a una tecla presionada. 

El almacenamiento de datos como la verificación de usuario se utiliza almacenamiento externo a través de memoria SD y donde se utiliza el protocolo SPI para su lectura y escritura. Se le asigna el SPI0 (SCLK0, SEL0, MISO0 y MOSI0) en modo Master.

Finalmente para el aviso por medio de sonido se utiliza un buzzer y se utiliza un puerto GPIO de salida para encender o apagar este periférico. 

## OBJETIVOS

Detectar presencia a 15 cm como máximo para evitar avisos incorrectos e innecesarios. 
Confirmar usuario y contraseña de al menos 2 personas del lugar a acceder.
Activación de alarma de sonido  en el momento de detección de presencia.
Informar correctamente el flujo del sistema para que el usuario pueda interpretar la forma de acceso y cómo avanzar sobre el mismo.
Almacenar los accesos fallidos y exitosos en una tarjeta SD

### ALCANCE
El sistema de acceso debe tener un flujo claro y pertinente al uso general, como así ser de fácil arreglo y manejo. Se considera que el usuario final es cualquier persona que necesite identificar el acceso a un lugar y tener un sistema de notificaciones sin dificultar el libre acceso de las personas. El equipo debe ser capaz de presenciar a 15 cm como máximo para solicitar acceso como así almacenar los usuarios habilitados para el ingreso.

## SOBRE EL HARDWARE

### MÓDULOS
A continuación se detalla los módulos que significaron un manejo especial en cuanto al hardware. 
_BUZZER_
Para el buzzer se asocia a un pin del microcontrolador para permitir su habilitación y deshabilitación del periférico. Se habilita con 3V+.

_ADAPTADOR SERIE PARA EL LCD_
 Se utiliza un adaptador que nos permite manejar el LCD con el protocolo I2C. Se realiza una interfaz de actualización de mensaje por medio del software para el LCD permitiendo la actualización periódica del mismo. 

_SENSOR DE PROXIMIDAD_
Para el módulo de proximidad se utiliza uno genérico que se basa en medir distancia por medio de ultrasonido. Para el cual primeramente se debe enviar un pulso de inicio de medición y luego contar el tiempo de respuesta. 

_TECLADO MATRICIAL_
El teclado matricial es un conjunto de switches que tiene columnas y filas asociadas para poder identificar la tecla seleccionada.

Se utiliza del microcontrolador 8 pines GPIO , 4 salidas y 4 entradas para identificar la columna y fila  asociada a un  valor de la tecla.

_MEMORIA EXTERNA_	
Para la memoria se utiliza el protocolo SPI 1, configurado en mmc_ssp de las librerías. Se almacena nombre, id y contraseña de cada usuario en un archivo llamado “user” , y por otra parte se almacena ID y el resultado del acceso. 

_PLACA PRINCIPAL_
	
La placa principal y única contiene todo el hardware y módulos, como así el microcontrolador. Posee una fuente de 5 volt y otra de 3,3 volt para alimentar los módulos correspondientes. Tiene botón de encendido y apagado como así también de reseteo del sistema. 
La corriente que maneja es de valor máximo de 1A. 


## SOBRE EL SOFTWARE
### TAREAS
El programa se compone de 7 tareas, la cual se divide en 3 grupos de acuerdo a su funcionalidad. 

El 1er grupo es de tareas temporizadas y periódicas para actualizar el LCD , realizar el barrido del teclado y mantener el clock de la tarjeta SD.
 El 2do grupo es de tareas de control las cuales se activan mediante recursos de sincronización. Se utilizan colas para enviar y recibir estructuras de datos para los siguientes escenarios:
para el LCD:se utiliza una cola para la recepción del mensaje a imprimir utilizando un agregado de tipos de mensajes para generar un formato específico del mensaje a mostrar
para el teclado: se utilizan colas, una para   recepción de teclas presionadas y otra para el envío de la palabra ingresada por el usuario. La cual se compone de un valor finito de valores de teclas ingresadas.
para la presencia de usuario: se utiliza un semáforo para verificar que esté presente el usuario. Luego de 6 segundos de sin presencia se descarta el ingreso de usuario.
para la Tarjeta SD: para la lectura de los usuarios disponibles en el sistema se utiliza un semáforo de habilitación y luego una cola para ingresar cada usuario encontrado y leído. Por otra parte se utiliza una cola para recibir los logs a ingresar en el sistema


El 3er grupo se compone de las máquinas de estado del flujo del sistema de acceso. 
La máquina de estado central se encarga de detectar presencia, habilitar el teclado , habilitar la verificación de presencia e informar el resultado del flujo. 
Por otra parte, la máquina de estado de presencia, la cual es activada por la máquina de estados central, se encarga de la lectura del teclado y verificación de contraseña como también verificar el ingreso máximos de intentos.

El software a su vez se compone de las funciones de interrupciones de los periféricos que se utilizan, a continuación se listan:
-Timer: permite medir la distancia del sensor de proximidad.
Para mayor detalle ir a 11.1 Anexo 1 - Diagrama de Tareas.

### CONCLUSIONES
El sistema actualmente se encuentra en funcionamiento. El flujo desarrollado permite extender su funcionalidad a nuevos periféricos, como por ejemplo comunicación inalámbrica para el envío de notificaciones, accionar luz en caso de presencia, entre otros. Así mismo se encuentran mejoras, tal como extender el logueo de usuario en el formato que se desee e incorporando memoria interna para mantener los datos en la memoria RAM del microcontrolador y agilizar el sistema. 
Cabe destacar que el sistema desarrollado busca ser parte de un sistema central capaz de generar un sistema de seguridad completo. El equipo desarrollado es útil cuando se trata de avisar accesos y notificar presencia sin embargo carece de accionamiento de apertura a hogares. Se considera que a esa funcionalidad particular se encargará un sistema de seguridad central. 
Dentro del desarrollo del equipo y el software del sistema se concluye que es de importancia contar con un diseño de placa PCB acorde al funcionamiento del sistema. Los test points ubicados en las señales de comunicación de los periféricos permitieron entender su funcionamiento y por lo tanto crear un programa acorde al objetivo del proyecto presente. 
Por otra parte, es fundamental conocer , entender y aplicar los conocimientos de FreeRTOS y sus recursos, para crear un software robusto, funcional y reutilizable. 
Finalmente, se concluye con un sistema de acceso que logra medir distancia, identificar presencia a 15cm, activar la alarma sonora y así mismo el LCD informa el flujo del sistema.
