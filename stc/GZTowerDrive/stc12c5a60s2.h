#ifndef __STC12C5A60S2_H__
#define __STC12C5A60S2_H__

/////////////////////////////////////////////////

//包含本头文件后,不用另外再包含"REG51.H"

__sfr       __at(0x80)      P0;
    __sbit  __at(0x80)      P00;
    __sbit  __at(0x81)      P01;
    __sbit  __at(0x82)      P02;
    __sbit  __at(0x83)      P03;
    __sbit  __at(0x84)      P04;
    __sbit  __at(0x85)      P05;
    __sbit  __at(0x86)      P06;
    __sbit  __at(0x87)      P07;

__sfr       __at(0x81)      SP;
__sfr       __at(0x82)      DPL;
__sfr       __at(0x83)      DPH;
__sfr       __at(0x87)      PCON;

__sfr       __at(0x88)      TCON;
    __sbit  __at(0x8F)      TF1;
    __sbit  __at(0x8E)      TR1;
    __sbit  __at(0x8D)      TF0;
    __sbit  __at(0x8C)      TR0;
    __sbit  __at(0x8B)      IE1;
    __sbit  __at(0x8A)      IT1;
    __sbit  __at(0x89)      IE0;
    __sbit  __at(0x88)      IT0;

__sfr       __at(0x89)      TMOD;
__sfr       __at(0x8A)      TL0;
__sfr       __at(0x8B)      TL1;
__sfr       __at(0x8C)      TH0;
__sfr       __at(0x8D)      TH1;
__sfr       __at(0x8E)      AUXR;
__sfr       __at(0x8F)      WAKE_CLKO;

__sfr       __at(0x90)      P1;
    __sbit  __at(0x90)      P10;
    __sbit  __at(0x91)      P11;
    __sbit  __at(0x92)      P12;
    __sbit  __at(0x93)      P13;
    __sbit  __at(0x94)      P14;
    __sbit  __at(0x95)      P15;
    __sbit  __at(0x96)      P16;
    __sbit  __at(0x97)      P17;

__sfr       __at(0x91)      P1M1;
__sfr       __at(0x92)      P1M0;
__sfr       __at(0x93)      P0M1;
__sfr       __at(0x94)      P0M0;
__sfr       __at(0x95)      P2M1;
__sfr       __at(0x96)      P2M0;
__sfr       __at(0x97)      CLK_DIV;

__sfr       __at(0x98)      SCON;
    __sbit  __at(0x9F)      SM0;
    __sbit  __at(0x9E)      SM1;
    __sbit  __at(0x9D)      SM2;
    __sbit  __at(0x9C)      REN;
    __sbit  __at(0x9B)      TB8;
    __sbit  __at(0x9A)      RB8;
    __sbit  __at(0x99)      TI;
    __sbit  __at(0x98)      RI;

__sfr       __at(0x99)      SBUF;
__sfr       __at(0x9A)      S2CON;
__sfr       __at(0x9B)      S2BUF;
__sfr       __at(0x9C)      BRT;
__sfr       __at(0x9D)      P1ASF;

__sfr       __at(0xA0)      P2;
    __sbit  __at(0xA0)      P20;
    __sbit  __at(0xA1)      P21;
    __sbit  __at(0xA2)      P22;
    __sbit  __at(0xA3)      P23;
    __sbit  __at(0xA4)      P24;
    __sbit  __at(0xA5)      P25;
    __sbit  __at(0xA6)      P26;
    __sbit  __at(0xA7)      P27;

__sfr       __at(0xA1)      BUS_SPEED;
__sfr       __at(0xA2)      AUXR1;

__sfr       __at(0xA8)      IE;
    __sbit  __at(0xAF)      EA;
    __sbit  __at(0xAE)      ELVD;
    __sbit  __at(0xAD)      EADC;
    __sbit  __at(0xAC)      ES;
    __sbit  __at(0xAB)      ET1;
    __sbit  __at(0xAA)      EX1;
    __sbit  __at(0xA9)      ET0;
    __sbit  __at(0xA8)      EX0;
    
__sfr       __at(0xA9)      SADDR;
__sfr       __at(0xAF)      IE2;

__sfr       __at(0xB0)      P3;
    __sbit  __at(0xB0)      P30;
    __sbit  __at(0xB1)      P31;
    __sbit  __at(0xB2)      P32;
    __sbit  __at(0xB3)      P33;
    __sbit  __at(0xB4)      P34;
    __sbit  __at(0xB5)      P35;
    __sbit  __at(0xB6)      P36;
    __sbit  __at(0xB7)      P37;

    __sbit  __at(0xB7)      RD;
    __sbit  __at(0xB6)      WR;
    __sbit  __at(0xB5)      T1;
    __sbit  __at(0xB4)      T0;
    __sbit  __at(0xB3)      INT1;
    __sbit  __at(0xB2)      INT0;
    __sbit  __at(0xB1)      TXD;
    __sbit  __at(0xB0)      RXD;

__sfr       __at(0xB1)      P3M1;
__sfr       __at(0xB2)      P3M0;
__sfr       __at(0xB3)      P4M1;
__sfr       __at(0xB4)      P4M0;

__sfr       __at(0xB5)      IP2;
__sfr       __at(0xB6)      IPH2;
__sfr       __at(0xB7)      IPH;

__sfr       __at(0xB8)      IP;
    __sbit  __at(0xBF)      PPCA;
    __sbit  __at(0xBE)      PLVD;
    __sbit  __at(0xBD)      PADC;
    __sbit  __at(0xBC)      PS;
    __sbit  __at(0xBB)      PT1;
    __sbit  __at(0xBA)      PX1;
    __sbit  __at(0xB9)      PT0;
    __sbit  __at(0xB8)      PX0;

__sfr       __at(0xB9)      SADEN;
__sfr       __at(0xBB)      P4SW;

__sfr       __at(0xBC)      ADC_CONTR;
__sfr       __at(0xBD)      ADC_RES;
__sfr       __at(0xBE)      ADC_RESL;

__sfr       __at(0xC0)      P4;
    __sbit  __at(0xC0)      P40;
    __sbit  __at(0xC1)      P41;
    __sbit  __at(0xC2)      P42;
    __sbit  __at(0xC3)      P43;
    __sbit  __at(0xC4)      P44;
    __sbit  __at(0xC5)      P45;
    __sbit  __at(0xC6)      P46;
    __sbit  __at(0xC7)      P47;

__sfr       __at(0xC1)      WDT_CONTR;
__sfr       __at(0xC2)      IAP_DATA;
__sfr       __at(0xC3)      IAP_ADDRH;
__sfr       __at(0xC4)      IAP_ADDRL;
__sfr       __at(0xC5)      IAP_CMD;
__sfr       __at(0xC6)      IAP_TRIG;
__sfr       __at(0xC7)      IAP_CONTR;

__sfr       __at(0xC8)      P5;
    __sbit  __at(0xC8)      P50;
    __sbit  __at(0xC9)      P51;
    __sbit  __at(0xCA)      P52;
    __sbit  __at(0xCB)      P53;

__sfr       __at(0xC9)      P5M1;
__sfr       __at(0xCA)      P5M0;

__sfr       __at(0xCD)      SPSTAT;
__sfr       __at(0xCE)      SPCTL;
__sfr       __at(0xCF)      SPDAT;

__sfr       __at(0xD0)      PSW;
    __sbit  __at(0xD7)      CY;
    __sbit  __at(0xD6)      AC;
    __sbit  __at(0xD5)      F0;
    __sbit  __at(0xD4)      RS1;
    __sbit  __at(0xD3)      RS0;
    __sbit  __at(0xD2)      OV;
    __sbit  __at(0xD1)      F1;
    __sbit  __at(0xD0)      P;

__sfr       __at(0xD8)      CCON;
    __sbit  __at(0xDF)      CF;
    __sbit  __at(0xDE)      CR;
    __sbit  __at(0xD9)      CCF1;
    __sbit  __at(0xD8)      CCF0;

__sfr       __at(0xD9)      CMOD;
__sfr       __at(0xDA)      CCAPM0;
__sfr       __at(0xDB)      CCAPM1;

__sfr       __at(0xE0)      ACC;
    
__sfr       __at(0xE9)      CL;
__sfr       __at(0xEA)      CCAP0L;
__sfr       __at(0xEB)      CCAP1L;

__sfr       __at(0xF0)      B;

__sfr       __at(0xF2)      PCA_PWM0;
__sfr       __at(0xF3)      PCA_PWM1;

__sfr       __at(0xF9)      CH;
__sfr       __at(0xFA)      CCAP0H;
__sfr       __at(0xFB)      CCAP1H;

#endif

