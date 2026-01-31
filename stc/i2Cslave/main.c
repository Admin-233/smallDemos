#include "stc8g.h"
static volatile char mask = 0b00000000;
static volatile char pins = 0b00000000;
__bit static volatile COMpin = 0;

void Timer0_Init(void)		//1毫秒@11.0592MHz
{
	IP |= 0b00000010;		//设置中断优先级为最高
	IPH |= 0b00000010;		//设置中断优先级为最高
	
	AUXR |= 0x80;			//定时器时钟1T模式
	TMOD &= 0xF0;			//设置定时器模式
	TL0 = 0xCD;				//设置定时初始值
	TH0 = 0xD4;				//设置定时初始值
	TF0 = 0;				//清除TF0标志
	TR0 = 1;				//定时器0开始计时
	ET0 = 1;				//使能定时器0中断
}

void main(void)
{
    P1M0 = 0x30; P1M1 = 0x30; //P14, P15, 开漏
	
	P3M0 = 0xff; P3M1 = 0x00; //P3, 全部推挽
    P5M0 |= 0x10; P5M1 &= ~0x10; //P54, 推挽

	Timer0_Init();
	
	//I2C
	
	IP2 |= 0b01000000;//设置中断优先级为1
	IP2H &= 0b10111111;//设置中断优先级为1
	
	P1 = 0b00110011;//地址检测脚(P10、P11)默认为高, 拉高SCL, SDA
	
	P_SW2 = 0b10000000;
	
	I2CCFG = 0b10000001;//启用i2c，从机模式，不设置速度
	
	I2CSLST = 0;//清除状态
	
	I2CSLADR = ((0b00110000 | (P1 & 0b00000011)) << 1) | 0b00000000;//设置地址，匹配地址
	
	I2CSLCR = 0b01111000;//开启所有中断，预备发送ack
	
	//I2C
	
	EA = 1;//开启总中断
	
	while(1){}
}



void Timer0_Isr(void) __interrupt(1)
{
	COMpin = !COMpin;
	COMpin ? (pins = 0xFF) : (pins = 0x00);
	pins ^= mask;
	
	P54 = COMpin;
	P3 = pins;
}

void I2Cisr(void) __interrupt(24)//mainly the logic of slave mode
{
	__bit static isFirstByte = 1;//default value
	
	if(I2CSLST & 0b01000000)//start
	{
		I2CSLST &= 0b10111111;
	}
	
	else if(I2CSLST & 0b00100000)//recvByte
	{
		I2CSLST &= 0b11011111;
		
		if(isFirstByte == 1)//process the first [address and R/W] byte.
		{
			isFirstByte = 0;
			if(I2CRXD & 0b00000001)//if master requested, prep data to send.
			{
				I2CTXD = mask;
			}
		}
		else
		{
			mask = I2CRXD;
		}
	}
	
	else if(I2CSLST & 0b00010000)//sendByte
	{
		I2CSLST &= 0b11101111;

		I2CTXD = mask;
	}
	
	else if(I2CSLST & 0b00001000)//stop
	{
		I2CSLST &= 0b11110111;
	}
}