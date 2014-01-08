/*
    FreeRTOS V7.0.1 - Copyright (C) 2011 Real Time Engineers Ltd.


    ***************************************************************************
     *                                                                       *
     *    FreeRTOS tutorial books are available in pdf and paperback.        *
     *    Complete, revised, and edited pdf reference manuals are also       *
     *    available.                                                         *
     *                                                                       *
     *    Purchasing FreeRTOS documentation will not only help you, by       *
     *    ensuring you get running as quickly as possible and with an        *
     *    in-depth knowledge of how to use FreeRTOS, it will also help       *
     *    the FreeRTOS project to continue with its mission of providing     *
     *    professional grade, cross platform, de facto standard solutions    *
     *    for microcontrollers - completely free of charge!                  *
     *                                                                       *
     *    >>> See http://www.FreeRTOS.org/Documentation for details. <<<     *
     *                                                                       *
     *    Thank you for using FreeRTOS, and thank you for your support!      *
     *                                                                       *
    ***************************************************************************


    This file is part of the FreeRTOS distribution.

    FreeRTOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License (version 2) as published by the
    Free Software Foundation AND MODIFIED BY the FreeRTOS exception.
    >>>NOTE<<< The modification to the GPL is included to allow you to
    distribute a combined work that includes FreeRTOS without being obliged to
    provide the source code for proprietary components outside of the FreeRTOS
    kernel.  FreeRTOS is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
    or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
    more details. You should have received a copy of the GNU General Public
    License and the FreeRTOS license exception along with FreeRTOS; if not it
    can be viewed here: http://www.freertos.org/a00114.html and also obtained
    by writing to Richard Barry, contact details for whom are available on the
    FreeRTOS WEB site.

    1 tab == 4 spaces!

    http://www.FreeRTOS.org - Documentation, latest information, license and
    contact details.

    http://www.SafeRTOS.com - A version that is certified for use in safety
    critical systems.

    http://www.OpenRTOS.com - Commercial support, development, porting,
    licensing and training services.
*/

/* BASIC INTERRUPT DRIVEN SERIAL PORT DRIVER. */
/* Also with polling serial functions, for use before scheduler is enabled */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include <util/delay.h>

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>

#include <ringBuffer.h>

#include <lib_serial.h>


/*-----------------------------------------------------------*/

#define vInterrupt0_On()									\
{															\
	uint8_t ucByte;								    		\
															\
	ucByte  = UCSR0B;										\
	ucByte |= _BV(UDRIE0);									\
	UCSR0B  = ucByte;										\
}


#define vInterrupt0_Off()									\
{															\
	uint8_t ucByte;								   			\
															\
	ucByte  = UCSR0B;										\
	ucByte &= ~_BV(UDRIE0);									\
	UCSR0B  = ucByte;										\
}

#if defined(__AVR_ATmega324P__)  || defined(__AVR_ATmega644P__)|| defined(__AVR_ATmega1284P__) || defined(__AVR_ATmega324PA__) || defined(__AVR_ATmega644PA__) ||defined(__AVR_ATmega640__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega1281__) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega2561__)

#define vInterrupt1_On()									\
{															\
	uint8_t ucByte;								    		\
															\
	ucByte  = UCSR1B;										\
	ucByte |= _BV(UDRIE1);									\
	UCSR1B  = ucByte;										\
}


#define vInterrupt1_Off()									\
{															\
	uint8_t ucByte;								   			\
															\
	ucByte  = UCSR1B;										\
	ucByte &= ~_BV(UDRIE1);									\
	UCSR1B  = ucByte;										\
}

#else

#define vInterrupt1_On()
#define vInterrupt1_Off()

#endif

#if  defined(__AVR_ATmega640__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega1281__) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega2561__)
#define vInterrupt2_On()									\
{															\
	uint8_t ucByte;								    		\
															\
	ucByte  = UCSR2B;										\
	ucByte |= _BV(UDRIE2);									\
	UCSR2B  = ucByte;										\
}

#define vInterrupt2_Off()									\
{															\
	uint8_t ucByte;								   			\
															\
	ucByte  = UCSR2B;										\
	ucByte &= ~_BV(UDRIE2);									\
	UCSR2B  = ucByte;										\
}

#define vInterrupt3_On()									\
{															\
	uint8_t ucByte;								    		\
															\
	ucByte  = UCSR3B;										\
	ucByte |= _BV(UDRIE3);									\
	UCSR3B  = ucByte;										\
}

#define vInterrupt3_Off()									\
{															\
	uint8_t ucByte;								   			\
															\
	ucByte  = UCSR3B;										\
	ucByte &= ~_BV(UDRIE3);									\
	UCSR3B  = ucByte;										\
}

#else

#define vInterrupt2_On()
#define vInterrupt2_Off()
#define vInterrupt3_On()
#define vInterrupt3_Off()

#endif

/*-----------------------------------------------------------*/

/* Create a handle for the serial port, USART0. */
/* This variable is special, as it is used in the interrupt */
xComPortHandle xSerialPort;

#if defined(__AVR_ATmega324P__)  || defined(__AVR_ATmega644P__)|| defined(__AVR_ATmega1284P__) || defined(__AVR_ATmega324PA__) || defined(__AVR_ATmega644PA__) ||defined(__AVR_ATmega640__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega1281__) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega2561__)

/* Create a handle for the other serial port, USART1. */
/* This variable is special, as it is used in the interrupt */
xComPortHandle xSerial1Port;

#endif
#if defined(__AVR_ATmega640__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega1281__) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega2561__)
/* Create a handle for the other serial ports, USART2,3. */
/* This variable is special, as it is used in the interrupt */
xComPortHandle xSerial2Port;
xComPortHandle xSerial3Port;
#endif
/*-----------------------------------------------------------------*/

// xSerialPrintf_P(PSTR("\r\nMessage %u %u %u"), var1, var2, var2);

void xSerialPrintf( const char * format, ...)
{
	va_list arg;

	va_start(arg, format);

	while(xSerialPort.serialWorkBufferInUse == ENGAGED ) taskYIELD();
	xSerialPort.serialWorkBufferInUse = ENGAGED;

	vsnprintf((char *)(xSerialPort.serialWorkBuffer), xSerialPort.serialWorkBufferSize, (const char *)format, arg);
	xSerialPrint((uint8_t *)(xSerialPort.serialWorkBuffer));

	xSerialPort.serialWorkBufferInUse = VACANT;

	va_end(arg);
}

void xSerialPrintf_P(PGM_P format, ...)
{
	va_list arg;

	va_start(arg, format);

	while(xSerialPort.serialWorkBufferInUse == ENGAGED ) taskYIELD();
	xSerialPort.serialWorkBufferInUse = ENGAGED;

	vsnprintf_P((char *)(xSerialPort.serialWorkBuffer), xSerialPort.serialWorkBufferSize, format, arg);
	xSerialPrint((uint8_t *)(xSerialPort.serialWorkBuffer));

	xSerialPort.serialWorkBufferInUse = VACANT;

	va_end(arg);
}

void xSerialPrint( uint8_t * str)
{
	int16_t i = 0;
	size_t stringlength;

	stringlength = strlen((char *)str);

	while(i < stringlength)
		xSerialPutChar( &xSerialPort, str[i++] );
}

void xSerialPrint_P(PGM_P str)
{
	uint16_t i = 0;
	size_t stringlength;

	stringlength = strlen_P(str);

	while(i < stringlength)
		xSerialPutChar( &xSerialPort, pgm_read_byte(&str[i++]) );
}
/*-----------------------------------------------------------*/


// These can be set to use any USART

void xSerialxPrintf( xComPortHandlePtr pxPort, const char * format, ...)
{
	va_list arg;

	va_start(arg, format);

	while(pxPort->serialWorkBufferInUse == ENGAGED ) taskYIELD();
	pxPort->serialWorkBufferInUse = ENGAGED;

	vsnprintf((char *)(pxPort->serialWorkBuffer), pxPort->serialWorkBufferSize, (const char *)format, arg);
	xSerialxPrint(pxPort, (uint8_t *)(pxPort->serialWorkBuffer));

	pxPort->serialWorkBufferInUse = VACANT;

	va_end(arg);
}

void xSerialxPrintf_P( xComPortHandlePtr pxPort, PGM_P format, ...)
{
	va_list arg;

	va_start(arg, format);

	while(pxPort->serialWorkBufferInUse == ENGAGED ) taskYIELD();
	pxPort->serialWorkBufferInUse = ENGAGED;

	vsnprintf_P((char *)(pxPort->serialWorkBuffer), pxPort->serialWorkBufferSize, format, arg);
	xSerialxPrint(pxPort, (uint8_t *)(pxPort->serialWorkBuffer));

	pxPort->serialWorkBufferInUse = VACANT;

	va_end(arg);
}

void xSerialxPrint( xComPortHandlePtr pxPort, uint8_t * str)
{
	int16_t i = 0;
	size_t stringlength;

	stringlength = strlen((char *)str);

	while(i < stringlength)
		xSerialPutChar( pxPort, str[i++]);
}

void xSerialxPrint_P( xComPortHandlePtr pxPort, PGM_P str)
{
	uint16_t i = 0;
	size_t stringlength;

	stringlength = strlen_P(str);

	while(i < stringlength)
		xSerialPutChar( pxPort, pgm_read_byte(&str[i++]) );
}
/*-----------------------------------------------------------*/

inline void xSerialFlush( xComPortHandlePtr pxPort )
{
	/* Flush received characters from the serial port buffer.*/

	uint8_t byte;

	switch (pxPort->usart)
	{
	case USART0:
		while ( UCSR0A & (1<<RXC0) )
			byte = UDR0;
		break;

	case USART1:
#if defined(__AVR_ATmega324P__)  || defined(__AVR_ATmega644P__)|| defined(__AVR_ATmega1284P__) || defined(__AVR_ATmega324PA__) || defined(__AVR_ATmega644PA__) ||defined(__AVR_ATmega640__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega1281__) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega2561__)
		while ( UCSR1A & (1<<RXC1) )
			byte = UDR1;
		break;
#endif

	case USART2:
#if  defined(__AVR_ATmega640__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega1281__) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega2561__)
		while ( UCSR2A & (1<<RXC2) )
			byte = UDR2;
		break;
#endif

	case USART3:
#if  defined(__AVR_ATmega640__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega1281__) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega2561__)
		while ( UCSR3A & (1<<RXC3) )
			byte = UDR3;
		break;
#endif
	default:
		break;
	}

	ringBuffer_Flush( &(pxPort->xRxedChars) );
}

inline uint16_t xSerialAvailableChar( xComPortHandlePtr pxPort )
{
	/* Are characters available in the serial port buffer.*/

	return ringBuffer_GetCount( &(pxPort->xRxedChars) );
}

inline portBASE_TYPE xSerialGetChar( xComPortHandlePtr pxPort, unsigned portBASE_TYPE *pcRxedChar )
{
	/* Get the next character from the ring buffer.  Return false if no characters are available */

	if( ringBuffer_IsEmpty( &(pxPort->xRxedChars) ) )
	{
		return pdFALSE;
	}
	else
	{
		* pcRxedChar = ringBuffer_Pop( &(pxPort->xRxedChars) );
		return pdTRUE;
	}
}

inline portBASE_TYPE xSerialPutChar( xComPortHandlePtr pxPort, unsigned const portBASE_TYPE cOutChar )
{
	/* Return false if there remains no room on the Tx ring buffer */
	if( ! ringBuffer_IsFull( &(pxPort->xCharsForTx) ) )
		ringBuffer_Poke( &(pxPort->xCharsForTx), cOutChar ); // poke in a fast byte
	else
	{
		 // go slower, per character rate for 115200 is 86us
		_delay_us(100); // delay for about one character (maximum _delay_loop_1() delay is 32 us at 22MHz)
		_delay_us(100);

		if( ! ringBuffer_IsFull( &(pxPort->xCharsForTx) ) )
			ringBuffer_Poke( &(pxPort->xCharsForTx), cOutChar ); // poke in a byte slowly
		else
			return pdFAIL; // if the Tx ring buffer remains full
	}
	pxPort->transmitting = true;

	switch (pxPort->usart)
	{
	case USART0:
		vInterrupt0_On();
		UCSR0A |= _BV(TXC0);
		break;

	case USART1:
		vInterrupt1_On();
		UCSR1A |= _BV(TXC1);
		break;

	case USART2:
		vInterrupt2_On();
		UCSR2A |= _BV(TXC2);
		break;

	case USART3:
		vInterrupt3_On();
		UCSR3A |= _BV(TXC3);
		break;

	default:
		break;
	}

	return pdPASS;
}
/*-----------------------------------------------------------*/

xComPortHandle xSerialPortInitMinimal( eCOMPort ePort, uint32_t ulWantedBaud, uint16_t uxTxQueueLength, uint16_t uxRxQueueLength )
{
	uint8_t * dataPtr;

	xComPortHandle newComPort;

	/* Create the ring-buffers used by the serial communications task. */
	if( (dataPtr = (uint8_t *)pvPortMalloc( sizeof(uint8_t) * uxRxQueueLength )))
		ringBuffer_InitBuffer( &(newComPort.xRxedChars), dataPtr, uxRxQueueLength);

	if( (dataPtr = (uint8_t *)pvPortMalloc( sizeof(uint8_t) * uxTxQueueLength )))
		ringBuffer_InitBuffer( &(newComPort.xCharsForTx), dataPtr, uxTxQueueLength);

	// create a working buffer for vsnprintf on the heap (so we can use extended RAM, if available).
	// create the structures on the heap (so they can be moved later).
	if( !(newComPort.serialWorkBuffer = (uint8_t *)pvPortMalloc( sizeof(uint8_t) * uxTxQueueLength )))
		newComPort.serialWorkBuffer = NULL;

	newComPort.usart = ePort; // containing eCOMPort
	newComPort.serialWorkBufferSize = uxTxQueueLength; // size of the working buffer for vsnprintf
	newComPort.serialWorkBufferInUse = VACANT;  // clear the occupation flag.
	newComPort.transmitting = false;

	portENTER_CRITICAL();
	switch (newComPort.usart)
	{
	case USART0:
		/*
		 * Calculate the baud rate register value from the equation in the data sheet. */

		/* As the 16MHz Arduino boards have bad karma for serial port, we're using the 2x clock U2X0 */
		// for Arduino at 16MHz; above data sheet calculation is wrong. Need below from <util/setbaud.h>
		// This provides correct rounding truncation to get closest to correct speed.
		// Normal mode gives 3.7% error, which is too much. Use 2x mode gives 2.1% error.
		// Or, use 22.1184 MHz over clock which gives 0.00% error, for all rates.

		// ulBaudRateCounter = ((configCPU_CLOCK_HZ + ulWantedBaud * 8UL) / (ulWantedBaud * 16UL) - 1); // for normal mode
		// ulBaudRateCounter = (uint16_t)((configCPU_CLOCK_HZ + ulWantedBaud * 4UL) / (ulWantedBaud * 8UL) - 1);  // for 2x mode
		UBRR0 = (uint16_t)((configCPU_CLOCK_HZ + ulWantedBaud * 4UL) / (ulWantedBaud * 8UL) - 1);  // for 2x mode, using 16 bit avr-gcc capability.

		/* Set the 2x speed mode bit */
		UCSR0A = _BV(U2X0);

		/* Enable the Rx and Tx. Also enable the Rx interrupt. The Tx interrupt will get enabled later. */
		UCSR0B = ( _BV(RXCIE0) | _BV(RXEN0) | _BV(TXEN0));

		/* Set the data bit register to 8n1. */
		UCSR0C = ( _BV(UCSZ01) | _BV(UCSZ00) );

		break;

	case USART1:
#if defined(__AVR_ATmega324P__)  || defined(__AVR_ATmega644P__)|| defined(__AVR_ATmega1284P__) || defined(__AVR_ATmega324PA__) || defined(__AVR_ATmega644PA__) ||defined(__AVR_ATmega640__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega1281__) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega2561__)
		PRR1 &= ~ _BV(PRUSART1);
		UBRR1 = (uint16_t)((configCPU_CLOCK_HZ + ulWantedBaud * 4UL) / (ulWantedBaud * 8UL) - 1);
		UCSR1A = _BV(U2X1);
		UCSR1B = ( _BV(RXCIE1) | _BV(RXEN1) | _BV(TXEN1));
		UCSR1C = ( _BV(UCSZ11) | _BV(UCSZ10) );

		break;
#endif

	case USART2:
#if  defined(__AVR_ATmega640__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega1281__) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega2561__)
		PRR1 &= ~ _BV(PRUSART2);
		UBRR2 = (uint16_t)((configCPU_CLOCK_HZ + ulWantedBaud * 4UL) / (ulWantedBaud * 8UL) - 1);
		UCSR2A = _BV(U2X2);
		UCSR2B = ( _BV(RXCIE2) | _BV(RXEN2) | _BV(TXEN2));
		UCSR2C = ( _BV(UCSZ21) | _BV(UCSZ20) );

		break;
#endif
	case USART3:
#if  defined(__AVR_ATmega640__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega1281__) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega2561__)
		PRR1 &= ~ _BV(PRUSART3);
		UBRR3 = (uint16_t)((configCPU_CLOCK_HZ + ulWantedBaud * 4UL) / (ulWantedBaud * 8UL) - 1);
		UCSR3A = _BV(U2X3);
		UCSR3B = ( _BV(RXCIE3) | _BV(RXEN3) | _BV(TXEN3));
		UCSR3C = ( _BV(UCSZ31) | _BV(UCSZ30) );

		break;
#endif
	default:
		break;
	}

	portEXIT_CRITICAL();

	return newComPort;
}


void xSerialFlushTX(xComPortHandlePtr xSerialPort) {
	switch(xSerialPort->usart) {
	case USART0:
		while (xSerialPort->transmitting && !(UCSR0A & _BV(TXC0)));
		break;
	case USART1:
		while (xSerialPort->transmitting && !(UCSR1A & _BV(TXC1)));
		break;
	case USART2:
		while (xSerialPort->transmitting && !(UCSR2A & _BV(TXC2)));
		break;
	case USART3:
		while (xSerialPort->transmitting && !(UCSR3A & _BV(TXC3)));
		break;
	}
	xSerialPort->transmitting = false;

}


void vSerialClose( xComPortHandlePtr oldComPortPtr )
{
	uint8_t ucByte;

	/* Turn off the interrupts.  We may also want to delete the queues and/or
	re-install the original ISR. */

	vPortFree( oldComPortPtr->serialWorkBuffer );
	vPortFree( oldComPortPtr->xRxedChars.start );
	vPortFree( oldComPortPtr->xCharsForTx.start );

	portENTER_CRITICAL();
	{
		switch (oldComPortPtr->usart)
		{
		case USART0:
			vInterrupt0_Off();
			ucByte = UCSR0B;
			ucByte &= ~(_BV(RXCIE0));
			UCSR0B = ucByte;
			break;

		case USART1:
#if defined(__AVR_ATmega324P__)  || defined(__AVR_ATmega644P__)|| defined(__AVR_ATmega1284P__) || defined(__AVR_ATmega324PA__) || defined(__AVR_ATmega644PA__) ||defined(__AVR_ATmega640__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega1281__) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega2561__)
			vInterrupt1_Off();
			ucByte = UCSR1B;
			ucByte &= ~(_BV(RXCIE1));
			UCSR1B = ucByte;
			PRR1 |= _BV(PRUSART1);
			break;
#endif

		case USART2:
#if  defined(__AVR_ATmega640__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega1281__) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega2561__)
			vInterrupt2_Off();
			ucByte = UCSR2B;
			ucByte &= ~(_BV(RXCIE2));
			UCSR2B = ucByte;
			PRR1 |= _BV(PRUSART2);
			break;
#endif
		case USART3:
#if  defined(__AVR_ATmega640__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega1281__) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega2561__)
			vInterrupt3_Off();
			ucByte = UCSR3B;
			ucByte &= ~(_BV(RXCIE3));
			UCSR3B = ucByte;
			PRR1 |= _BV(PRUSART3);
			break;
#endif
		default:
			break;
		}
	}
	portEXIT_CRITICAL();
}

/*-----------------------------------------------------------*/

// polling read and write routines, for use before freeRTOS vTaskStartScheduler (interrupts enabled).
// same as above, but doesn't use interrupts.

void avrSerialPrintf(const char * format, ...)
{
	va_list arg;

	va_start(arg, format);

	while(xSerialPort.serialWorkBufferInUse == ENGAGED ) _delay_us(25);
	xSerialPort.serialWorkBufferInUse = ENGAGED;

	vsnprintf((char *)(xSerialPort.serialWorkBuffer), xSerialPort.serialWorkBufferSize, (const char *)format, arg);
	avrSerialPrint((xSerialPort.serialWorkBuffer));

	xSerialPort.serialWorkBufferInUse = VACANT;

	va_end(arg);
}

void avrSerialPrintf_P(PGM_P format, ...)
{
	va_list arg;

	va_start(arg, format);

	while(xSerialPort.serialWorkBufferInUse == ENGAGED ) _delay_us(25);
	xSerialPort.serialWorkBufferInUse = ENGAGED;

	vsnprintf_P((char *)(xSerialPort.serialWorkBuffer), xSerialPort.serialWorkBufferSize, format, arg);
	avrSerialPrint((xSerialPort.serialWorkBuffer));

	xSerialPort.serialWorkBufferInUse = VACANT;

	va_end(arg);
}

void avrSerialPrint(uint8_t * str)
{
	uint16_t i = 0;
	size_t stringlength;

	stringlength = strlen((char *)str);

	while(i < stringlength)
		avrSerialWrite(str[i++]);
}

void avrSerialPrint_P(PGM_P str)
{
	uint16_t i = 0;
	size_t stringlength;

	stringlength = strlen_P(str);

	while(i < stringlength)
		avrSerialWrite(pgm_read_byte(&str[i++]));
}

inline void avrSerialWrite(int8_t DataOut)
{
	while (!avrSerialCheckTxReady())		// while NOT ready to transmit
        _delay_us(25);     					// delay
	UDR0 = DataOut;
}

inline int8_t avrSerialRead(void)
{

	while (!avrSerialCheckRxComplete())		// While data is NOT available to read
		_delay_us(25);     					// delay
	/* Get status and data */
	/* from buffer */

	/* If error, return 0xFF */
	if ( UCSR0A & (_BV(FE0)|_BV(DOR0)|_BV(UPE0)) )
	return 0xFF;
	else
	return UDR0;
}

inline int8_t avrSerialCheckRxComplete(void)
{
	return( UCSR0A & (_BV(RXC0)) );			// nonzero if serial data is available to read.
}

inline int8_t avrSerialCheckTxReady(void)
{
	return( UCSR0A & (_BV(UDRE0)) );		// nonzero if transmit register is ready to receive new data.
}

/*-----------------------------------------------------------*/

// Polling read and write routines, for use before freeRTOS vTaskStartScheduler (interrupts enabled).
// Same as above, but doesn't use interrupts.
// These can be set to use any USART

void avrSerialxPrintf(xComPortHandlePtr pxPort, const char * format, ...)
{
	va_list arg;

	va_start(arg, format);

	while(pxPort->serialWorkBufferInUse == ENGAGED ) _delay_us(25);
	pxPort->serialWorkBufferInUse = ENGAGED;

	vsnprintf((char *)(pxPort->serialWorkBuffer), pxPort->serialWorkBufferSize, (const char *)format, arg);
	avrSerialxPrint(pxPort, pxPort->serialWorkBuffer);

	pxPort->serialWorkBufferInUse = VACANT;

	va_end(arg);
}

void avrSerialxPrintf_P(xComPortHandlePtr pxPort, PGM_P format, ...)
{
	va_list arg;

	va_start(arg, format);

	while(pxPort->serialWorkBufferInUse == ENGAGED ) _delay_us(25);
	pxPort->serialWorkBufferInUse = ENGAGED;

	vsnprintf_P((char *)(pxPort->serialWorkBuffer), pxPort->serialWorkBufferSize, format, arg);
	avrSerialxPrint(pxPort, pxPort->serialWorkBuffer);

	pxPort->serialWorkBufferInUse = VACANT;

	va_end(arg);
}

void avrSerialxPrint(xComPortHandlePtr pxPort, uint8_t * str)
{
	uint16_t i = 0;
	size_t stringlength;

	stringlength = strlen((char *)str);

	while(i < stringlength)
		avrSerialxWrite(pxPort, str[i++]);
}

void avrSerialxPrint_P(xComPortHandlePtr pxPort, PGM_P str)
{
	uint16_t i = 0;
	size_t stringlength;

	stringlength = strlen_P(str);

	while(i < stringlength)
		avrSerialxWrite(pxPort, pgm_read_byte(&str[i++]));
}

inline void avrSerialxWrite(xComPortHandlePtr pxPort, int8_t DataOut)
{
	while (!avrSerialxCheckTxReady(pxPort))		// while NOT ready to transmit
        _delay_us(25);     						// delay

	switch (pxPort->usart)
	{
		case USART0:
			UDR0 = DataOut;
			break;

		case USART1:
#if defined(__AVR_ATmega324P__)  || defined(__AVR_ATmega644P__)|| defined(__AVR_ATmega1284P__) || defined(__AVR_ATmega324PA__) || defined(__AVR_ATmega644PA__) ||defined(__AVR_ATmega640__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega1281__) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega2561__)
			UDR1 = DataOut;
			break;
#endif

		case USART2:
#if defined(__AVR_ATmega640__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega1281__) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega2561__)
			UDR2 = DataOut;
			break;
#endif

		case USART3:
#if defined(__AVR_ATmega640__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega1281__) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega2561__)
			UDR3 = DataOut;
			break;
#endif

		default:
			break;
	}
}

inline int8_t avrSerialxRead(xComPortHandlePtr pxPort)
{
	while (!avrSerialxCheckRxComplete(pxPort))	// While data is NOT available to read
		_delay_us(25);     						// delay
	/* Get status and data */
	/* from buffer */

	switch (pxPort->usart)
	{
		case USART0:
			/* If error, return 0xFF */
			if ( UCSR0A & (_BV(FE0)|_BV(DOR0)|_BV(UPE0)) )
				return 0xFF;
			else
				return UDR0;
			break;

		case USART1:
#if defined(__AVR_ATmega324P__)  || defined(__AVR_ATmega644P__)|| defined(__AVR_ATmega1284P__) || defined(__AVR_ATmega324PA__) || defined(__AVR_ATmega644PA__) ||defined(__AVR_ATmega640__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega1281__) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega2561__)
			/* If error, return 0xFF */
			if ( UCSR1A & (_BV(FE1)|_BV(DOR1)|_BV(UPE1)) )
				return 0xFF;
			else
				return UDR1;
			break;
#endif

		case USART2:
#if defined(__AVR_ATmega640__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega1281__) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega2561__)
			/* If error, return 0xFF */
			if ( UCSR2A & (_BV(FE2)|_BV(DOR2)|_BV(UPE2)) )
				return 0xFF;
			else
				return UDR2;
			break;
#endif

		case USART3:
#if defined(__AVR_ATmega640__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega1281__) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega2561__)
			/* If error, return 0xFF */
			if ( UCSR3A & (_BV(FE3)|_BV(DOR3)|_BV(UPE3)) )
				return 0xFF;
			else
				return UDR3;
			break;
#endif
		default:
			break;
	}
	/* If error, return 0xFF */
	return 0xFF;
}

inline int8_t avrSerialxCheckRxComplete(xComPortHandlePtr pxPort )
{
	switch (pxPort->usart)
	{
		case USART0:
			return( UCSR0A & (_BV(RXC0)) );			// nonzero if serial data is available to read.
			break;

		case USART1:
#if defined(__AVR_ATmega324P__)  || defined(__AVR_ATmega644P__)|| defined(__AVR_ATmega1284P__) || defined(__AVR_ATmega324PA__) || defined(__AVR_ATmega644PA__) ||defined(__AVR_ATmega640__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega1281__) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega2561__)
			return( UCSR1A & (_BV(RXC1)) );			// nonzero if serial data is available to read.
			break;
#endif

		case USART2:
#if defined(__AVR_ATmega640__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega1281__) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega2561__)
			return( UCSR2A & (_BV(RXC2)) );			// nonzero if serial data is available to read.
			break;
#endif
		case USART3:
#if defined(__AVR_ATmega640__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega1281__) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega2561__)
			return( UCSR3A & (_BV(RXC3)) );			// nonzero if serial data is available to read.
			break;
#endif
		default:
			break;
	}
	return 0;
}

inline int8_t avrSerialxCheckTxReady(xComPortHandlePtr pxPort )
{
	switch (pxPort->usart)
	{
		case USART0:
			return( UCSR0A & (_BV(UDRE0)) );		// nonzero if transmit register is ready to receive new data.
			break;

		case USART1:
#if defined(__AVR_ATmega324P__)  || defined(__AVR_ATmega644P__)|| defined(__AVR_ATmega1284P__) || defined(__AVR_ATmega324PA__) || defined(__AVR_ATmega644PA__) ||defined(__AVR_ATmega640__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega1281__) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega2561__)
			return( UCSR1A & (_BV(UDRE1)) );		// nonzero if transmit register is ready to receive new data.
			break;
#endif
		case USART2:
#if defined(__AVR_ATmega640__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega1281__) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega2561__)
			return( UCSR2A & (_BV(UDRE2)) );		// nonzero if transmit register is ready to receive new data.
			break;
#endif

		case USART3:
#if defined(__AVR_ATmega640__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega1281__) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega2561__)
			return( UCSR3A & (_BV(UDRE3)) );		// nonzero if transmit register is ready to receive new data.
			break;
#endif
		default:
			break;
	}
	return 0;
}
/*-----------------------------------------------------------*/

#if defined(__AVR_ATmega640__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega1281__) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega2561__)
ISR( USART0_RX_vect )
#elif defined(__AVR_ATmega324P__)  || defined(__AVR_ATmega644P__)|| defined(__AVR_ATmega1284P__) || defined(__AVR_ATmega324PA__) || defined(__AVR_ATmega644PA__)
ISR( USART0_RX_vect )
#elif defined(__AVR_ATmega168__) || defined(__AVR_ATmega328P__)
ISR( USART_RX_vect )
#endif
{
	uint8_t cChar;

	/* Get status and data from buffer */

	/* If error bit set (Frame Error, Data Over Run, Parity), return nothing */
	if ( ! (UCSR0A & ((1<<FE0)|(1<<DOR0)|(1<<UPE0)) ) )
	{
		/* If no error, get the character and post it on the buffer of Rxed characters.*/
		cChar = UDR0;

		if( ! ringBuffer_IsFull( &(xSerialPort.xRxedChars) ) )
			ringBuffer_Poke( &(xSerialPort.xRxedChars), cChar);
	}
	else {
		uint8_t dummy = UDR0;
	}
}
/*-----------------------------------------------------------*/

#if defined(__AVR_ATmega640__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega1281__) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega2561__)
ISR( USART0_UDRE_vect )
#elif defined(__AVR_ATmega324P__)  || defined(__AVR_ATmega644P__)|| defined(__AVR_ATmega1284P__) || defined(__AVR_ATmega324PA__) || defined(__AVR_ATmega644PA__)
ISR( USART0_UDRE_vect )
#elif defined(__AVR_ATmega168__) || defined(__AVR_ATmega328P__)
ISR( USART_UDRE_vect )
#endif
{
	if( ringBuffer_IsEmpty( &(xSerialPort.xCharsForTx) ) )
	{
		// Queue empty, nothing to send.
		vInterrupt0_Off();
	}
	else
	{
		UDR0 = ringBuffer_Pop( &(xSerialPort.xCharsForTx) );
	}
}
/*-----------------------------------------------------------*/

#if defined(__AVR_ATmega324P__)  || defined(__AVR_ATmega644P__)|| defined(__AVR_ATmega1284P__) || defined(__AVR_ATmega324PA__) || defined(__AVR_ATmega644PA__) || defined(__AVR_ATmega640__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega1281__) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega2561__)
ISR( USART1_RX_vect )
{
	uint8_t cChar;

	/* Get status and data from buffer */

	/* If error bit set (Frame Error, Data Over Run, Parity), return nothing */
	if ( ! (UCSR1A & ((1<<FE1)|(1<<DOR1)|(1<<UPE1)) ) )
	{
		/* If no error, get the character and post it on the buffer of Rxed characters. */
		cChar = UDR1;

		if( ! ringBuffer_IsFull( &(xSerial1Port.xRxedChars) ) )
			ringBuffer_Poke( &(xSerial1Port.xRxedChars), cChar);
	}
	else {
		uint8_t dummy = UDR1;
	}
}
/*-----------------------------------------------------------*/

ISR( USART1_UDRE_vect )
{
	if( ringBuffer_IsEmpty( &(xSerial1Port.xCharsForTx) ) )
	{
		// Queue empty, nothing to send.
		vInterrupt1_Off();
	}
	else
	{
		UDR1 = ringBuffer_Pop( &(xSerial1Port.xCharsForTx) );
	}
}
/*-----------------------------------------------------------*/

#endif

#if defined(__AVR_ATmega640__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega1281__) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega2561__)
ISR( USART2_RX_vect )
{
	uint8_t cChar;

	/* Get status and data from buffer */

	/* If error bit set (Frame Error, Data Over Run, Parity), return nothing */
	if ( ! (UCSR2A & ((1<<FE2)|(1<<DOR2)|(1<<UPE2)) ) )
	{
		/* If no error, get the character and post it on the buffer of Rxed characters. */
		cChar = UDR2;

		if( ! ringBuffer_IsFull( &(xSerial2Port.xRxedChars) ) )
			ringBuffer_Poke( &(xSerial2Port.xRxedChars), cChar);
	}
	else {
		uint8_t dummy = UDR2;
	}

}
/*-----------------------------------------------------------*/

ISR( USART2_UDRE_vect )
{
	if( ringBuffer_IsEmpty( &(xSerial2Port.xCharsForTx) ) )
	{
		// Queue empty, nothing to send.
		vInterrupt2_Off();
	}
	else
	{
		UDR2 = ringBuffer_Pop( &(xSerial2Port.xCharsForTx) );
	}
}
/*-----------------------------------------------------------*/
ISR( USART3_RX_vect )
{
	uint8_t cChar;

	/* Get status and data from buffer */

	/* If error bit set (Frame Error, Data Over Run, Parity), return nothing */
	if ( ! (UCSR3A & ((1<<FE3)|(1<<DOR3)|(1<<UPE3)) ) )
	{
		/* If no error, get the character and post it on the buffer of Rxed characters. */
		cChar = UDR3;

		if( ! ringBuffer_IsFull( &(xSerial3Port.xRxedChars) ) )
			ringBuffer_Poke( &(xSerial3Port.xRxedChars), cChar);
	}
	else {
		uint8_t dummy = UDR3;
	}

}
/*-----------------------------------------------------------*/

ISR( USART3_UDRE_vect )
{
	if( ringBuffer_IsEmpty( &(xSerial3Port.xCharsForTx) ) )
	{
		// Queue empty, nothing to send.
		vInterrupt3_Off();
	}
	else
	{
		UDR3 = ringBuffer_Pop( &(xSerial3Port.xCharsForTx) );
	}
}
/*-----------------------------------------------------------*/
#endif



