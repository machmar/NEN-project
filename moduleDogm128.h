//*****************************************************************************
// header:  moduleDogm128.h
// author:  LL
// created: 03/2024
// project: header for graphic display DOGM128
//*****************************************************************************

#ifndef MODULEDOGM128_H
#define	MODULEDOGM128_H

//-----------------------------------------------------------------------------
// pinout of display

// DISPLAY
#define RST_DISP_H  (LATC |= (1<<1))
#define RST_DISP_L  (LATC &= ~(1<<1))
#define A0_DISP_H   (LATD |= (1<<0))
#define A0_DISP_L   (LATD &= ~(1<<0))
#define PWM_DISP_H  (LATC |= (1<<2))
#define PWM_DISP_L  (LATC &= ~(1<<2))


void initDisplay(void);
void clearAllDisplay(void);
void clearPageDisplay(unsigned char page);
void fillChessBoardDisplay(void);
void writeTextToDisplay(unsigned char page, unsigned char column, char* txt);

#endif	/* MODULEDOGM128_H */

