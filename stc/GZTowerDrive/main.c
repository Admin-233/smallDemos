#include "stc12c5a60s2.h"

#define _nop_() __asm nop __endasm

static unsigned short int __xdata frameBuf[16];

void delay(unsigned short int ms);

void ioInit(void)
{
	P4SW=0xFF;//启用P4
	
    P0M0 = 0xff; P0M1 = 0x00; //行IO全部推挽
    P1M0 = 0xff; P1M1 = 0x00; 
    //P3M0 |= 0x04; P3M1 &= ~0x04; 
    P4M0 |= 0xe0; P4M1 &= ~0xe0; 
	
    P3M0 |= 0xfb; P3M1 |= 0xfb; //列IO全部开漏
    P2M0 = 0xff; P2M1 = 0xff; 
    P4M0 |= 0x10; P4M1 |= 0x10; 
	
	P0=0;//行
	P1=0;
	P32=0;
	P45=0;
	P46=0;
	P47=0;
	
	P3=0;//列
	P2=0;
	P44=0;
}

void Timer0_Init(void)//100微秒@12.000MHz
{
	EA = 1;
	AUXR |= 0x80;			//定时器时钟1T模式
	TMOD &= 0xF0;			//设置定时器模式
	TL0 = 0x50;				//设置定时初始值
	TH0 = 0xFB;				//设置定时初始值
	TF0 = 0;				//清除TF0标志
	TR0 = 1;				//定时器0开始计时
	ET0 = 1;				//使能定时器0中断
}

void configRow(unsigned short int config)
{
	P0  = config & 0b11111111;
	P46 = (config >> 8) & 0b00000001;
	P1  = (config >> 8) & 0b11111110;
}

void configCol(unsigned short int config)
{
	P3 = ~((config & 0b00000011) | ((config & 0b11111000) << 1));
	
	P2 = ~((config >> 7) & 0b11111111);

	P4 = ~((P4M0 & ~(1 << 4)) | (((config >> 15) & 1) << 4));
}

#define topOff(); 	P3M0 &= ~0x04; P3M1 |= 0x04;
#define topOn();	P3M0 |= 0x04; P3M1 &= ~0x04; 

void frameTransition1(unsigned short int target[16])//以行过度
{
    unsigned char i;
    for (i = 0; i < 16; i++)
    {
        // 第一步：该行全部点亮
        frameBuf[i] = 0xFFFF;
        delay(8);

        // 第二步：更新为目标内容
        frameBuf[i] = target[i];
        delay(7);
    }
}

void frameTransition2(unsigned short int target[16])//以列过度
{
	unsigned char col, row;
    for (col = 0; col < 16; col++) {               // 遍历每一列
        unsigned short mask = 1 << col;            // 该列对应的位掩码
        for (row = 0; row < 16; row++) {
            if (target[row] & mask)
                frameBuf[row] |= mask;             // 目标为1，置位
            else
                frameBuf[row] &= ~mask;            // 目标为0，清零
        }
        delay(15);                                 // 每列之间延时15ms
    }
}

void frameTransition3(unsigned short int target[16])//以行过度,过度1的反向
{
    unsigned char i;
    for (i = 0; i < 16; i++)
    {
        // 第一步：该行全部点亮
        frameBuf[i] = 0xFFFF;
        delay(8);

        // 第二步：更新为目标内容
        frameBuf[15-i] = target[15-i];
        delay(7);
    }
}

void topAnimation0(void)
{
	for(char _ = 0; _ < 2; _++)
	{
		topOff();
		delay(30);
		topOn();
		delay(30);
	}
}

void animation1(void)
{	
	const unsigned short wave[] =
	{
		0x0180,
		0x03C0,
		0x07E0,
		0x0FF0,
		0x1FF8,
		0x0FF0,
		0x07E0,
		0x03C0
	};

	unsigned char phase = 0;
	
	
	for(unsigned char _ = 0; _ < 128; _++)
	{
		for(unsigned char y=0;y<16;y++)
		{
			unsigned short row;

			row = wave[(y + phase) & 7];

			unsigned char shift =
				(phase + y) & 15;

			row =
				(row << shift) |
				(row >> (16 - shift));

			row &= ~(1 << 0);
			row &= ~(1 << 2);

			frameBuf[y] = row;
		}

		phase++;

		delay(39);
	}
}

void animation2(void)
{
	const unsigned short firstFrame[16] =
	{
		0x0200, // row 0  –  col 9
		0x1000, // row 1  –  col 12
		0x8000, // row 2  –  col 15
		0x0004, // row 3  –  col 2
		0x0020, // row 4  –  col 5
		0x0000, // row 5  –  无雨滴
		0x0800, // row 6  –  col 11
		0x0000, // row 7  –  无雨滴
		0x0002, // row 8  –  col 1
		0x0010, // row 9  –  col 4
		0x0080, // row 10 –  col 7
		0x0400, // row 11 –  col 10
		0x0000, // row 12 –  无雨滴
		0x0001, // row 13 –  col 0
		0x0000, // row 14 –  无雨滴
		0x0040  // row 15 –  col 6
	};
	frameTransition1(firstFrame);

#define RAIN_DROPS 12   // 雨滴数量，编译前修改此处即可
#define RAIN_FRAMES 40  // 动画总帧数（可一并调整）
unsigned char rainDelay = 50;


    struct Drop {
        unsigned char col;  // 0~15
        char row;           // -1 未激活，0~15 有效
    };
    struct Drop drops[RAIN_DROPS];
    unsigned char i, f;

    // 初始化雨滴：根据编号散布在不同列和行（简单伪随机）
    for (i = 0; i < RAIN_DROPS; i++) {
        drops[i].col = (i * 5 + 2) % 16;      // 不同列
        drops[i].row = (i * 7 + 3) % 16;      // 不同起始行，避免完全同步
    }

	unsigned char count = 0;
	unsigned int count2 = 0;
	char isTopOff = 0;
	for (unsigned short int _ = 0; _<512; _++)
	{
		for (f = 0; f < RAIN_FRAMES; f++) {
			// 清屏
			for (i = 0; i < 16; i++) {
				frameBuf[i] = 0;
			}

			// 绘制所有雨滴
			for (i = 0; i < RAIN_DROPS; i++) {
				if (drops[i].row >= 0 && drops[i].row < 16) {
					frameBuf[drops[i].row] |= (1 << drops[i].col);
				}
			}

			// 更新位置：下落
			for (i = 0; i < RAIN_DROPS; i++) {
				drops[i].row++;
				if (drops[i].row >= 16) {         // 落出屏幕，回到顶部换列
					drops[i].row = 0;
					drops[i].col = (drops[i].col * 5 + 3) % 16; // 换列公式
				}
			}
			
			#define minDelay 8
			count++;
			if(count == 2){count = 0;if(rainDelay > minDelay){rainDelay--;}}
			if(rainDelay <= minDelay){count2++;}
			if(count2 >= 600){count2 = 0;goto outLabel;}
			
			
			if(isTopOff){topOn();isTopOff=0;}else{topOff();isTopOff=1;}
			
			
			delay(rainDelay);
		}
	}
	outLabel:
}

void animation3(void)
{
	const unsigned short firstFrame[16] =
	{
    0x8000, // 行 0（偶） col = 15
    0x0002, // 行 1（奇） col = 1
    0x2000, // 行 2（偶） col = 13
    0x0008, // 行 3（奇） col = 3
    0x0800, // 行 4（偶） col = 11
    0x0020, // 行 5（奇） col = 5
    0x0200, // 行 6（偶） col = 9
    0x0080, // 行 7（奇） col = 7
    0x0080, // 行 8（偶） col = 7
    0x0200, // 行 9（奇） col = 9
    0x0020, // 行10（偶） col = 5
    0x0800, // 行11（奇） col = 11
    0x0008, // 行12（偶） col = 3
    0x2000, // 行13（奇） col = 13
    0x0002, // 行14（偶） col = 1
    0x8000  // 行15（奇） col = 15
	};
	frameTransition2(firstFrame);
	topAnimation0();
	
    unsigned char posA = 0;   // 顺时针（右移）
    unsigned char posB = 15;  // 逆时针（左移）

    unsigned char y;
	
    for(short int i = 0; i<512; i++)
	{






        for (y = 0; y < 16; y++)
        {
            unsigned char col;

            if (y & 1)
            {
                /* 奇数行：斜线A */
                col = (posA + y) & 0x0F;   // %16
            }
            else
            {
                /* 偶数行：斜线B */
                col = (posB + 16 - y) & 0x0F;
            }

            frameBuf[y] = (1U << col);
        }

        delay(15);

        /* A向右循环 */
        posA = (posA + 1) & 0x0F;

        /* B向左循环 */
        posB = (posB - 1) & 0x0F;




	}
}

void main(void)
{
    ioInit();
	
	Timer0_Init();
	
	P47 = 1;//底
	P45 = 1;//顶
	P32 = 1;//中
	//这些设置没什么用

while(1)
{
	topAnimation0();
	
	animation2();
	
	animation3();
	
	const unsigned short firstFrame[16] =
	{
		0x0180,
		0x0780,
		0x1F80,
		0x7F80,
		0xFF80,
		0xFE00,
		0xF800,
		0xE000,

		0x8000,
		0x8002,
		0x801A,
		0x807A,
		0x81FA,
		0x01FA,
		0x81F8,
		0x01E0
	};
	frameTransition2(firstFrame);//过渡到首帧
	topAnimation0();
	
	while(1)//最后循环执行动画1
	{
		animation1();
	}
}

}


void delay(unsigned short int ms)
{
    volatile static __data unsigned char j;
    volatile static __data unsigned int i;

	_nop_();
	_nop_();
	i = 12 * ms;
	j = 168;
	do
	{
		while (--j);
	} while (--i);
}

void Timer0_Isr(void) __interrupt(1)
{
	static unsigned char currentRow = 0;
	
	configRow(0);
	
	configCol(frameBuf[currentRow] & 0b1111111111111010);//过滤坏列
	
	configRow(1 << currentRow);
	
	currentRow++;
	
	if(currentRow >= 16){currentRow = 0;}
}