#include <xc.h>
#include <stdint.h>
#include "globals.h"
#include "dogm128_fast.h"
#include "utils.h"
#include "raycasting.h"

// CONFIG (example)
#pragma config FOSC = HSPLL_HS
#pragma config PLLDIV = 4
#pragma config CPUDIV = OSC1_PLL2
#pragma config USBDIV = 2
#pragma config WDT = OFF
#pragma config LVP = OFF
#pragma config PBADEN = OFF

typedef uint32_t millis_t;

volatile millis_t millis = 0;

void init_ports(void)
{
    ADCON1 = 0x0F;   // all pins digital

    TRISA = 0x00;    // LEDs
    TRISB = 0xFF;    // buttons
    TRISC = 0x00;    // SPI + control lines
}

uint8_t read_buttons()
{
    uint8_t y = ~PORTB;
    uint8_t z = ((y & (1 << 5)) >> 5) |
                ((y & (1 << 4)) >> 4) |
                ((y & (1 << 2)) << 0) |
                ((y & (1 << 3)) << 0) |
                ((y & (1 << 0)) << 5);
    return z;
    // 5 4 2 3 0
}


void set_LED(uint8_t button, _Bool state)
{
    switch (button)
    {
        case 1:
            LATAbits.LATA0 = state;
            break;
                    
        case 2:
            LATAbits.LATA1 = state;
            break;
                    
        case 3:
            LATAbits.LATA2 = state;
            break;
                    
        case 4:
            LATAbits.LATA3 = state;
            break;
                    
        case 5:
            LATAbits.LATA4 = state;
            break;
            
        default:
            break;
    }
}

void set_LEDs(uint8_t state)
{
    state &= 0x1F;
    LATA = state;
}

static void pwm_ccp1_init(void)
{
    ADCON1 = 0x0F;              // all digital
    CMCON  = 0x07;              // comparators off, if applicable
    TRISCbits.TRISC2 = 0;       // RC2 = CCP1 output

    // PWM period ~ 3.989 kHz
    PR2 = 187;

    // CCP1 PWM mode
    CCP1CONbits.CCP1M = 0b1100;

    // initial duty = 0
    CCPR1L = 0;
    CCP1CONbits.DC1B = 0;

    // Timer2 prescaler = 1:16
    T2CONbits.T2CKPS0 = 1;
    T2CONbits.T2CKPS1 = 1;

    TMR2 = 0;
    PIR1bits.TMR2IF = 0;
    PIE1bits.TMR2IE = 1;        // enable Timer2 interrupt

    T2CONbits.TMR2ON = 1;       // start Timer2

    INTCONbits.PEIE = 1;
    INTCONbits.GIE  = 1;
}

void Backlight(uint16_t duty10)
{
    if (duty10 > 1023) duty10 = 1023;

    CCPR1L = duty10 >> 2;                 // upper 8 bits
    CCP1CONbits.DC1B = duty10 & 0x03;     // lower 2 bits
}

static millis_t PMill = 0;

void main(void)
{
    init_ports();
    pwm_ccp1_init();
    dogm128_init();
    Backlight(1023);
    set_LEDs(0x00);

    player_t camera;
    camera.posX = 22;
    camera.posY = 12;
    camera.dirX = -1;
    camera.dirY = 0;
    camera.planeX = 0;
    camera.planeY = 0.66;
    
    while (1)
    {
        dogm128_clear();
        
        DrawFrame(camera);
        
        dogm128_refresh();
        
        if (read_buttons() & (1 << 0))
        {
            camera.posX++;
            if (camera.posX > 22) camera.posX = 0;
        }
        if (read_buttons() & (1 << 1))
        {
            camera.posX--;
            if (camera.posX < 0) camera.posX = 22;
        }
        if (read_buttons() & (1 << 2))
        {
            camera.posY++;
            if (camera.posY > 22) camera.posY = 0;
        }
        if (read_buttons() & (1 << 3))
        {
            camera.posY--;
            if (camera.posY < 0) camera.posY = 22;
        }
    }
}

void __interrupt() isr(void)
{
    if (PIR1bits.TMR2IF)
    {
        PIR1bits.TMR2IF = 0;
        static uint8_t local_tick = 0;
        local_tick++;
        if (local_tick & 0b100)
        {
            local_tick = 0;
            // 1 kHz task here
            millis++;
        }
    }
}