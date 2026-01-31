/*
Due to the problem of designing
TXbufferPosition will increase 1 BEFORE read or write
RXbufferPosition will increase 1 AFTER  read or write
and the first byte of TXbuffer is used to storage the addr and R/W byte
so actually the real available space of TXbuffer to storage message to send is TXbufferLength - 1

This hal 可以让你以一次I2C事务作为单位处理数据传输.

Call I2Cbegin before everything.

When using the master mode, please follow the specified logic.
first calling I2CsetupTransmission
then I2Cwrite or I2Crequest
finally I2CstartTransmission
If two transmission needs to be stick together, then call I2CstartTransmission(false), and setup another Transmission
I2CstartTransmission will return the status of I2C transmission.
Also error handling only availble in master mode.
If the slave doesn't respond(nack), then the transmission will be stopped.

When using the slave mode, please define onReceiveCallback and onRequestCallback
using I2ConReceive() and I2ConRequest()
It's basically the the same as Arduino Wire, 
except for the two callbacks are both not taking any parameters or having any returns.

Due to the large storage space consumed by buffer re-allocate feature(nearly 2.5k bytes),
and the unstablity it has,
I2CsetBuffer and related variables has been disabled.
Please allocate fixed buffer manually,
by defining TXbufferLength and RXbufferLength.
*/

#include "stc8g.h"
#include "i2c.h"

static unsigned char I2Cspeed = 26;
static unsigned char I2Ctimeout = 1;

static unsigned char I2CerrorStatus = OK;//OK -- 0, NACK -- 1, TIMEOUT -- 2

/*//function disabled
static __xdata char* TXbuffer = NULL;
static __xdata char* RXbuffer = NULL;
static unsigned char TXbufferLength = 32;
static unsigned char RXbufferLength = 32;
*/

#ifndef TXbufferLength
#define TXbufferLength 32
#endif
#ifndef RXbufferLength
#define RXbufferLength 32
#endif
static char TXbuffer[TXbufferLength];
static char RXbuffer[RXbufferLength];

static unsigned char TXbufferPosition = 0;
static unsigned char RXbufferPosition = 0;

static unsigned char bytesQuantityToReceive = 0;//for master read

static unsigned char RXbufferAvailable = 0;
static unsigned char RXbufferPositionToRead = 0;

static void (*onReceiveCallback)(void) = NULL;
static void (*onRequestCallback)(void) = NULL;
static unsigned char RWstatus = UNKNOWN;//Read/Write status of MASTER.

void I2Cconfig(unsigned char speed, unsigned char timeout)
{
	I2Cspeed = speed;
	I2Ctimeout = timeout;
}

/*//function disabled
void I2CsetBuffer(unsigned char TXsize, unsigned char RXsize)
{
	TXbufferLength = TXsize;
	RXbufferLength = RXsize;
	
	RXbuffer = (__xdata char *)realloc(RXbuffer, RXbufferLength * sizeof(char));
	TXbuffer = (__xdata char *)realloc(RXbuffer, TXbufferLength * sizeof(char));	
}
*/

void I2Cbegin(signed char address)//正数作为从机，负数作为主机
{
	
	/*//function disabled
	if(RXbuffer == NULL || TXbuffer == NULL)
	{
		RXbuffer = (__xdata char *)calloc(RXbufferLength, sizeof(char));
		TXbuffer = (__xdata char *)calloc(RXbufferLength, sizeof(char));
	}
	*/
	
	P1M0 |= 0x30;//设置引脚为开漏模式
	P1M1 |= 0x30;
	
	P14 = 1;
	P15 = 1;
	
	P_SW2 = 0b10000000;
	
	if (address & 0b10000000)//主机模式
	{
		I2CCFG = (0b11000000 | I2Cspeed);//启用i2c，主机模式，设置速度
		
		I2CMSST = 0;//清除状态
		
		I2CMSCR = 0b00000000;//关闭中断，无命令
		
		I2CMSAUX = 0b00000000;//关闭自动发送
	}
	else//从机模式
	{	
		I2CCFG = 0b10000001;//启用i2c，从机模式，不设置速度
		
		I2CSLST = 0;//清除状态
		
		I2CSLADR = (address << 1) | 0x01;//设置地址，匹配地址
		
		EA = 1;//开启总中断
		
		I2CSLCR = 0b01111000;//开启所有中断，预备发送ack
	}
}

static void __waitForCompletion(void)
{
	__data static unsigned int timeout;
	timeout = 2 * 109 * I2Ctimeout;
	while(!(I2CMSST & 0b01000000))//check MSIF
	{
		timeout -= 1;
		if(timeout == 0)
		{
			I2CerrorStatus = TIMEOUT;
			break;
		}//check timeout
	}
	
	I2CMSST &= 0b10111111;//clear MSIF
}

void I2CsetupTransmission(unsigned char addr, RWswitch choice)//todo: buffer init
{
	RWstatus = choice;
	
	if(choice == WRITE)
	{
		I2CerrorStatus = OK;
		TXbufferPosition = 0;
		TXbuffer[TXbufferPosition] = (addr << 1) | choice;
	}
	else if(choice == READ)
	{
		TXbufferPosition = 0;
		TXbuffer[TXbufferPosition] = (addr << 1) | choice;
		
		RXbufferPosition = 0;
		bytesQuantityToReceive = 0;
		RXbufferAvailable = 0;
		RXbufferPositionToRead = 0;
	}
}

void I2Crequest(unsigned char quantity)//plan bytes to request
{
	if(bytesQuantityToReceive + quantity <= RXbufferLength)
	{
		bytesQuantityToReceive += quantity;
	}
}

void I2Cwrite(unsigned char data)//write data to buffer
{
	if(TXbufferPosition + 1 < TXbufferLength)
	{
		TXbufferPosition += 1;
		TXbuffer[TXbufferPosition] = data;
	}
}

unsigned char I2Cread(void)//read data from buffer
{
	if(RXbufferAvailable != 0)
	{
		unsigned char tmp = RXbuffer[RXbufferPositionToRead];
		RXbuffer[RXbufferPositionToRead] = 0x00;
		RXbufferAvailable -= 1;
		RXbufferPositionToRead += 1;
		return tmp;
	}
	else
	{
		return 0x00;
	}
}

unsigned char I2Cavailable(void)
{
	return RXbufferAvailable;
}

unsigned char I2CstartTransmission(unsigned char sendStop)//todo: if not stop transmission
{
	I2CMSST = 0;//clear flags
	
	I2CMSCR = 0b00000001;//start
	__waitForCompletion();
	
	if(RWstatus == WRITE)
	{
		for(unsigned char i = 0; i <= TXbufferPosition; i++)
		{
			//P10 = !P10;//debug
			I2CTXD = TXbuffer[i];
			I2CMSCR = 0b00001010;//send + recvack
			__waitForCompletion();
			if(I2CMSST & 0b00000010)//if nack
			{
				I2CerrorStatus = NACK;
				break;
			}
		}
	}
	if(RWstatus == READ)
	{
		I2CTXD = TXbuffer[0];//send addr and READ to slave
		I2CMSCR = 0b00001010;//send + recvack
		__waitForCompletion();
		
		for(unsigned char i = 0; i < bytesQuantityToReceive; i++)
		{
			if(bytesQuantityToReceive - i == 1)
			{
				I2CMSCR = 0b00001100;
			}
			else
			{
				I2CMSCR = 0b00001011;
			}
			__waitForCompletion();
			RXbuffer[RXbufferPosition] = I2CRXD;
			RXbufferPosition += 1;
		}
		RXbufferAvailable = RXbufferPosition;
		if(RXbufferPosition > 0)//if actually read somethng
		{
			RXbufferPosition -= 1;//locate it to the last byte
		}
	}
	
	if(sendStop != 0)
	{
		I2CMSCR = 0b00000110;
		__waitForCompletion();
	}
	RWstatus = UNKNOWN;
	
	return I2CerrorStatus;
}

void I2ConReceive(void (*userCallback)(void))
{
	onReceiveCallback = userCallback;
}

void I2ConRequest(void (*userCallback)(void))
{
	onRequestCallback = userCallback;
}

void I2Cisr(void) __interrupt(24)//mainly the logic of slave mode
{
	__bit static isFirstByte = 1;//default value
	__bit static isRestart = 0;  //default value
	
	if(I2CSLST & 0b01000000)//start
	{
		I2CSLST &= 0b10111111;
		
		if(isRestart == 1)//regard restart as a stop
		{
			RXbufferAvailable = RXbufferPosition;
			if(RXbufferPosition > 0)//if actually read somethng
			{
				RXbufferPosition -= 1;//locate it to the last byte
			}
			
			if(onReceiveCallback != NULL)
			{
				onReceiveCallback();
			}
		}
		isRestart = 1;
		
		isFirstByte = 1;
		TXbufferPosition = 0;
		RXbufferPosition = 0;
		RXbufferAvailable = 0;
		RXbufferPositionToRead = 0;
	}
	
	else if(I2CSLST & 0b00100000)//recvByte
	{
		I2CSLST &= 0b11011111;
		
		if(isFirstByte == 1)//process the first [address and R/W] byte.
		{
			isFirstByte = 0;
			RWstatus = I2CRXD & 0b00000001;
			if(RWstatus == READ)//if master requested, prep data to send.
			{
				if(onRequestCallback != NULL)
				{
					onRequestCallback();
				}
				TXbufferPosition = 1;//first byte of txbuffer is used to storage addr and RW, which is used in master WRITE and not required here
				I2CTXD = TXbuffer[TXbufferPosition];//get the first byte prepared to send
			}
		}
		else
		{
			if(RXbufferPosition < RXbufferLength)//boundary check
			{
				RXbuffer[RXbufferPosition] = I2CRXD;
				RXbufferPosition += 1;
			}
		}
	}
	
	else if(I2CSLST & 0b00010000)//sendByte
	{
		//P10 = !P10;//debug
		I2CSLST &= 0b11101111;

		if(TXbufferPosition + 1 < TXbufferLength)
		{
			TXbufferPosition += 1;
			I2CTXD = TXbuffer[TXbufferPosition];
			TXbuffer[TXbufferPosition] = 0x00;//reset data
		}
		else
		{
			I2CTXD = 0x00;
		}
	}
	
	else if(I2CSLST & 0b00001000)//stop
	{
		I2CSLST &= 0b11110111;
		
		isRestart = 0;
		
		RXbufferAvailable = RXbufferPosition;
		if(RXbufferPosition > 0)//if actually read somethng
		{
			RXbufferPosition -= 1;//locate it to the last byte
		}
		//P10 = !P10;//debug
		if(onReceiveCallback != NULL)
		{
			onReceiveCallback();
		}
	}
}