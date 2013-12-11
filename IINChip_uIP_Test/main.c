////////////////////////////////////////////////////////
////////////////////////////////////////////////////////
////    main.c
////////////////////////////////////////////////////////
////////////////////////////////////////////////////////

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <avr/io.h>

/* Scheduler include files. */
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <semphr.h>

/* serial interface include file. */
#include <lib_serial.h>

#include <spi.h>

/*-----------------------------------------------------------*/

extern void vuIP_Task( void *pvParameters);			//* The task that handles all uIP data. */

static void TaskCharEcho( void *pvParameters);		//* The task that echos characters between serial ports. */

/* Optionally, create a reference to the handle for the serial port, USART0. */
extern xComPortHandle xSerialPort;
/* Optionally, create a reference to the handle for the other serial port, USART1. */
extern xComPortHandle xSerial1Port;

/*-----------------------------------------------------------*/

/* Main program loop */
int16_t main(void) __attribute__((OS_main));

int16_t main(void)
{

    // turn on the serial port for debugging or for other USART reasons.
	xSerialPort = xSerialPortInitMinimal( USART0, 115200, portSERIAL_BUFFER_TX, portSERIAL_BUFFER_RX); //  serial port: USART, WantedBaud, TxQueueLength, RxQueueLength (8n1)

	avrSerialxPrint_P(&xSerialPort, PSTR("\r\n\nHello World!")); // Ok, so we're alive...

#ifdef portW5200
	spiBegin(SDCard); // make sure the Elecrow W5200 SDCard SS line is high, to disable its MISO, and prevent it from disturbing the SPI bus.
#endif

    xTaskCreate(
    	vuIP_Task
		,  (const signed portCHAR *)"uIP Task" // IP task including httpd
		,  1024
		,  NULL
		,  2
		,  NULL); // */

    xTaskCreate(
    	TaskCharEcho
		,  (const signed portCHAR *)"Char Echo Task" // Echo characters
		,  140
		,  NULL
		,  3
		,  NULL); // */

    avrSerialxPrintf_P(&xSerialPort, PSTR("\r\nFree Heap Size: %u\r\n"), xPortGetFreeHeapSize() ); // needs heap_1, heap_2 or heap_4 for this function to succeed.

	vTaskStartScheduler();

	avrSerialxPrint_P(&xSerialPort, PSTR("\r\nGoodbye... no space for idle task!\r\n")); // Doh, so we're dead...
}

/*-----------------------------------------------------------*/
/* Standard Tasks                                            */
/*-----------------------------------------------------------*/

static void TaskCharEcho(void *pvParameters) // Echo characters from one USART port to the other for SIM900 testing
{
    (void) pvParameters;

    portTickType xLastWakeTime;
	/* The xLastWakeTime variable needs to be initialised with the current tick
	count.  Note that this is the only time we access this variable.  From this
	point on xLastWakeTime is managed automatically by the vTaskDelayUntil()
	API function. */
	xLastWakeTime = xTaskGetTickCount();

    uint8_t character;

	DDRB  |= _BV(DDB2) | _BV(DDB3);  // keep the SIM900 NRESET & PERKEY outputs.
	PORTB &= ~(_BV(PORTB2) | _BV(PORTB3)); // keep the SIM900 NRESET & PERKEY low.

	vTaskDelayUntil( &xLastWakeTime, ( 1000 / portTICK_RATE_MS ) );

	PORTB |= _BV(PORTB2);			// keep the SIM900 PERKEY high for reset for one second.
	vTaskDelayUntil( &xLastWakeTime, ( 1000 / portTICK_RATE_MS ) );

	PORTB &= ~_BV(PORTB2);			// keep the SIM900 PERKEY low and wait for being enabled.
	vTaskDelayUntil( &xLastWakeTime, ( 2250 / portTICK_RATE_MS ) );

    // turn on the other serial port for debugging or for communicating with the Arduino GSM Shield SIM900.
	xSerial1Port = xSerialPortInitMinimal( USART1, 115200, portSERIAL_BUFFER_TX, portSERIAL_BUFFER_RX); //  serial port: USART, WantedBaud, TxQueueLength, RxQueueLength (8n1)

    while(1)
    {
		if(xSerialGetChar( &xSerialPort, &character))
			xSerialPutChar( &xSerial1Port, character);

		if(xSerialGetChar( &xSerial1Port, &character))
			xSerialPutChar( &xSerialPort, character);

		vTaskDelay( 1 );
    }

}

/*-----------------------------------------------------------*/

void vApplicationStackOverflowHook( xTaskHandle xTask,
                                    signed portCHAR *pcTaskName )
{
	DDRB  |= _BV(DDB7);
	PORTB |= _BV(PORTB7);       // main (red PB7) LED on. Goldilocks LED on and die.
	while(1);
}

/*-----------------------------------------------------------*/
