#include <xc.h>
#include <stdint.h>
#include "globals.h"
#include "dogm128_fast.h"
#include "utils.h"
#include "raycasting.h"
#include "fx8.h"
#include "moduleDogm128.h"
#include "assets.h"

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
    TRISB = 0x3D;    // buttons
    TRISC = 0x00;    // SPI + control lines
    TRISD = 0x00;
    TRISE = 0x00;
}

buttons_t read_buttons()
{
    uint8_t y = ~PORTB;
    buttons_t buttons;
    buttons.back = y & (1 << 5);
    buttons.front = y & (1 << 4);
    buttons.use = y & (1 << 2);
    buttons.left = y & (1 << 3);
    buttons.right = y & (1 << 0);
    return buttons;
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
    initDisplay();
    pwm_ccp1_init();
    dogm128_init();
    Backlight(1023);
    set_LEDs(0x00);

    buttons_t buttons = {0};

    player_t camera;
    camera.posX = FX(22);
    camera.posY = FX(12);
    camera.dirX = FX(-1);
    camera.dirY = FX(0);
    camera.planeX = FX(0);
    camera.planeY = 0x00a9; // 0.66
    
    char buf[64];

    entity_t entities[10] = {0};
    entities[0].posX = 17;
    entities[0].posY = 12;
    entities[0].health = 100;
    entities[0].sprite = sprite;

    entities[1].posX = 17;
    entities[1].posY = 8.5;
    entities[1].health = 100;
    entities[1].sprite = sprite;

    while (1)
    {
        static millis_t PMill = 0;
        static millis_t frame_length = 0;
        PMill = millis;
        dogm128_clear();
        buttons = read_buttons();

        MoveCamera(&camera, buttons);
        RenderFrame(camera, frame_buffer[0]);
        DrawEntities(camera, entities, MAX_ENTITIES, dogm_fb);
        DrawBuffer(frame_buffer[0]);
        
        frame_length = millis - PMill;
        
        utoa(FX_TO_INT(camera.posX), buf);
        dogm128_text(0, 0, buf);
        utoa(FX_TO_INT(camera.posY), buf);
        dogm128_text(20, 0, buf);
        utoa(1000 / frame_length, buf);
        dogm128_text(40, 0, buf);
        
        uint8_t y = ~PORTB;
    uint8_t z = ((y & (1 << 5)) >> 5) |
                ((y & (1 << 4)) >> 3) |
                ((y & (1 << 2)) << 0) |
                ((y & (1 << 3)) << 0) |
                ((y & (1 << 0)) << 5);
    // 5 4 2 3 0
        
        utoa(z, buf);
        dogm128_text(80, 0, buf);
        
        dogm128_refresh();
        static char led = 0xFF;
        led = ~led;
        set_LEDs(led);
    }
}

void __interrupt() isr(void)
{
    if (PIR1bits.TMR2IF)
    {
        PIR1bits.TMR2IF = 0;
        static uint8_t local_tick = 0;
        local_tick++;
        AdvanceDither();
        if (local_tick & 0b100)
        {
            local_tick = 0;
            // 1 kHz task here
            millis++;
        }
    }
}