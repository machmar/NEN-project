#include <builtins.h>
#include <xc.h>
#include <stdint.h>
#include <string.h>
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
#pragma config WDTPS = 1        // shortest timeout (~4 ms typical)
#pragma config LVP = OFF
#pragma config PBADEN = OFF

static millis_t PMill = 0;
player_t camera;
buttons_t buttons = {0};
static entity_t entities[MAX_ENTITIES];
map_t *CurrentMap = &Level0Map;
dialogue_t *CurrentDialogue = NULL;
millis_t usePMill = 0;
uint16_t backlightVal = 1023;
_Bool showFPS = 0;
uint8_t menuOpen = 0;
uint8_t currentLevelNum = 0;

void init_ports(void) {
    ADCON1 = 0x0F; // all pins digital

    TRISA = 0x00; // LEDs
    TRISB = 0x3D; // buttons
    TRISC = 0x00; // SPI + control lines
    TRISD = 0x00;
    TRISE = 0x00;
}

buttons_t read_buttons() {
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

void set_LED(uint8_t button, _Bool state) {
    switch (button) {
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

void set_LEDs(uint8_t state) {
    state &= 0x1F;
    LATA = state;
}

static void pwm_ccp1_init(void) {
    ADCON1 = 0x0F; // all digital
    CMCON = 0x07; // comparators off, if applicable
    TRISCbits.TRISC2 = 0; // RC2 = CCP1 output

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
    PIE1bits.TMR2IE = 1; // enable Timer2 interrupt

    T2CONbits.TMR2ON = 1; // start Timer2

    INTCONbits.PEIE = 1;
    INTCONbits.GIE = 1;
}

void Backlight(uint16_t duty10) {
    if (duty10 > 1023) duty10 = 1023;

    CCPR1L = duty10 >> 2; // upper 8 bits
    CCP1CONbits.DC1B = duty10 & 0x03; // lower 2 bits
}

void ChangeLevel(uint8_t level) {
    switch (level) {
        case 0:
            CurrentMap = &Level0Map;
            break;
        case 1:
            CurrentMap = &Level1Map;
            break;
        case 2:
            CurrentMap = &Level2Map;
            for (uint8_t i = 0; i < MAX_ENTITIES; i++) {
                uint8_t type = rand16();
                while (type > 2) type = rand16();
                memcpy(&entities[i], enemieTemplates[type], sizeof (entity_t));
                RespawnEntity(CurrentMap, &entities[i]);
            }
            break;
        case 3:
            CurrentMap = &Level3Map;
            for (uint8_t i = 0; i < MAX_ENTITIES; i++) {
                memcpy(&entities[i], &soilderTemplate, sizeof (entity_t));
                RespawnEntity(CurrentMap, &entities[i]);
            }
            break;
    }

    camera.posX = FX(CurrentMap->DefaultSpwanPoint[0]);
    camera.posY = FX(CurrentMap->DefaultSpwanPoint[1]);

    currentLevelNum = level;
}

void DrawMenu(buttons_t state, bool disallow_resume) {
    if (menuOpen == 0) {
        if (state.all >= 0b11111) menuOpen = 1;
        else return;
    }
    if (menuOpen == 1) {
        if (state.all == 0)
            menuOpen = 2;
    }

    dogm128_fill_rect(0, 64 - 7, 128, 7, DISP_COL_WHITE);
    dogm128_hline(0, 64 - 7, 128, DISP_COL_BLACK);
    dogm128_text(1, 64 - 5, "resume");
    dogm128_text(127 - 20, 64 - 5, "reset");
    dogm128_text(30, 64 - 5, "fps");
    if (showFPS) dogm128_text(64 - 10, 64 - 5, "level");

    if (menuOpen == 2) {
        if (state.back && !disallow_resume) menuOpen = 0;
        if (state.right) {
            WDTCONbits.SWDTEN = 1;
            while (1);
        }
        if (state.front) {
            showFPS = !showFPS;
            menuOpen = 0;
        }
        if (state.use && showFPS) {
            currentLevelNum++;
            if (currentLevelNum > 3)
                currentLevelNum = 0;
            ChangeLevel(currentLevelNum);
            menuOpen = 0;
        }
    }
}

static void OnMapEvent(uint8_t param1, uint8_t param2) {
    // param1 = eventNum (tile & 0x0F), param2 = stepOn
    if (param1 == 0 && param2 == 1) { // stepped on teleportation tile in Level0
        //backlightVal = 300;
        ChangeLevel(1);
    }

    if (param1 == 1 && param2 == 1) { // stepped on teleportation tile in Level1
        //backlightVal = 300;
        ChangeLevel(2);
    }

    if (param1 == 2 && param2 == 1) { // stepped on teleportation tile in Level1
        //backlightVal = 300;
        ChangeLevel(3);
    }
}

void main(void) {
    WDTCONbits.SWDTEN = 0; // disable WDT
    init_ports();
    initDisplay();
    pwm_ccp1_init();
    dogm128_init();
    Backlight(backlightVal);
    set_LEDs(0x00);
    MapEventCallback = OnMapEvent;

    camera.posX = FX(CurrentMap->DefaultSpwanPoint[0]);
    camera.posY = FX(CurrentMap->DefaultSpwanPoint[1]);
    camera.angle = FX_ANGLE_HALF; // facing -X
    camera.dirX = fx_cos(camera.angle);
    camera.dirY = fx_sin(camera.angle);
    camera.planeX = fx_mul(camera.dirY, (fx_t) 0x00a9);
    camera.planeY = fx_neg(fx_mul(camera.dirX, (fx_t) 0x00a9));
    // Filling zBuffer with max distance to test sprite rendering
    for (int i = 0; i < 48 - 1; i++) {
        camera.zBuffer[i] = FX(64);
    }

    camera.health = 5;
    camera.kills = 0;

    char buf[10];

    while (1) {
        static millis_t PMill = 0;
        static millis_t frame_length = 0;
        static uint8_t prevFrames = 0;
        PMill = millis;
        buttons = read_buttons();

        static bool prevMenu = 0;
        if (menuOpen == 0) {
            dogm128_clear();
            MoveCamera(&camera, CurrentMap, buttons, &CurrentDialogue, prevMenu);
            RenderFrame(&camera, CurrentMap);
            if (CurrentMap != &Level0Map && CurrentMap != &Level1Map) {
                DrawEntities(&camera, entities, MAX_ENTITIES, dogm_fb, buttons, CurrentMap);
                EnemyAi(&camera, entities, MAX_ENTITIES, CurrentMap, prevMenu);
            }

            HUD_DrawBorders();
            HUD_DrawItem(camera.currentItem);
            HUD_DrawMap(CurrentMap, &camera);
            HUD_DrawCompass(camera.angle, camera.dirX, camera.dirY);
            if (CurrentMap != &Level0Map) {
                HUD_DrawBanner(CurrentMap->Banner);
                HUD_DrawStats(camera.health, camera.kills);
                HUD_DrawItemPOV(camera.currentItem, usePMill + 200 > millis);
            }

            static _Bool prevUse = 0;
            _Bool usePressed = buttons.use && !prevUse;
            prevUse = buttons.use;

            _Bool dialogueActive = (CurrentDialogue != NULL);
            HUD_DrawDialogue(&CurrentDialogue, usePressed && dialogueActive);
            if (usePressed) {
                // use button available for future interactions
                usePMill = millis;
            }

            if (camera.health == 0) {
                dogm128_fill_rect((96 / 2) - 25, (64 / 2) - 10, 50, 20, DISP_COL_WHITE);
                dogm128_text((96 / 2) - 6, (64 / 2) - 2, "RIP");
                menuOpen = 1;
            }
            prevMenu = 0;
        } else prevMenu = 1;

        DrawMenu(buttons, camera.health == 0);


        // display FPS in the corner for testing
        if (showFPS) {
            frame_length = millis - PMill;
            utoa_mine(1000 / frame_length, buf, 0);
            dogm128_text(0, 0, buf);
        }

        dogm128_refresh();
        set_LEDs(HUD_GetLEDHP(camera.health));
        Backlight(backlightVal);
    }
}

void __interrupt() isr(void) {
    if (PIR1bits.TMR2IF) {
        PIR1bits.TMR2IF = 0;
        static uint8_t local_tick = 0;
        local_tick++;
        if (local_tick & 0b100) {
            local_tick = 0;
            // 1 kHz task here
            millis++;
        }
    }
}