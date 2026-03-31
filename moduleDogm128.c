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
void sendDataToDisplay(unsigned char dat);
void setColumnAddress(unsigned char column);
void setPageAddress(unsigned char page);

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
    
    clearAllDisplay();
}

//*****************************************************************************
// clear all 8 pages
void clearAllDisplay(void)
{
    unsigned char pg; // page(8)
    for(pg=0; pg<8; pg++)
    {
        clearPageDisplay(pg);
    }
}

//*****************************************************************************
// clear one of 8 pages (0 - 7)
void clearPageDisplay(unsigned char page)
{
    int cl; // collumn (128)
    setColumnAddress(0);
    setPageAddress(page);
    for(cl=0; cl<128;cl++)
    {
        sendDataToDisplay(0x00);
    }
}

//*****************************************************************************
// fill display as a  chess board
void fillChessBoardDisplay(void)
{
    unsigned char cl, pg; // collumn (128), page(8))
    for(pg=0; pg<8; pg++)
    {
        setColumnAddress(0);
        setPageAddress(pg);
        for(cl=0; cl<128;cl++)
        {
            sendDataToDisplay(0xAA);
            sendDataToDisplay(0x55);
        }
    }
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

//*****************************************************************************
// send data (A0 = H)
void sendDataToDisplay(unsigned char dat)
{
    A0_DISP_H;          // data
    CS_DISP_L;          // CS active in low
    __nop();            // delay 1 step
    sendByteSpi(dat);   // send data
    __nop();            // delay 1 step
    CS_DISP_H;          // CS inactive in H
}

//*****************************************************************************
// set addres of collumn on a page (0 - 127)
void setColumnAddress(unsigned char column)
{
    unsigned char col;
    if(column > 127)
    {
        column = 127;
    }
    col = (column & 0x0F) | 0x00;
    sendCommandToDisplay(col);
    col = (column >> 4) | 0x10;
    sendCommandToDisplay(col);
}

//*****************************************************************************
// set page (0-7)
void setPageAddress(unsigned char page)
{
    if(page > 7)
    {
        page = 7;
    }
    sendCommandToDisplay(0xB0 | page);
}