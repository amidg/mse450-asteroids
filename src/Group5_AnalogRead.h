//Custom analog functions created by Dmitrii Gusev (Group 05)  for the MSE450 Asteroids Project
//Simon Fraser University - Summer 2021

//this library is licensed under GPL-3.0, please, keep this description

/*
	How to use?
	SIMPLE! COPY THE LINE BELOW!
	unsigned int analogvalue = analogReadPE(x); //x must be 2 (PE2) or 3 (PE3)
*/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "tm4c123gh6pm.h" //used for register implementation

void initializePE3(void) { 
	/* Enable Clock to ADC0 and GPIO pins*/
    SYSCTL_RCGCGPIO_R |= (1<<4);   /* Enable Clock to GPIOE or PE3/AN0 */
    SYSCTL_RCGCADC_R |= (1<<0);    /* AD0 clock enable*/
    
    /* initialize PE3 for AIN0 input  */
    GPIO_PORTE_AFSEL_R |= (1<<3);       /* enable alternate function */
    GPIO_PORTE_DEN_R &= ~(1<<3);        /* disable digital function */
    GPIO_PORTE_AMSEL_R |= (1<<3);       /* enable analog function */
   
    /* initialize sample sequencer3 */
    ADC0_ACTSS_R &= ~(1<<3);        /* disable SS3 during configuration */
    ADC0_EMUX_R &= ~0xF000;    /* software trigger conversion */
    ADC0_SSMUX3_R = 0;         /* get input from channel 0 */
    ADC0_SSCTL3_R |= (1<<1)|(1<<2);        /* take one sample at a time, set flag at 1st sample */
    ADC0_ACTSS_R |= (1<<3);         /* enable ADC0 sequencer 3 */
}

void initializePE2(void) {
	SYSCTL_RCGC2_R |= 0x00000010;   // 1) activate clock for Port E

	GPIO_PORTE_DIR_R &= ~0x04;      // 2) make PE2 input
	GPIO_PORTE_AFSEL_R |= 0x04;     // 3) enable alternate function on PE2
	GPIO_PORTE_DEN_R &= ~0x04;      // 4) disable digital I/O on PE2
	GPIO_PORTE_AMSEL_R |= 0x04;     // 5) enable analog function on PE2
	SYSCTL_RCGC0_R |= 0x00010000;   // 6) activate ADC0 
 
	SYSCTL_RCGC0_R &= ~0x00000300;  // 7) configure for 125K 	
	ADC0_SSPRI_R = 0x0123;          // 8) Sequencer 3 is highest priority
	ADC0_ACTSS_R &= ~0x0008;        // 9) disable sample sequencer 3
	ADC0_EMUX_R &= ~0xF000;         // 10) seq3 is software trigger
	ADC0_SSMUX3_R = (ADC0_SSMUX3_R&0xFFFFFFF0)+1; // 11) channel Ain1 (PE2)
	ADC0_SSCTL3_R = 0x0006;         // 12) no TS0 D0, yes IE0 END0
	ADC0_ACTSS_R |= 0x0008;         // 13) enable sample sequencer 3
}

int analogReadPE(int PEx) {
	//read 12-bit analog value on the ADC0 channel
	int result;
	switch (PEx) {
		case 1:
			//printStringOnUART5("PE1 not implemented");
			break;
		case 2:
			initializePE2();
			ADC0_PSSI_R = 0x0008;        //initiate SS3
        	while((ADC0_RIS_R & 8) == 0) ;   //wait for conversion
        	result = ADC0_SSFIFO3_R & 0xFFF; //read result
        	ADC0_ISC_R = 8;          /* clear coversion clear flag bit*/
			break;
		case 3:
			initializePE3();
			ADC0_PSSI_R |= (1<<3);        /* Enable SS3 conversion or start sampling data from AN0 */
        	while((ADC0_RIS_R & 8) == 0) ;   /* Wait untill sample conversion completed*/
        	result = ADC0_SSFIFO3_R; /* read adc coversion result from SS3 FIFO*/
        	ADC0_ISC_R = 8;          /* clear coversion clear flag bit*/
			break;
	}
	
	return result;
}