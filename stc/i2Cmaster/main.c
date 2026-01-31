#include "stc8g.h"
#include "betterCallHAL/btHAL.h"

void delay(void)	//@11.0592MHz
{
	__data static unsigned char i, j, k;

	i = 29;
	j = 14;
	k = 54;
	do
	{
		do
		{
			while (--k);
		} while (--j);
	} while (--i);
}


void main(void)
{
    I2Cbegin(-1);
	
	while(1)
	{
		I2CsetupTransmission(0x33, WRITE);
		I2Cwrite(0b00001111);
		I2CstartTransmission(1);
		
		delay();
		
		I2CsetupTransmission(0x33, WRITE);
		I2Cwrite(0b00000000);
		I2CstartTransmission(1);
		
		delay();
	}
}
