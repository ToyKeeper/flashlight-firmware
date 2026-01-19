/*--------------------------------------------------------------------------
SN8F5701.H

Header file for SNOiX SN8F5701 microcontroller.
Copyright (c) 2016 SONiX Technology Co., Ltd.

Version 1.1    2016-12-21
--------------------------------------------------------------------------*/

#ifndef __SN8F5701_H__
#define __SN8F5701_H__

/*      Register      */
__sfr     __at (0x80)    P0;
  __sbit  __at (0x80+5)  P05;
  __sbit  __at (0x80+4)  P04;
  __sbit  __at (0x80+3)  P03;
  __sbit  __at (0x80+2)  P02;
  __sbit  __at (0x80+1)  P01;
  __sbit  __at (0x80+0)  P00;
__sfr     __at (0x81)    SP;
__sfr     __at (0x82)    DPL;
__sfr     __at (0x83)    DPH;
__sfr     __at (0x86)    WDTR;
__sfr     __at (0x87)    PCON;
__sfr     __at (0x88)    TCON;
  __sbit  __at (0x88+7)  TF1;
  __sbit  __at (0x88+6)  TR1;
  __sbit  __at (0x88+5)  TF0;
  __sbit  __at (0x88+4)  TR0;
  __sbit  __at (0x88+3)  IE1;
  __sbit  __at (0x88+1)  IE0;
__sfr     __at (0x89)    TMOD;
__sfr     __at (0x8A)    TL0;
__sfr     __at (0x8B)    TL1;
__sfr     __at (0x8C)    TH0;
__sfr     __at (0x8D)    TH1;
__sfr     __at (0x8E)    CKCON;
__sfr     __at (0x8F)    PEDGE;
__sfr     __at (0x93)    DPC;
__sfr     __at (0x94)    PECMD;
__sfr16   __at(((0x95+1U)<<8) | 0x95) PEROM;
__sfr     __at (0x95)    PEROML;
__sfr     __at (0x96)    PEROMH;
__sfr     __at (0x97)    PERAM;
__sfr     __at (0x98)    S0CON;
  __sbit  __at (0x98+7)  SM0;
  __sbit  __at (0x98+6)  SM1;
  __sbit  __at (0x98+5)  SM20;
  __sbit  __at (0x98+4)  REN0;
  __sbit  __at (0x98+3)  TB80;
  __sbit  __at (0x98+2)  RB80;
  __sbit  __at (0x98+1)  TI0;
  __sbit  __at (0x98+0)  RI0;
__sfr     __at (0x99)    S0BUF;
__sfr     __at (0x9A)    IEN2;
__sfr     __at (0x9E)    P0CON;
__sfr			__at (0xA1)    T3M;
__sfr			__at (0xA2)    T3CL;
__sfr			__at (0xA3)    T3CH;
__sfr16   __at (((0xA4+1U)<<8) | 0xA4) T3Y;
__sfr			__at (0xA4)    T3YL;
__sfr			__at (0xA5)    T3YH;
__sfr16   __at (((0xA6+1U)<<8) | 0xA6) PW0D;
__sfr			__at (0xA6)    PW0DL;
__sfr			__at (0xA7)    PW0DH;
__sfr     __at (0xA8)    IEN0;
  __sbit   __at (0xA8+7) EAL;
  __sbit   __at (0xA8+4) ES0;
  __sbit   __at (0xA8+3) ET1;
	__sbit   __at (0xA8+2) EX1;	
  __sbit   __at (0xA8+1) ET0;
  __sbit   __at (0xA8+0) EX0;
__sfr     __at (0xA9)    IP0;
__sfr     __at (0xAA)    S0RELL;
__sfr16   __at (((0xAB+1U)<<8) | 0xAB) PW1D;
__sfr     __at (0xAB)    PW1DL;
__sfr		  __at (0xAC)    PW1DH;
__sfr16   __at (((0xAD+1U)<<8) | 0xAD) PW2D;
__sfr     __at (0xAD)    PW2DL;
__sfr     __at (0xAE)    PW2DH;
__sfr16   __at (((0xB1+1U)<<8) | 0xB1) PW3D;
__sfr     __at (0xB1)    PW3DL;
__sfr     __at (0xB2)    PW3DH;
__sfr16   __at (((0xB3+1U)<<8) | 0xB3) PW4D;
__sfr     __at (0xB3)    PW4DL;
__sfr     __at (0xB4)    PW4DH;
__sfr16   __at (((0xB5+1U)<<8) | 0xB5) PW5D;
__sfr     __at (0xB5)    PW5DL;
__sfr     __at (0xB6)    PW5DH;
__sfr     __at (0xB9)    IP1;
__sfr     __at (0xBA)    S0RELH;
__sfr     __at (0xBC)    PWNV;
__sfr     __at (0xBD)    PWO;
__sfr     __at (0xBE)    PWCH;
__sfr     __at (0xBF)    IRCON2;
__sfr     __at (0xD0)    PSW;
  __sbit  __at (0xD0+7)  CY;
  __sbit  __at (0xD0+6)  AC;
  __sbit  __at (0xD0+5)  F0;
  __sbit  __at (0xD0+4)  RS1;
  __sbit  __at (0xD0+3)  RS0;
  __sbit  __at (0xD0+2)  OV;
  __sbit  __at (0xD0+1)  F1;
  __sbit  __at (0xD0+0)  P;
__sfr     __at (0xD2)    ADM;
__sfr     __at (0xD3)    ADB;
__sfr     __at (0xD4)    ADR;
__sfr     __at (0xD5)    VREFH;
__sfr     __at (0xD8)    S0CON2;
  __sbit  __at (0xD8+7)  BD;
__sfr     __at (0xE0)    ACC;
__sfr     __at (0xE4)    P0OC;
__sfr     __at (0xE5)    CLKSEL;
__sfr     __at (0xE6)    CLKCMD;
__sfr     __at (0xE7)    TCON0;
__sfr     __at (0xF0)    B;
__sfr     __at (0xF1)    P0UR;
__sfr16   __at (((0xF2+1U)<<8) | 0xF2) FRQ;
__sfr     __at (0xF2)    FRQL;
__sfr     __at (0xF3)    FRQH;
__sfr     __at (0xF4)    FRQCMD;
__sfr     __at (0xF7)    SRST;
__sfr     __at (0xF9)    P0M;
__sfr     __at (0xFF)    PFLAG;

/*      Interrupt Vector      */
#define ISRInt0    0
#define ISRTimer0  1
#define ISRInt1    2
#define ISRTimer1  3
#define ISRUart    4
#define ISRInt2    16
#define ISRAdc     17
#define ISRTimer3  29

/*      SDCC Macros      */
#ifdef __SDCC
#define NOP() \
  __asm \
    nop \
  __endasm
#define IDLE() {PCON |= 0x01;}
#define STOP() {PCON |= 0x02;}
#define PISP(ROM_ADDRESS, RAM_ADDRESS) {PERAM = (RAM_ADDRESS); PEROM = ((ROM_ADDRESS) & 0xFFE0); PECMD = 0x5A; NOP(); NOP();}
#define BISP(ROM_ADDRESS, RAM_ADDRESS) {PERAM = (RAM_ADDRESS); PEROM = ((ROM_ADDRESS) & 0xFFFF); PECMD = 0x1E; NOP(); NOP();}

#endif  // __SDCC

#endif  // __SN8F5701_H__
