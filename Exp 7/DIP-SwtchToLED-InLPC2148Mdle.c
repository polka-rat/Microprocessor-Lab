
/*****************************************************************************

DEVELOPED BY:- ARK DEVELOPER
WHAT PROGRAM DO:- LED ON/OFF based on 8-PIN DIP SWITCH STATUS	

******************************************************************************/

#include "LPC2148.H"                       /* LPC214x definitions */
#include "led.h"
#include "delay.h"

//-------------------------------------------------------------------------------------------------

#define DIP_SW_D0 (1 << 16)		// P0.16
#define DIP_SW_D1 (1 << 17)		// P0.17
#define DIP_SW_D2 (1 << 18)		// P0.18
#define DIP_SW_D3 (1 << 19)		// P0.19

#define DIP_SW_D4 (1 << 22)		// P0.22
#define DIP_SW_D5 (1 << 23)		// P0.23
#define DIP_SW_D6 (1 << 24)		// P0.24
#define DIP_SW_D7 (1 << 25)		// P0.25

#define DIP_SW_DIR		IO1DIR
#define DIP_SW_PIN		IO1PIN

#define DIP_SW_DATA_MASK	(DIP_SW_D7 | DIP_SW_D6 | DIP_SW_D5 | DIP_SW_D4 | DIP_SW_D3 | DIP_SW_D2 | DIP_SW_D1 | DIP_SW_D0)

//-------------------------------------------------------------------------------------------------

void set_dipswitch_port_input( void )
{
	DIP_SW_DIR &= ~(DIP_SW_DATA_MASK);
}

unsigned long read_dip_switch( void )
{
	return DIP_SW_PIN; 
}

/*****************************************************************************
**   Main Function  main()
******************************************************************************/
int main (void)
{
	unsigned long sw_status;

	set_led_port_output();
	set_dipswitch_port_input();

	while(1)
	{
		sw_status = read_dip_switch();
/*
		if(sw_status & DIP_SW_D0){ LED1_OFF;} else{ LED1_ON;}
		if(sw_status & DIP_SW_D1){ LED2_OFF;} else{ LED2_ON;}
		if(sw_status & DIP_SW_D2){ LED3_OFF;} else{ LED3_ON;}
		if(sw_status & DIP_SW_D3){ LED4_OFF;} else{ LED4_ON;}

		if(sw_status & DIP_SW_D4){ LED5_OFF;} else{ LED5_ON;}
		if(sw_status & DIP_SW_D5){ LED6_OFF;} else{ LED6_ON;}
		if(sw_status & DIP_SW_D6){ LED7_OFF;} else{ LED7_ON;}
		if(sw_status & DIP_SW_D7){ LED8_OFF;} else{ LED8_ON;}
*/

		if(sw_status & DIP_SW_D0){ LED1_OFF;} else{ LED1_ON;}
				delay_mSec(10);
		if(sw_status & DIP_SW_D1){ LED2_OFF;} else{ LED2_ON;}
				delay_mSec(10);
		if(sw_status & DIP_SW_D2){ LED3_OFF;} else{ LED3_ON;}
				delay_mSec(10);
		if(sw_status & DIP_SW_D3){ LED4_OFF;} else{ LED4_ON;}
				delay_mSec(10);

		if(sw_status & DIP_SW_D4){ LED5_OFF;} else{ LED5_ON;}
				delay_mSec(10);
		if(sw_status & DIP_SW_D5){ LED6_OFF;} else{ LED6_ON;}
				delay_mSec(10);
		if(sw_status & DIP_SW_D6){ LED7_OFF;} else{ LED7_ON;}
				delay_mSec(10);
		if(sw_status & DIP_SW_D7){ LED8_OFF;} else{ LED8_ON;}
				delay_mSec(10);

		delay_mSec(100);
	}

//    return 0;
}

