//*****************************************************************************
// source:  moduleDogm128.c 128x64 pixels
// project: module for graphic display DOGM128
// author:  LL
// created: 03/2024
//*****************************************************************************
#include <xc.h>
#include "moduleDogm128.h"
#include "spi.h"

// private functions
void sendCommandToDisplay(unsigned char comm);

//*****************************************************************************
// initialization of display DOGM128
// 128x64 pixels, 128 columns, 8 pages (each has 8 rows)
void initDisplay(void)
{
    initSpi();
    PWM_DISP_H;     // Light full
    RST_DISP_H;     // RST inactive
    
    //sendCommandToDisplay(0xAF);
    sendCommandToDisplay(0x40); // Display start at line 0
    sendCommandToDisplay(0xA1); // ADC reverse
    sendCommandToDisplay(0xC0); // Normal COM0 - COM63
    sendCommandToDisplay(0xA6); // Display normal
    sendCommandToDisplay(0xA2); // Set Bias 1/9
    sendCommandToDisplay(0x2F); // Booster, regulator and follower on
    sendCommandToDisplay(0xF8); // Set Internal Booster to 4x
    sendCommandToDisplay(0x00);
    sendCommandToDisplay(0x27); // Voltage regulator set
    sendCommandToDisplay(0x81); // Contrast set
    sendCommandToDisplay(0x16);
    sendCommandToDisplay(0xAC); // No indicator
    sendCommandToDisplay(0x00);
    sendCommandToDisplay(0xAF); // Display on
}

//*****************************************************************************
// private functions
//*****************************************************************************
// send command (A0 = L)
void sendCommandToDisplay(unsigned char comm)
{
    A0_DISP_L;          // command
    CS_DISP_L;          // CS active in low
    __nop();            // delay 1 step
    sendByteSpi(comm);  // send command
    __nop();            // delay 1 step
    CS_DISP_H;          // CS inactive in H
}
