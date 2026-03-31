//*****************************************************************************
// header:  spi.h
// mcu:     PIC18LF4550
// author:  LL
// created: 03/2024
// project: graphicKit ver1 graphic display DOGM128 controll - example
//*****************************************************************************

// CS   RC0
// CLK  RB1
// MOSI RC7  
// MISO RB0 (do not used)
// CS   RCO must be separate controll 
// RST  RC  reset of display


#ifndef SPI_H
#define	SPI_H

#define CS_DISP_H   (LATC |= (1<<0))
#define CS_DISP_L   (LATC &= ~(1<<0))

void initSpi(void);
unsigned char sendByteSpi(unsigned char dat);


#endif	/* SPI_H */

