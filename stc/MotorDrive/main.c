#include "stc8g.h"

//应该改用FG信号线测速，n=60f/P，P数为5
//此程序作废，作为示例程序，演示如何驱动及结合各个外设

void delay(unsigned int ms);

/////////////////Basic functions

void IOInit(void)
{
	P3M0 |= 0x08; P3M1 &= ~0x08; //P33 推挽
	P5M0 &= ~0x20; P5M1 |= 0x20; //P55高阻
	
    P3M0 &= ~0x03; P3M1 &= ~0x03; //P30,P31准双向，仅供测试
}

void PCAInit(void)
{
	CCON = 0b00000000;
	CMOD = 0b00001000;//直接输入系统时钟，10.24mhz（实际10.125左右，pwm输出最接近10khz）
	CCAPM1 = 0b01000010;//允许PCA模块1的比较功能和PWN功能
	
	PCA_PWM1 = 0b00000000;//8位PWM
}

void PCAConfig(unsigned short value)//占空比配置，数值上升低电平增加，0为高电平256为低电平
{
	PCA_PWM1 &= 0b11001101;
	PCA_PWM1 |= (value >> 7) & 0b00000010;
	
	CCAP1H = value & 0x00FF;
}

void PCAStart(void)
{
	CCON |= 0b01000000;
}

void PCAStop(void)
{
	CCON &= 0b10111111;
}

void ADCInit(void)
{
	//stc8g1k08a的adc引脚编号并不按照stc8g1k08系列来，P3.0/ADC0:0000  P3.1/ADC1:0001  P3.2/ADC2:0010  P3.3/ADC3:0011  P5.4/ADC4:0100  P5.5/ADC5:0101
	ADC_CONTR = 0b10000100;	//打开ADC，设置端口为P54
	ADCCFG = 0b00100000;	//数据右对齐，速度设置为SYSclk/2/1
}

unsigned short ADCRead(void)
{
	ADC_CONTR |= 0b01000000;			//开始转换
	while(! (ADC_CONTR & 0b00100000) );	//读取标志位
	ADC_CONTR &= 0b11011111;			//清除标志位
	
	return (ADC_RES << 8) | ADC_RESL;	//返回10位结果
}

void Timer0_Init(void)		//100us@10.125MHz
{
	AUXR |= 0x80;			//定时器时钟1T模式
	TMOD &= 0xF0;			//设置定时器模式
	TL0 = 0x0C;				//设置定时初始值
	TH0 = 0xFC;				//设置定时初始值
	TF0 = 0;				//清除TF0标志
	ET0 = 1;				//使能定时器0中断
	
	EA = 1;					//开总中断
}

void UART1_Timer1_Init(void)	//115200bps@10.125MHz
{
	AUXR |= 0x40;		//定时器时钟1T模式
	TMOD &= 0x0F;		//设置定时器模式
	TL1 = 0xEA;			//设置定时初始值
	TH1 = 0xFF;			//设置定时初始值
	ET1 = 0;			//禁止定时器中断
	TR1 = 1;			//定时器1开始计时
}

void UART1_Init(void)
{
	SCON = 0b01000000;	//模式1可变波特率8为数据，不允许串口接收，不启用第九位数据，中断位置0
	PCON &= 0b00111111;	//波特率不加倍，不启用帧错误检测
	AUXR &= 0b11011110;	//（模式0波特率不加倍，不影响），选择定时器1作为波特率发生器
}

void UART1_SendByte(unsigned char byte)
{
	SBUF = byte;	//数据发送寄存器
	while(! (SCON & 0b00000010) );
	SCON &= 0b11111101;
}

void UART1_SendIntinASCII(unsigned short num)
{
    UART1_SendByte(num / 10000 + '0');
    UART1_SendByte(num % 10000 / 1000 + '0');
    UART1_SendByte(num % 1000 / 100 + '0');
    UART1_SendByte(num % 100 / 10 + '0');
    UART1_SendByte(num % 10 + '0');

    UART1_SendByte('\r');
    UART1_SendByte('\n');
}

/////////////Advanced functions

void setDuty(unsigned short duty)//高电平时间占比(占空比)，范围0-1000
{
	PCAConfig((unsigned short)(((unsigned long)duty * 256 + 500) / 1000));
}

volatile unsigned short int clockTick;//counting in 100us/0.1ms
volatile unsigned char clockOverflow;// set to 1 when clockTick overflowed

void resetClock(void)//both reset and start
{
	TR0 = 0;
	TL0 = 0x0C;				//设置定时初始值
	TH0 = 0xFC;				//设置定时初始值
	clockTick = 0;
	clockOverflow = 0;
	TR0 = 1;				//定时器0开始计时
}

unsigned short getSpeed(void)//return speed in RPM
{
	enum Status {WAITING, CHECKDOWN, CHECKUP} status = CHECKDOWN;
	unsigned short ADCValue, ADCRawValue, timeWaitingCache, timeSpeedCalCache1, timeSpeedCalCache2;
	unsigned char checkCount = 0;
	
	ADCValue = 850;
	
	resetClock();
	
	while(1)
	{
		ADCRawValue = ADCRead();
		if(ADCRawValue >= 700){ADCValue = ADCRawValue;}//only update when value >= 700
		
		if(status == WAITING)
		{
			if(clockTick - timeWaitingCache > 100)//delay 10ms before next checking.
			{status = CHECKDOWN;}
			else
			{continue;}
		}
		
		if(ADCValue < 800 && status == CHECKDOWN)
		{
			timeWaitingCache = clockTick;
			status = CHECKUP;
		}
		
		if(ADCValue > 850 && status == CHECKUP)
		{
			if(checkCount == 0){timeSpeedCalCache1 = clockTick;}
			
			if(checkCount == 2)//cal and return speed here.
			{
				timeSpeedCalCache2 = clockTick - timeSpeedCalCache1;
				//return (unsigned short)(600000 / (timeSpeedCalCache2 / 2.0f));//浮点数慢慢算
				return (unsigned short)(600000 / (timeSpeedCalCache2 >> 1));//极致优化，结果会偏大
			}
			else
			{
				checkCount++;
				status = WAITING;
			}
		}
	}
}

//////////////MAIN
void main(void)
{
    IOInit();
	PCAInit();
	ADCInit();
	UART1_Init();
	UART1_Timer1_Init();
	Timer0_Init();
	
	setDuty(0);
	PCAStart();
	
	resetClock();
	
	unsigned short value = 0;
	while(1)
	{
		setDuty(400);
		delay(1000);
		UART1_SendIntinASCII(getSpeed());
	}
}

void delay(unsigned int ms)
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

void Timer0_Isr(void) __interrupt(1)
{
	if(clockTick == 0xFFFF)
	{
		TR0 = 0;				//stop Timer
		clockOverflow = 1;
		return;
	}
	
	clockTick++;
}