//展示了一个简单的模拟寄存器读写的i2c从机
//利用它更好的了解stc从机发送数据的时机，应该在收到主机请求后立即发送

#include "stc8g.h"

void main(void)
{
    P1M0 = 0x30; P1M1 = 0x30; //P14, P15, 开漏
	P1M0 |= 0x04; P1M1 &= ~0x04; 
	
	//I2C
	
	IP2 |= 0b01000000;//设置中断优先级为1
	IP2H &= 0b10111111;//设置中断优先级为1
	
	P1 = 0b00110011;//地址检测脚(P10、P11)默认为高, 拉高SCL, SDA
	
	P_SW2 = 0b10000000;
	
	I2CCFG = 0b10000001;//启用i2c，从机模式，不设置速度
	
	I2CSLST = 0;//清除状态
	
	I2CSLADR = 0b00000001;//设置地址，匹配地址
	
	I2CSLCR = 0b01111000;//开启所有中断，预备发送ack
	
	//I2C
	
	EA = 1;//开启总中断
	
	while(1){}
}

unsigned char regAddr;
unsigned char simReg1;
unsigned char simReg2;

void I2Cisr(void) __interrupt(24)//mainly the logic of slave mode
{
	static unsigned char recvByteCount = 0;
	
	if(I2CSLST & 0b01000000)//start
	{
		recvByteCount = 1;
		
		I2CSLST &= 0b10111111;
	}
	
	else if(I2CSLST & 0b00100000)//recvByte DONE!
	{
		if(recvByteCount == 1)//process the first [address and R/W] byte.
		{
			if(I2CRXD & 0b00000001)//master read
			{
				if(regAddr == 1)
				{
					I2CTXD = simReg1;
				}
				else if(regAddr == 2)
				{
					I2CTXD = simReg2;
				}
				else
				{
					I2CTXD = 0xCA;
				}
			}
			else//master write
			{
				recvByteCount = 2;
			}
		}
		else if(recvByteCount == 2)// if the second recv byte
		{
			regAddr = I2CRXD;
			recvByteCount = 3;
		}
		else if(recvByteCount == 3)// third
		{
			if(regAddr == 1)
			{
				P12 = !P12;
				simReg1 = I2CRXD;
			}
			else
			{
				simReg2 = I2CRXD;
			}
			
			recvByteCount = 4;
		}
		
		I2CSLST &= 0b11011111;
	}
	
	else if(I2CSLST & 0b00010000)//sendByte DONE!
	{
		I2CSLST &= 0b11101111;
	}
	
	else if(I2CSLST & 0b00001000)//stop
	{
		I2CSLST &= 0b11110111;
	}
}