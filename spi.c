//*****************************************************************************
// source:  spi.h
// mcu:     PIC18LF4550
// author:  LL
// created: 03/2024
// project: graphicKit ver1 graphic display DOGM128 controll - example
//*****************************************************************************

#include <xc.h>
#include "spi.h"


void initSpi(void)
{
    SSPSTAT = 0x00;    // input data sampled at middle, transmit data from L->H
    SSPCON1 = 0x10; // master fosc/4 (12MHz), iddle for CLK is high
    SSPCON1bits.SSPEN = 1;  // SPI enable
    PIR1bits.SSPIF = 0;     // clear flag of interrupt
    PIE1bits.SSPIE = 0;     // interrupt disable
}

unsigned char sendByteSpi(unsigned char dat)
{
    SSPBUF = dat;               // transmitt data
    while(!(PIR1bits.SSPIF));   // wait until data is sent
    PIR1bits.SSPIF = 0;         // clear flag of data sending
    return SSPBUF;              // retur data from SPI (not use)
}

