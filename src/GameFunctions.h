//GAME FUNCTIONS
//Screen Functions

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "tm4c123gh6pm.h" //used for register implementation
#include "Nokia5110.h"
#include "Group5_AnalogRead.h"
#include "Group5_UART.h"
#include "Object.h"
#include "bitmaps.h"

//variables
char positionOfBullet[20]; //debug only

//game functions:
int map(int x, int in_min, int in_max, int out_min, int out_max) { //interpolation function
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

int * setGunDirection() { //100% works, tested
	//function calculates angle with respect to the analog reading

	//unsigned char direction[2]; //x, y
	int direction[2];
	
	int mappedX = (-1)*map(analogReadPE(2), 200, 4000, -42, 42); //values are negative due to the direction of the actual hardware
	int mappedY = (-1)*map(analogReadPE(3), 200, 4000, -24, 24);

	direction[0] = 42 + mappedX;
	direction[1] = 24 + mappedY;

	return direction; //x, y
}

void moveObjectToTargetByOnePixel(SObj *entity, unsigned char desiredX, unsigned char desiredY) {
	//this function moves selected object to the desired position
	unsigned char currentX = entity->x;
	unsigned char currentY = entity->y;

	if (desiredX != currentX) {
		if (desiredX < currentX) {
			currentX--;
		} else if (desiredX > currentX) {
			currentX++;
		}
	} 
	
	if (desiredY != currentY) {
		if (desiredY < currentY) {
			currentY--;
		} else if (desiredY > currentY) {
			currentY++;
		} 
	}
	//two IFs are required in order to make system correctly work with X, Y coordinates

	//apply the new values to the struct in order to modify it
	entity->x = currentX;
	entity->y = currentY;
}

void drawBullet(unsigned char X, unsigned char Y, unsigned char size) {
	//function that draws bullet -> due to the nature of the screen, please, use only odd numbers... 1, 3, 5, 7 etc
	unsigned char i;
	unsigned char j;

	//create buffer area
	for (i = 0; i < size; i++) {
		//uses rows only to create two columns of empty pixes right in front of the bullet and behind the bullet
		Nokia5110_ClearPixel(X-((size)/2), Y-((size-1)/2) + i);
		Nokia5110_ClearPixel(X+((size)/2), Y-((size-1)/2) + i);
	}

	Nokia5110_SetPixel(X, Y);
	for (i = 0; i < size; i++) {
		for (j = 0; j < size; j++) {
			Nokia5110_SetPixel(X-((size-1)/2) + j, Y-((size-1)/2) + i);
		}
	}
}

uint8_t registerObjectHit(unsigned char objectX, unsigned char objectY, unsigned char asteroidX, unsigned char asteroidY) {
	//compares asteroid X,Y with object X,Y (bullet or ship)
	uint8_t isThereDirectHit = 0; //false

	if (asteroidX > objectX) {
		//asteroid on the right side
		if (asteroidY > objectY) {
			//asteroid right bottom quartile
			if (objectX >= asteroidX && objectY >= (asteroidY - 5)) { //adjust coordinates with respect to the bottom left corner of the asteroid
				GPIO_PORTF_DATA_R = 0x08;
				delayOnTimer0(100);
				isThereDirectHit = 1; //set to true
			}
		} else if (asteroidY < objectY) {
			//asteroid right top quartile
			if (objectX >= asteroidX && objectY <= asteroidY) { //adjust coordinates with respect to the bottom left corner of the asteroid
				GPIO_PORTF_DATA_R = 0x08;
				delayOnTimer0(100);
				isThereDirectHit = 1; //set to true
			}
		}
	} else if (asteroidX < objectX) {
		//asteroid on the left side
		if (asteroidY > objectY) {
			//asteroid left bottom quartile
			if (objectX <= (asteroidX + 8) && objectY >= (asteroidY - 5)) { //adjust coordinates with respect to the bottom left corner of the asteroid
				GPIO_PORTF_DATA_R = 0x08;
				delayOnTimer0(100);
				isThereDirectHit = 1; //set to true
			}
		} else if (asteroidY < objectY) {
			//asteroid left top quartile
			if (objectX <= (asteroidX + 8) && objectY <= asteroidY) { //adjust coordinates with respect to the bottom left corner of the asteroid
				GPIO_PORTF_DATA_R = 0x08;
				delayOnTimer0(100);
				isThereDirectHit = 1; //set to true
			}
		}
	}

	GPIO_PORTF_DATA_R = 0x00;
	return isThereDirectHit;
} 

void generateOneAsteroid(SObj *asteroidEntity, unsigned char pos_x, unsigned char pos_y, unsigned char x_target, unsigned char y_target) { //generate single asteroid 
	//while loop allows to prevent asteroids from appearing at the same positon as ship
	while ( (pos_x > 25 && pos_x < 50) && (pos_y > 20 && pos_y < 30) ) {
		pos_x = rand() % 84;
		pos_y = rand() % 48;
	}

	//apply final X,Y and target coordinates to asteroid
	asteroidEntity->x = pos_x;
	asteroidEntity->y = pos_y;
	asteroidEntity->xtarget = x_target;
	asteroidEntity->ytarget = y_target;
}

void resetObjectAfterHit(SObj *asteroidEntity, SObj *bulletEntity) {
	//reset asteroid after hit
	generateOneAsteroid(asteroidEntity, rand() % 84, rand() %48, rand() % 84, rand() % 48); //generate one asteroid at random coordinates with random direction

	//reset bullet
	bulletEntity->x = 40;
	bulletEntity->y = 20;
	bulletEntity->xtarget = 40;
	bulletEntity->ytarget = 20;
}

void clearBulletAtScreenEdge(SObj *objectEntity, unsigned char defaultX, unsigned char defaultY) {
	unsigned char X = objectEntity->x;
	unsigned char Y = objectEntity->y;

	//clear bullet if it reached its target without hitting anything, reduce screen buffer BUT bullet must not be at the ship
	if ( (X != defaultX && Y != defaultY) && (X == objectEntity->xtarget && Y == objectEntity->ytarget) ) {
		objectEntity->x = defaultX;
		objectEntity->y = defaultY;

		objectEntity->xtarget = defaultX;
		objectEntity->ytarget = defaultY;
	}

	if ( X >= 75 || X <= 5 || Y <= 10 || Y >= 40 ) {
		//clear bullet if it reached screen edge
		objectEntity->x = defaultX;
		objectEntity->y = defaultY;

		objectEntity->xtarget = defaultX;
		objectEntity->ytarget = defaultY;
	}
}

//DEBUGGING FUNCTIONS
void bitmapTest(unsigned char startX, unsigned char startY) {
	Nokia5110_Clear();
	Nokia5110_ClearBuffer();
	Nokia5110_DisplayBuffer();

	Nokia5110_PrintBMP(startX, startY, debug, 0);
	Nokia5110_DisplayBuffer();
	delay(100);
}
