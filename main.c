#include <builtins.h>
#include <xc.h>
#include <stdint.h>
#include "globals.h"
#include "dogm128_fast.h"
#include "utils.h"
#include "raycasting.h"
#include "fx8.h"
#include "moduleDogm128.h"
#include "assets.h"
#include "HUDStuff.h"

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
player_t camera;
buttons_t buttons = {0};
static entity_t entities[2];
map_t *CurrentMap = &TestMap;

void main(void)
{
    init_ports();
    initDisplay();
    pwm_ccp1_init();
    dogm128_init();
    Backlight(1023);
    set_LEDs(0x00);


    camera.posX = FX(CurrentMap->DefaultSpwanPoint[0]);
    camera.posY = FX(CurrentMap->DefaultSpwanPoint[1]);
    camera.angle = FX_ANGLE_HALF; // facing -X
    camera.dirX = fx_cos(camera.angle);
    camera.dirY = fx_sin(camera.angle);
    camera.planeX = fx_mul(camera.dirY, (fx_t)0x00a9);
    camera.planeY = fx_neg(fx_mul(camera.dirX, (fx_t)0x00a9));
    // Filling zBuffer with max distance to test sprite rendering
    for(int i = 0; i < 48 - 1; i++){
        camera.zBuffer[i] = FX(64);
    }
    
    char buf[10];

    entities[0].posX = FX(12);
    entities[0].posY = FX(12);
    entities[0].health = 100;
    entities[0].sprite = &enemySprite;

    entities[1].posX = FX(2);
    entities[1].posY = FX(2);
    entities[1].health = 100;
    entities[1].sprite = &enemySprite;

    while (1)
    {
        static millis_t PMill = 0;
        static millis_t frame_length = 0;
        PMill = millis;
        dogm128_clear();
        buttons = read_buttons();

        MoveCamera(&camera, CurrentMap, buttons);
        RenderFrame(&camera, CurrentMap, frame_buffer[0]);
        DrawBuffer(frame_buffer[0]);
        DrawEntities(&camera, entities, 2, dogm_fb);
        HUD_DrawBanner((millis / 3000) % 5);        
        
        
        dogm128_vline(96, 0, 64, DISP_COL_BLACK);
        dogm128_hline(96, 32, 32, DISP_COL_BLACK);
        HUD_DrawMap(96, 0, CurrentMap, &camera);
        
    
        
        frame_length = millis - PMill;
        utoa(1000 / frame_length, buf);
        dogm128_text(0, 0, buf);
        utoa(FX_I(camera.posX), buf);
        dogm128_text(0, 6, buf);
        utoa(FX_I(camera.posY), buf);
        dogm128_text( 20, 6, buf);
        
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