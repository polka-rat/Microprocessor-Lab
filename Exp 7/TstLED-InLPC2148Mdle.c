//LPC2148- C program to display a value on the LEDs
#include "LPC214x.h"                        /* LPC21xx definitions */
 
#define LED_IOPIN		IO0PIN
#define BIT(x)	(1 << x)

#define LED_D0	(1 << 10)		// P0.10
#define LED_D1	(1 << 11)		// P0.11
#define LED_D2	(1 << 12)		// P0.12
#define LED_D3	(1 << 13)		// P0.13

#define LED_D4	(1 << 15)		// P0.15
#define LED_D5	(1 << 16)		// P0.16
#define LED_D6	(1 << 17)		// P0.17
#define LED_D7	(1 << 18)		// P0.18
#define LED_DATA_MASK			((unsigned long)((LED_D7 | LED_D6 | LED_D5 | LED_D4 | LED_D3 | LED_D2 | LED_D1 | LED_D0)))
	#ifndef LED_DRIVER_OUTPUT_EN
#define LED_DRIVER_OUTPUT_EN (1 << 5)	// P0.5
#endif
#define LED1_ON		LED_IOPIN |= (unsigned long)(LED_D0);		// LED1 ON
#define LED2_ON		LED_IOPIN |= (unsigned long)(LED_D1);		// LED2 ON
#define LED3_ON		LED_IOPIN |= (unsigned long)(LED_D2);		// LED3 ON
#define LED4_ON		LED_IOPIN |= (unsigned long)(LED_D3);		// LED4 ON
#define LED5_ON		LED_IOPIN |= (unsigned long)(LED_D4);		// LED5 ON
#define LED6_ON		LED_IOPIN |= (unsigned long)(LED_D5);		// LED6 ON
#define LED7_ON		LED_IOPIN |= (unsigned long)(LED_D6);		// LED7 ON
#define LED8_ON		LED_IOPIN |= (unsigned long)(LED_D7);		// LED8 ON

int main (void)
{
	 
  IO0DIR |= LED_DATA_MASK;			// GPIO Direction control -> pin is output 
	IO0DIR |= LED_DRIVER_OUTPUT_EN;		// GPIO Direction control -> pin is output 
	IO0CLR |= LED_DRIVER_OUTPUT_EN;
	
	
	while(1)
	{
		int value=0x86;
		 
	if(value & BIT(0)) LED8_ON;
	if(value & BIT(1)) LED7_ON;
	if(value & BIT(2)) LED6_ON;
	if(value & BIT(3)) LED5_ON;

	if(value & BIT(4)) LED4_ON;
	if(value & BIT(5)) LED3_ON;
	if(value & BIT(6)) LED2_ON;
	if(value & BIT(7)) LED1_ON;	
	}

  return 0;
}

/******************************************************************************
**                            End Of File
******************************************************************************/

