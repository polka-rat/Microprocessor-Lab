/*5V Stepper Motor
 
Connector J16 connect with stepper motor  as per below mentioned configuration

Pin - 1  : BLUE 
Pin - 2  : PINK
Pin - 3  : YELLOW
Pin - 4  : ORANGE
Pin - 5  : Red (Motor Vcc)

Motor Pins:
P0.4 to P0.7
*/

#include <LPC214x.h>                        /* LPC21xx definitions */

void delay_mSec(int);

int main (void)
{
	 int i;
unsigned char steps[4] = {0x09, 0x0c, 0x06, 0x03};  //standard step sequence for stepper motor
signed char x = 0; 


	PINSEL0 = 0x0;												// Pin function Select -> P0.0 to P0.15 -> GPIO Port
	IO0DIR |= 0xF0;                       // Set stepper motor pins as output in IO0 port
	delay_mSec(10);
 
	while(1)
	{
	for(i=0;i<2500;i++)
	{
		IO0PIN = (steps[x++] << 4); //send the 4 bit step value to stepper motor lines connected to IO0 port
		if(x > 3)
			x = 0;
			
		delay_mSec(2);	
	}
	}
    return 0;
}

void delay_mSec(int dCnt)		// pr_note:~dCnt mSec
{
  int j=0,i=0;
  while(dCnt--) 
  {
	  for(j=0;j<1000;j++)
	  {
	    /* At 60Mhz, the below loop introduces
	    delay of 10 us */
	    for(i=0;i<10;i++);
	  }
  }
}

