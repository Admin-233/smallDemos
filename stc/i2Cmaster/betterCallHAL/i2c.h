//#include <stdlib.h>

#define NULL 0

typedef enum
{
	WRITE = 0,
	READ = 1,
	UNKNOWN = 2,
} RWswitch;

typedef enum
{
	OK = 0,
	NACK = 1,
	TIMEOUT = 2,
} ERRORstatus;

void I2Cconfig(unsigned char speed, unsigned char timeout);
//void I2CsetBuffer(unsigned char TXsize, unsigned char RXsize);
void I2Cbegin(signed char addr);
void I2CsetupTransmission(unsigned char addr, RWswitch choice);
void I2Crequest(unsigned char quantity);
void I2Cwrite(unsigned char data);
unsigned char I2Cread(void);
unsigned char I2CstartTransmission(unsigned char sendStop);
unsigned char I2Cavailable(void);
void I2ConReceive(void (*userCallback)(void));
void I2ConRequest(void (*userCallback)(void));
void I2Cisr(void) __interrupt(24);