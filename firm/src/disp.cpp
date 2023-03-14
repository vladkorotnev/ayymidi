#include <Arduino.h>
#include <LCD_I2C.h>
#include <util.h>
#include <status.h>
#include <disp.h>
#include <ay.h>

LCD_I2C lcd(0x27, 16, 2); // Default address of most PCF8574 modules, change according

#define CHAR_ICON_IN_OFF 0
PROGMEM const uint8_t IN_ICON_OFF[] = {
    0b00011,
    0b00101,
    0b01001,
    0b10001,
    0b10001,
    0b01001,
    0b00101,
    0b00011
};

#define CHAR_ICON_IN_ON 1
PROGMEM const uint8_t IN_ICON_ON[] = {
    0b00011,
    0b00111,
    0b01111,
    0b11111,
    0b11111,
    0b01111,
    0b00111,
    0b00011
};

#define CHAR_ICON_ACB 2
PROGMEM const uint8_t MODE_ICON_ACB[] = {
    0b01001,
    0b10110,
    0b11110,
    0b10101,
    0b01010,
    0b01100,
    0b01010,
    0b01100
};

#define CHAR_ICON_ABC 0x20 // empty

static bool is_home_screen = false;
static TimeOut_t last_midi_in;;
static TickType_t remain_lit = 0;

void _disp_task(void*);

void disp_begin() {
    lcd.begin();

    lcd.createChar(CHAR_ICON_IN_OFF, IN_ICON_OFF);
    lcd.createChar(CHAR_ICON_IN_ON, IN_ICON_ON);

    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.noCursor();
    lcd.clear();

    xTaskCreate(
    _disp_task
    ,  "DISP"
    ,  DISP_STACKSZ
    ,  NULL
    ,  DISP_PRIORITY
    ,  NULL );
}

void disp_intro() {
    is_home_screen = false;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("Genjitsu Labs    "));
    lcd.setCursor(0, 1);
    lcd.print(F("     AYYMIDI v1.0"));
}

void disp_midi_light() {
    remain_lit = pdMS_TO_TICKS(500);
    vTaskSetTimeOutState(&last_midi_in);
}


// LCD home screen example
// +----------------+
// | L-L-L  Eeee  Z<|
// +----------------+
// | Taa Tbb Tcc Nf |
// +----------------+
//
// where:
// - Taa, Tbb, Tcc: TONE value of ch. A/B/C
// - Nf: FREQ value of noise generator
// - Eeee: Envelope FREQ value
// - L: VOL value of ABC
// - <: MIDI IN data sign
// - Z: ACB icon if channel remap is active

void disp_show_volume() {
    if(!is_home_screen) return;

    static char buf[6] = { 0 };
    uint8_t lv_a = status_regi_get_blocking(AY_REGI_LVL_A) & 0x0F;
    uint8_t lv_b = status_regi_get_blocking(AY_REGI_LVL_B) & 0x0F;
    uint8_t lv_c = status_regi_get_blocking(AY_REGI_LVL_C) & 0x0F;

    sprintf(buf, "%01x %01x %01x", lv_a, lv_b, lv_c);

    lcd.setCursor(1, 0);
    lcd.print(buf);
}

void disp_show_env() {
    if(!is_home_screen) return;
    lcd.setCursor(8,0);
    static char buf[5] = { 0 };
    uint16_t env = (status_regi_get_blocking(AY_REGI_ENV_ROUGH) << 8) | status_regi_get_blocking(AY_REGI_ENV_FINE);
    sprintf(buf, "%04x", env);
    lcd.print(buf);
}

void disp_show_sta() {
    if(!is_home_screen) return;

    lcd.setCursor(14, 0);
    lcd.write(CHAR_ICON_ABC);

    if(xTaskCheckForTimeOut( &last_midi_in, &remain_lit ) == pdFALSE) {
        lcd.write(CHAR_ICON_IN_ON);
    } else {
        lcd.write(CHAR_ICON_IN_OFF);
    }
}

void disp_show_tone(uint8_t idx) {
    if(!is_home_screen || idx > 2) return;

    uint16_t tone = ((status_regi_get_blocking(1 + idx) & 0xF) << 8) | status_regi_get_blocking(idx);
    static char buf[4] = {0};
    sprintf(buf, "%03x", tone);

    switch (idx) {
        case 0:
            lcd.setCursor(1,1);
            break;

        case 1:
            lcd.setCursor(5,1);
            break;

        case 2:
            lcd.setCursor(9,1);
            break;

        default:
            return;
    }
    lcd.print(tone);
}

void disp_show_noise() {
    if(!is_home_screen) return;

    uint8_t noise = status_regi_get_blocking(AY_REGI_NOISE) & 0x1F;
    static char buf[3] = { 0 };
    sprintf(buf, "%02x", noise);
    lcd.setCursor(13, 1);
    lcd.print(buf);
}

void disp_draw_home() {
    disp_show_volume();
    disp_show_env();
    disp_show_sta();
    disp_show_tone(0);
    disp_show_tone(1);
    disp_show_tone(2);
}

void disp_rst_home() {
    // redraw the home screen in full
    if(is_home_screen) return;

    is_home_screen = true;

    lcd.clear();
    disp_draw_home();
}

void _disp_task(void* pvParameters) {
    __UNUSED_ARG(pvParameters);

    disp_intro();
    vTaskDelay(pdMS_TO_TICKS(2000));

    status_wait_change_regi(); // wait until any midi message or something does a register write
    disp_rst_home(); // show whole home screen

    for(;;) {
        // infinitely do partial updates
        regi_valu_tuple_t tuple = status_wait_change_regi();
#ifdef FEATURE_PARTIAL_UPDATES
        switch(REGI_OF_TUPLE(tuple)) {
            case AY_REGI_TONE_A_COARSE:
            case AY_REGI_TONE_A_FINE:
                disp_show_tone(0);
                break;

            case AY_REGI_TONE_B_COARSE:
            case AY_REGI_TONE_B_FINE:
                disp_show_tone(1);
                break;

            case AY_REGI_TONE_C_COARSE:
            case AY_REGI_TONE_C_FINE:
                disp_show_tone(2);
                break;

            case AY_REGI_NOISE:
                disp_show_noise();
                break;

            case AY_REGI_LVL_A:
            case AY_REGI_LVL_B:
            case AY_REGI_LVL_C:
                disp_show_volume();
                break;

            case AY_REGI_ENV_FINE:
            case AY_REGI_ENV_ROUGH:
                disp_show_env();
            
            default:
                break;
        }
#else
        disp_draw_home();
#endif
        vTaskDelay(pdMS_TO_TICKS(16)); // roughly 60 fps should be enough
    }
}