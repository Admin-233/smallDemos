//P3连接到了一个r2r dac，此处的软件是单片机接收器，以及send.py发送器，发送46080hz采样率的pcm文件，以串口作为时钟。
#include "stc8g.h"

unsigned char rx_data;
unsigned char rx_flag;

void SendBinString(unsigned char dat);
void delay(unsigned short ms);

void Uart2_Init(void)	//460800bps@11.0592MHz
{
	S2CON = 0x50;		//8位数据,可变波特率
	AUXR |= 0x04;		//定时器时钟1T模式
	T2L = 0xFA;			//设置定时初始值
	T2H = 0xFF;			//设置定时初始值
	AUXR |= 0x10;		//定时器2开始计时
	IE2 |= 0x01;		//使能串口2中断
}

void main(void)
{
    P3M0 = 0xff; P3M1 = 0x00; 
    P1M0 = (P1M0 & ~0x01) | 0x02; P1M1 = (P1M1 & ~0x02) | 0x01; //P11推挽 P10高阻
	
    P1M0 |= 0x04; P1M1 &= ~0x04; //P12推挽 Debug
	
	Uart2_Init();
	EA = 1;
	
	P12 = 0;
	
    while (1)
    {
        // if (rx_flag)
        // {
            // rx_flag = 0;
            // SendBinString(rx_data);
        // }
    }
}

// void SendBinString(unsigned char dat)
// {
    // unsigned char i;

    // for (i = 0; i < 8; i++)
    // {
        // if (dat & 0x80)
            // S2BUF = '1';
        // else
            // S2BUF = '0';

        // dat <<= 1;

        // while (!(S2CON & 0x02));   // 等待发送完成
        // S2CON &= ~0x02;             // 清发送完成标志
    // }
	// S2BUF='\n';
	// while (!(S2CON & 0x02));
	// S2CON &= ~0x02;
	// S2BUF='\r';
	// while (!(S2CON & 0x02));
	// S2CON &= ~0x02;
// }

void Uart2_Isr(void) __interrupt(8)
{
    if (S2CON & 0x01)
    {
        S2CON &= ~0x01;

        rx_data = S2BUF;
        P3 = rx_data;
        P12 = 1;
    }
}

void delay(unsigned short ms)	//@11.0592MHz
{
	static unsigned char __data j;
	static unsigned int __data i;

	i = 15 * ms;
	j = 90;
	do
	{
		while (--j);
	} while (--i);
}
