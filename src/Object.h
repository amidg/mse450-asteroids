//object struct

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct Object {
  volatile unsigned char x;      // real-time x coordinate
  volatile unsigned char y;      // real-time y coordinate

  volatile unsigned char xtarget; //final desired coordinates
  volatile unsigned char ytarget;

  const unsigned char bitmap; //bitmap of the object
  volatile uint8_t life;            // 0=dead, 1=alive
};  

typedef struct Object SObj;