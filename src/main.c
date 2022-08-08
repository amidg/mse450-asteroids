/* MSE450 - SUMMER 2021 - GROUP 05 

Dmitrii Gusev – 301297008

FINAL PROJECT: ATARI 2600 ASTEROIDS GAME (1971)

THIS CODE IS DEVELOPED BY GROUP 05. CODE WILL BE RELEASED UNDER GPL-3.0 LICENSE ON GITHUB ONCE COMPLETED.

*/

/*
	PROJECT DESCRIPTION:
	Atari Asteroids (1971) developed for this project utilizes the following functionality:
	1. UART5 (PE4, PE5) for the PC communication via MCP2221 USB-UART bridge via Type-C cable
	2. Analog-to-Digital converter for the analog joystick for the game controls
	3. Buttons and LEDs connected to the GPIO peripherals for the game controls
	4. Nokia 5110 84x48 LCD display for game UI

	PROJECT WORKFLOW:
	1. Code is written in Visual Studio Code to ensure compatibility between Linux (Dmitrii develops here), Windows (Amr checks here) and MacOS (Aldiyar and Khamat develop here) machines.
	2. This project supports both Keil uVision (tested on version 4 and 5) and libopencm3 (Linux / MacOS).
	3. Project is not guaranteed to work in the Keil simulator but 100% compatible with EK-TM4C123GXL board.
	IF YOU DON'T HAVE REAL HARDWARE - IT IS NOT MY PROBLEM! 
*/

//#include "TExaS.h" //this garbage ignored
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "tm4c123gh6pm.h" //used for register implementation
#include "GameFunctions.h" 
#include <libopencm3/lm4f/nvic.h>

/*
	GameFunctions.h already initializes the following:
	#include "Group5_UART.h" //UART function library created by Dmitrii Gusev for MSE450 Project -> supports both UART0 and UART5
	#include "Group5_AnalogRead.h" //analog read function created by Dmitrii Gusev for MSE450 project -> Supports PE2 and PE3 Analog functionality of the Tiva C board
	#include "Nokia5110.h" //default Keil library for Nokia screen
	#include "Object.h"
	#include "bitmaps.h" //bitmap library created by Dmitrii Gusev
*/

//	Global Variables
unsigned long SW1,SW2;  

//OS compiler selection -> never define both as 1 
#define LIBOPENCM3 1 //uses this if libopencm3 UNIX package is used for compilation (Linux/Mac/Windows)
#define KEIL 0 //uses this ISR if KEIL is used for code complilation (Windows ONLY)
#define GAMESPEED 100 //lower value -> faster game happens, higher FPS. Value of 100 is recommended

//debug only, comment later
char mesg[50];

//game variables
int spaceshipX = 32;
int spaceshipY = 24;
int numberOfBullets = 0;
int numberOfAsteroids = 4;

SObj asteroid[4];

SObj bullet[50];

SObj spaceship;

uint8_t isShipAlive = 1; //shows if ship is alive or not

uint8_t score; //max 255, because who will play longer, it will just save some RAM/ROM
char scoreText[20]; //score text for screen

//Function declaration ==========================================================================================================================================================================
void PortF_Init(void); //init Port F for SW1, SW2 and RGB LED
void enableSW1interrupt(void); //enable interrupt on SW1 pin
void delayOnTimer0(int ms);
void init_rand(void); //create random initialization

//Screen Functions
void welcomeScreen(void);
void gameScreen(uint8_t isDebugging);
void deadScreen(uint8_t isShipAlive);
void redrawGameScreen(void);

//game functions that are responsible for the general applications (e.g. moving objects) are located in the separate header file. Local game functions are located below:
void generateAsteroids(void); //generate new asteroids
void generateBullets(void); //generate bullets
void moveAsteroids(void); //move asteroids to the desired location
void moveBullets(); //move bullets to the desired location
void asteroidDestroyChecker();
void spaceshipDestroyChecker();

//MAIN FUNCTION FOR EXECUTION  ==================================================================================================================================================================
int main(void){    
	//main function of the project
	//functions for initialization
	//TExaS_Init(SW_PIN_PF40,LED_PIN_PF321); 	//	TExaS_Init initializes the real board grader for lab 4
	PortF_Init();        										// Call initialization of port PF  
	Nokia5110_Init(); //screen init
	initializeUART(5); //choose which UART to initialize: UART0, UART5 or both
	delay(100);

	//main loop function
	while(1){ //MAIN FUNCTION LOOP -> PUT CODE UNDER IT
		//UART debug lines -> uncomment only if needed
		//printStringOnUART5("\r\nHELLO BLYAT");

		// sprintf(mesg,"\r\nHorizontal Position: %u", analogReadPE(2));
		// printStringOnUART5(mesg);

		welcomeScreen(); //start game
		//end of while(1)
	}
}

//FUNCTION DEFINITIONS ===========================================================================================================================================================================
//port F initialization
void PortF_Init(void){ //comments by Dmitrii Gusev
	SYSCTL_RCGC2_R |= 0x00000020;     // 1) enable clock on port F -> must enable clock on any pin that is used
	GPIO_PORTF_LOCK_R = 0x4C4F434B;   // 2) unlock PortF PF0  is set using GPIOCR register table 
	GPIO_PORTF_CR_R = 0x1F;           // allow changes to PF4-0  -> allow GPIOPUR register to commit        
	GPIO_PORTF_AMSEL_R = 0x00;        // 3) disable analog function
	GPIO_PORTF_PCTL_R = 0x00000000;   // 4) GPIO clear bit PCTL  
	GPIO_PORTF_DIR_R = 0x0E;          // 5) PF3,PF2,PF1 are set to be digital output, PF4,PF0 seem to be digital input by default   
	GPIO_PORTF_AFSEL_R = 0x00;        // 6) no alternate function
	GPIO_PORTF_PUR_R = 0x11;          // enable pullup resistors on PF4,PF0       
	GPIO_PORTF_DEN_R = 0x1F;          // 7) enable digital pins PF4-PF0        
}

void enableSW1interrupt(void) {
	SYSCTL_RCGCGPIO_R |= (1<<5);   /* Set bit5 of RCGCGPIO to enable clock to PORTF*/
		
	/* PORTF0 has special function, need to unlock to modify */
	GPIO_PORTF_LOCK_R = 0x4C4F434B;   /* unlock commit register */
	GPIO_PORTF_CR_R = 0x01;           /* make PORTF0 configurable */
	GPIO_PORTF_LOCK_R = 0;            /* lock commit register */

	/*Initialize PF3 as a digital output, PF0 and PF4 as digital input pins */
	GPIO_PORTF_DIR_R &= ~(1<<4)|~(1<<0);  /* Set PF4 and PF0 as a digital input pins */
	GPIO_PORTF_DIR_R |= (1<<3);           /* Set PF3 as digital output to control green LED */
	GPIO_PORTF_DEN_R |= (1<<4)|(1<<3)|(1<<0);             /* make PORTF4-0 digital pins */
	GPIO_PORTF_PUR_R |= (1<<4)|(1<<0);             /* enable pull up for PORTF4, 0 */
		
	/* configure PORTF4, 0 for falling edge trigger interrupt */
	GPIO_PORTF_IS_R  &= ~(1<<4)|~(1<<0);        /* make bit 4, 0 edge sensitive */
	GPIO_PORTF_IBE_R &=~(1<<4)|~(1<<0);         /* trigger is controlled by IEV */
	GPIO_PORTF_IEV_R &= ~(1<<4)|~(1<<0);        /* falling edge trigger */
	GPIO_PORTF_ICR_R |= (1<<4)|(1<<0);          /* clear any prior interrupt */
	GPIO_PORTF_IM_R  |= (1<<4)|(1<<0);          /* unmask interrupt */
		
	/* enable interrupt in NVIC and set priority to 5 */
	NVIC_PRI7_R = (NVIC_PRI7_R&0xFF00FFFF)|0x00A00000; // (g) priority 5
	NVIC_EN0_R = 0x40000000;      // (h) enable interrupt 30 in NVIC
}

void inti_rand(void){
	time_t t;
	srand((unsigned) time(&t));
}

//INTERRUPT SERVICE ROUTINE ON SW1
#ifdef LIBOPENCM3 //uses this if libopencm3 UNIX package is used for compilation (Linux/Mac/Windows)
	void gpiof_isr(void) {
		int gunDirection[2]; //x, y
		//char gunDirectionString[20];
	
		//interrupt handler on SW1, executes shooting function
		gunDirection[0] = 42 + (-1)*map(analogReadPE(2), 200, 4000, -42, 42);
		gunDirection[1] = 24 + (-1)*map(analogReadPE(3), 200, 4000, -24, 24);


		bullet[numberOfBullets].xtarget = (unsigned char)gunDirection[0]; //set final x coordinate
		bullet[numberOfBullets].ytarget = (unsigned char)gunDirection[1]; //set final y coordinate

		//sprintf(gunDirectionString, "\r\nGunDirection: %u, %u", bullet[numberOfBullets].xtarget, bullet[numberOfBullets].ytarget);
		//printStringOnUART5(gunDirectionString);

		if (numberOfBullets < 50) {
			numberOfBullets++;
		} else {
			numberOfBullets = 0;
		}		
		
		//end of interrupt routine
		GPIO_PORTF_ICR_R |= 0x10; //clear interrupt flag
	}
#endif

#ifdef KEIL //uses this ISR if KEIL is used for code complilation (Windows ONLY)
	void GPIOF_Handler(void) { 
		//interrupt handler on SW1, executes shooting function
		int *gunDirection; //x, y
		gunDirection = setGunDirection(); //set gun direction based on ananlog input

		bullet[numberOfBullets].xtarget = (unsigned char)gunDirection[0]; //set final x coordinate
		bullet[numberOfBullets].ytarget = (unsigned char)gunDirection[1]; //set final y coordinate


		if (numberOfBullets < 50) {
			numberOfBullets++;
		} else {
			numberOfBullets = 0;
		}
		
		//end of interrupt routine
		GPIO_PORTF_ICR_R |= 0x10; //clear interrupt flag
	}
#endif
//END OF INTERRUPT SERVICE ROUTINE ON SW1

void delayOnTimer0(int ms) {
	int i;
    SYSCTL_RCGCTIMER_R |= 1;     //enable clock to Timer Block 0
 
    TIMER0_CTL_R = 0;            //disable Timer before initialization
    TIMER0_CFG_R = 0x04;         //16-bit option
    TIMER0_TAMR_R = 0x02;        //periodic mode and down-counter
    TIMER0_TAILR_R = 16000 - 1;  //Timer A interval load value register
    TIMER0_ICR_R = 0x1;          //clear the TimerA timeout flag
    TIMER0_CTL_R |= 0x01;        //enable Timer A after initialization
 
    for(i = 0; i < ms; i++) { 
		while ((TIMER0_RIS_R & 0x1) == 0) { 
			/* wait for TimerA timeout flag */ 
		}      
        TIMER0_ICR_R = 0x1;      //clear the TimerA timeout flag
    }
}

//SCREEN FUNCTIONS -> CONNECT TO GAME FUNCTIONS THRU HEADER
void welcomeScreen(void) {
	
	//WELCOME SCREEN
	/*
		Since characters are 8 pixels tall and 5 pixels wide, 12 characters fit per row
		Screen is divided into chunks of 55 chunks (X = 11, Y = 5)
		Y = 1: most top line
		Y = 5: most bottom line
	*/

	Nokia5110_Clear();
	Nokia5110_SetCursor(1, 1);
    Nokia5110_OutString("*Group 05*"); //10 characters

	Nokia5110_SetCursor(1, 2);
    Nokia5110_OutString("Asteroids"); // 9 characters

	Nokia5110_SetCursor(1, 3);
    Nokia5110_OutString(">Press SW1<"); // 11 characters

	printStringOnUART5("\r\nPress SW1 to start!");

	SW1 = GPIO_PORTF_DATA_R & 0x10;	//Get PF4
	if (!SW1) { //switch is pressed when SW1 is low
		gameScreen(0); //leave uncommented during game
	} 
}

void gameScreen(uint8_t isDebugging) {
	//debug function
	if (isDebugging) {
		while(1) { 
			bitmapTest(1, 1);
		}
	} else {
		//actual game
		Nokia5110_Clear(); //clear screen
		Nokia5110_ClearBuffer();
		Nokia5110_DisplayBuffer();      // draw buffer

		generateAsteroids(); //generates asteroids in their starting position
		generateBullets(); //generates bullets at the center of the ship
		enableSW1interrupt(); //enable interrupt on SW1 pin

		Nokia5110_PrintBMP(spaceshipX, spaceshipY, spaceInvadersShip, 0); //draw initial spaceship bmp

		delayOnTimer0(100); //100ms delay

		/*
			Project algorithm:
			1. Clear screen
			2. Generate asteroids in their default position
			3. enable interrupt on SW1
			
			while (forever):
				1. Draw spaceship 
				2. Set target X,Y for asteroids and bullets (if present)
					2.1. Draw asteroid on new position
					2.2. Draw bullet in new position
					
				3. Compare bullet X,Y with asteroid X,Y
					3.1. If bullet X,Y == asteroid X,Y -> destroy asteroid
					3.2. generate new asteroid
					3.3. Add score

				4. clear screen
		*/

		while(1) {
			//game must forever stay here, so we don't care about going back
			//every iteration of the screen update clears it, then redraws objects with respect to their new coordinates
			spaceshipDestroyChecker(); //must go before asteroid check to ensure bullets inside the ship do not kill asteroid
			asteroidDestroyChecker(); //check if asteroid was destroyed by bullet
			redrawGameScreen(); //contains all the screen update functions
			deadScreen(isShipAlive);
		}
	}
}

void deadScreen(uint8_t isShipAlive) {
	//dead screen is shown when ship is dead
	if (!isShipAlive) {
		Nokia5110_Clear();
		Nokia5110_SetCursor(1, 2);
   		Nokia5110_OutString("YOU DIED"); //8 characters
		printStringOnUART5("\r\n YOU DIED"); //reference to Dark Souls games

		sprintf(scoreText,"\r\n Score: %i", score);
		Nokia5110_SetCursor(1, 3);
		Nokia5110_OutString(scoreText);
		printStringOnUART5(scoreText);

		Nokia5110_DisplayBuffer();

		score = 0;
		isShipAlive = 1;
		GPIO_PORTF_DATA_R = 0x02; //show red
		delayOnTimer0(5000);

		welcomeScreen();
	}
}

void redrawGameScreen(void) {
	//redraws entire screen
	uint8_t asteroidIter;
	uint8_t bulletIter;

	//clear previous screen -> three retarded functions must go together! 
	Nokia5110_Clear();
	Nokia5110_ClearBuffer();
	Nokia5110_DisplayBuffer();

	/*
		BOTTOM LINE SCORE
		Since characters are 8 pixels tall and 5 pixels wide, 12 characters fit per row,
	*/		
	Nokia5110_SetCursor(1, 5); //most bottom line
	sprintf(scoreText,"\r\n Score: %i", score);
	Nokia5110_OutString(scoreText);
	printStringOnUART5(scoreText);

	//redraw spaceship
	Nokia5110_PrintBMP(spaceshipX, spaceshipY, spaceInvadersShip, 0);

	//redraw asteroids -> works
	moveAsteroids();
	for (asteroidIter = 0; asteroidIter < 4; asteroidIter++) {
		Nokia5110_PrintBMP(asteroid[asteroidIter].x, asteroid[asteroidIter].y, asteroidSmall, 0);
		Nokia5110_DisplayBuffer(); //update buffer
	}
	
	//redraw bullets
	moveBullets(); //x, y
	for (bulletIter = 0; bulletIter < numberOfBullets; bulletIter++) {
		drawBullet(bullet[bulletIter].x, bullet[bulletIter].y, 3);
		Nokia5110_DisplayBuffer();
	}

	delayOnTimer0(GAMESPEED); //100ms timer based on system Timer0 -> controls game speed. Higher delay -> lower FPS
}

//game functions
void moveAsteroids() {
	// moves asteroid the the target locatoin pixel by pixel
	uint8_t i;

	for (i = 0; i < numberOfAsteroids; i++) {
		moveObjectToTargetByOnePixel(&asteroid[i], asteroid[i].xtarget, asteroid[i].ytarget);  
		if ( (asteroid[i].x == 2 || asteroid[i].x == 80) || (asteroid[i].y == 2 || asteroid[i].y == 45) || (asteroid[i].x == asteroid[i].xtarget && asteroid[i].y == asteroid[i].ytarget) ) {
			//check if asteroid reached screen border or its target
			generateOneAsteroid(&asteroid[i], rand() % 84, rand() %48, rand() % 84, rand() % 48); //clear this asteroid by resetting its position
		}
	}
}

void moveBullets() {
	//this function moves bullets
	uint8_t i;

	for (i = 0; i < numberOfBullets; i++) {
		clearBulletAtScreenEdge(&bullet[i], 40, 20); //clear bullets that reach screen edge then place them in their default position
		moveObjectToTargetByOnePixel(&bullet[i], bullet[i].xtarget, bullet[i].ytarget);
	}
}

void asteroidDestroyChecker() {
	//this function compares position of all bullets and asteroids and checks if there is any direct hit
	uint8_t asteroidIter;
	uint8_t bulletIter;

	for (asteroidIter = 0; asteroidIter < numberOfAsteroids; asteroidIter++) {
		//do bullet collide check for every asteroid
		for (bulletIter = 0; bulletIter < numberOfBullets; bulletIter++) {
			//check for the object collision
			if ( registerObjectHit(bullet[bulletIter].x, bullet[bulletIter].y, asteroid[asteroidIter].x, asteroid[asteroidIter].y) ) {
				//there is a hit! 
				score += 1; //update score
				resetObjectAfterHit(&asteroid[asteroidIter], &bullet[bulletIter]);
			}
		}
	}
}

void spaceshipDestroyChecker() {
	//this function checks if asteroid collides with the spaceship
	uint8_t asteroidIter;
	char finalScore[12];

	for (asteroidIter = 0; asteroidIter < numberOfAsteroids; asteroidIter++) {
		if ( asteroid[asteroidIter].x == spaceshipX && asteroid[asteroidIter].y == spaceshipY ) {
			//there is a hit! 
			isShipAlive = 0;
		}
	}
}

void generateAsteroids(void) {
	//creates initial asteroids -> not random, but more like Nintendo egg game
	uint8_t i;

	for (i = 0; i < numberOfAsteroids; i++) {
		if (i == 0) {
			generateOneAsteroid(&asteroid[i], rand() % 40, rand() % 24, (((unsigned char)(84 - 75 + 1)/RAND_MAX) * rand() + 75),(((unsigned char)(20 - 12 + 1)/RAND_MAX) * rand() + 12));
		} else if (i == 1) {
			generateOneAsteroid(&asteroid[i], (((unsigned char)(84 - 34 + 1)/RAND_MAX) * rand() + 34), rand() % 24, rand() % 5,(((unsigned char)(45 - 30 + 1)/RAND_MAX) * rand() + 30));
		} else if (i == 2) {
			generateOneAsteroid(&asteroid[i], rand() % 40, (((unsigned char)(48 - 20 + 1)/RAND_MAX) * rand() + 20),(((unsigned char)(54 - 42 + 1)/RAND_MAX) * rand() + 42), (((unsigned char)(37 - 27 + 1)/RAND_MAX) * rand() + 27));
		} else if (i == 3) {
			generateOneAsteroid(&asteroid[i], (((unsigned char)(75 - 34 + 1)/RAND_MAX) * rand() + 34), (((unsigned char)(40 - 20 + 1)/RAND_MAX) * rand() + 20), (((unsigned char)(48 - 35 + 1)/RAND_MAX) * rand() + 35), (((unsigned char)(48 - 37 + 1)/RAND_MAX) * rand() + 37));
		}
	}
}

void generateBullets(void) {
	//creates initial asteroids -> not random, but more like Nintendo egg game
	uint8_t i;

	for (i = 0; i < 50; i++) {
		bullet[i].life = 1;

		bullet[i].x = spaceshipX + 8;
		bullet[i].y = spaceshipY - 4;
		drawBullet(bullet[i].x, bullet[i].y, 3);
	}
}
