#include "LPC23xx.h"

 void delay(void)
 { int i,j;
 for(i=0; i<0xff;i++)
 for(j=0; j<0XFF;j++);
 }

 int main ()
 {
 while(1)
 {
 FIO3DIR=0xFFFFFFFF; // LEDs are connected to lower 8 bits of
 // Fast IO port 3 (FIO3PIN). Hence set the lower 8 bits
 // as output in the corresponding direction
 // register (FIO3DIR)
 FIO3PIN=0xFFFFFFFF; //Port to which actual data is to be written
 delay; //to be written
 FIO3PIN=0x00000000;
 delay; //to be written
 }
 return 0;
 }