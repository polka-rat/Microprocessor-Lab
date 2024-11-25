//DAC program for LPC2148 SUNTECH KIT


#include "LPC214x.H"                       /* LPC214x definitions */

#define DAC_BIAS			0x00010000
void mydelay(int);

void DACInit( void )
{
    /* setup the related pin to DAC output */
	PINSEL1 &= 0xFFF3FFFF;
    PINSEL1 |= 0x00080000;	/* set p0.25 to DAC output */   
    return;
}

int main (void)
{
	DACInit();

//SQUARE WAVE
while(1)
{
	DACR=0|DAC_BIAS;
	  mydelay(0x0F);
	  DACR=(0x3FF<<6)|DAC_BIAS;
	  mydelay(0x0f);
}
	return 0;

}
void mydelay(int x)
{
int j,k;
for(j=0;j<=x;j++)
{
    for(k=0;k<=0xFF;k++);
}
}