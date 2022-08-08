//Custom UART functions created by Dmitrii Gusev (Group 05) for the MSE450 Asteroids Project
//Simon Fraser University - Summer 2021

//this library is licensed under GPL-3.0, please, keep this description

/*
	How to use?
	SIMPLE! COPY THE LINE BELOW!
	initializeUART(x); //x must be 0 (for debug port on Tiva itself), 5 (UART5, PE4, PE5) or 50 (both)
	use functions shown below
*/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "tm4c123gh6pm.h" //used for register implementation

void delay(int ms) { 
	//delay works based on clock cycles
	int i, delayLength;
	for (delayLength = 0; delayLength < ms; delayLength++) {
		for(i = 0; i < 3180; i++) { } //1ms delay
	}
}

void initializeUART5(void) {
	//this function initializes UART5

	SYSCTL_RCGCUART_R |= 0x20;  /* enable clock to UART5 */
    SYSCTL_RCGCGPIO_R |= 0x10;  /* enable clock to PORTE for PE4/Rx and RE5/Tx */
    delay(1);

    /* UART0 initialization */
    UART5_CTL_R = 0;         /* UART5 module disbable */
    UART5_IBRD_R = 104;      /* for 9600 baud rate, integer = 104 */
    UART5_FBRD_R = 11;       /* for 9600 baud rate, fractional = 11*/
    UART5_CC_R = 0;          /*select system clock*/
    UART5_LCRH_R = 0x60;     /* data lenght 8-bit, not parity bit, no FIFO */
    UART5_CTL_R = 0x301;     /* Enable UART5 module, Rx and Tx */

    /* UART5 TX5 and RX5 use PE4 and PE5. Configure them digital and enable alternate function */
    GPIO_PORTE_DEN_R = 0x30;      /* set PE4 and PE5 as digital */
    GPIO_PORTE_AFSEL_R = 0x30;    /* Use PE4,PE5 alternate function */
    GPIO_PORTE_AMSEL_R = 0;    /* Turn off analg function*/
    GPIO_PORTE_PCTL_R = 0x00110000;     /* configure PE4 and PE5 for UART */
}

void initializeUART0(void) {
	//uart0 initialization function
	SYSCTL_RCGCUART_R |= 1; /* provide clock to UART0 */
 	SYSCTL_RCGCGPIO_R |= 1; /* enable clock to PORTA */
 
 /* UART0 initialization */
	UART0_CTL_R = 0; /* disable UART0 */
	UART0_IBRD_R = 104; /* 16MHz/16=1MHz, 1MHz/104=9600 baud rate */
	UART0_FBRD_R = 11; /* fraction part, see Example 4-4 */
	UART0_CC_R = 0; /* use system clock */
	UART0_LCRH_R = 0x60; /* 8-bit, no parity, 1-stop bit, no FIFO */
	UART0_CTL_R = 0x301; /* enable UART0, TXE, RXE */
 
 /* UART0 TX0 and RX0 use PA0 and PA1. Set them up. */
 	GPIO_PORTA_DEN_R = 0x03; /* Make PA0 and PA1 as digital */
 	GPIO_PORTA_AFSEL_R = 0x03; /* Use PA0,PA1 alternate function */
 	GPIO_PORTA_PCTL_R = 0x11; /* configure PA0 and PA1 for UART */
}

void initializeUART(int UARTx) {
	//universal function that initializes UARTs:
	switch (UARTx) {
		case 0:
			initializeUART0();
			break;
		case 5:
			initializeUART5();
			break;
		case 50:
			initializeUART5();
			initializeUART0();
			break;
	}
}

char UART5receiver(void) {
	//Receive a char byte from UART5
	char data;
	while((UART5_FR_R & (1<<4)) != 0) {
		/* wait until Rx buffer is not full */
	} 

    data = UART5_DR_R ;  	/* before giving it another byte */
    return (unsigned char) data; //perform automatical data conversion to unsigned char
}

char UART0receiver(void) {
	//Receive a char byte from UART5
	char data;
	while((UART0_FR_R & (1<<4)) != 0) {
		/* wait until Rx buffer is not full */
	} 

    data = UART0_DR_R ;  	/* before giving it another byte */
    return (unsigned char) data; //perform automatical data conversion to unsigned char
}

void UART5transmitter(unsigned char data) {
	//send to UART5 data at 9600 baud rate
	while((UART5_FR_R & (1<<5)) != 0); /* wait until Tx buffer not full */
    UART5_DR_R = data;                  /* before giving it another byte */
}

void UART0transmitter(unsigned char data) {
	while ((UART0_FR_R & 0x20) != 0);				/* wait until Tx buffer not full */  
  	UART0_DR_R = data;	
}

void printStringOnUART5(char *str) {
	//send string via UART5
	while(*str) {
		UART5transmitter(*(str++));
	}
}

void printStringOnUART0(char *str) {
	//send string via UART5
	while(*str) {
		UART0transmitter(*(str++));
	}
}