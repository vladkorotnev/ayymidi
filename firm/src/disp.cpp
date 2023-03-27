#include <Arduino.h>
#include <AsyncCrystal_I2C.h>
#include <util.h>
#include <status.h>
#include <disp.h>
#include <ay.h>

AsyncCrystal_I2C lcd(0x27, 16, 2); // Default address of most PCF8574 modules, change according

const char CHAR_ICON_IN_OFF = 1;
PROGMEM const char IN_ICON_OFF[] = {
    0b00011,
    0b00101,
    0b01001,
    0b10001,
    0b10001,
    0b01001,
    0b00101,
    0b00011
};

const char CHAR_ICON_IN_ON = 2;
PROGMEM const char IN_ICON_ON[] = {
    0b00011,
    0b00111,
    0b01111,
    0b11111,
    0b11111,
    0b01111,
    0b00111,
    0b00011
};

const char CHAR_ICON_CHSWAP = 3;
PROGMEM const char MODE_ICON_ACB[] = {
    0b00000,
    0b00100,
    0b00010,
    0b11111,
    0b00000,
    0b11111,
    0b01000,
    0b00100,
};

const char CHAR_ICON_NO_CHSWAP = ' ';

static bool is_chswap = false;
static uint8_t midi_in_sts = 0;

void disp_intro() {
    lcd.setCursor(0, 0);
    lcd.print(F("Genjitsu Labs    "));
    lcd.flush();
    lcd.setCursor(0, 1);
    lcd.print(F("    AYYMIDI v1.0"));
    lcd.flush();
}

void disp_midi_light() {
    midi_in_sts = 4;
}

void disp_ch_swap(bool is_swap) {
    is_chswap = is_swap;
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

void disp_draw_home_top() {
    static char buf[17] = { 0 };
    uint8_t lv_a = status_regi_get_blocking(AY_REGI_LVL_A) & 0x0F;
    uint8_t lv_b = status_regi_get_blocking(AY_REGI_LVL_B) & 0x0F;
    uint8_t lv_c = status_regi_get_blocking(AY_REGI_LVL_C) & 0x0F;
    uint16_t env = (status_regi_get_blocking(AY_REGI_ENV_ROUGH) << 8) | status_regi_get_blocking(AY_REGI_ENV_FINE);

    sprintf(buf, " %.1X %.1X %.1X  %.4X  %c%c", lv_a, lv_b, lv_c, env, is_chswap ? CHAR_ICON_CHSWAP : CHAR_ICON_NO_CHSWAP, (midi_in_sts > 0) ? CHAR_ICON_IN_ON : CHAR_ICON_IN_OFF);
    lcd.setCursor(0, 0);
    lcd.print(buf);
}

void disp_draw_home_bottom() {
    static char buf[17] = { 0 };
    uint16_t tone_a = ((status_regi_get_blocking(1) & 0xF) << 8) | status_regi_get_blocking(0);
    uint16_t tone_b = ((status_regi_get_blocking(3) & 0xF) << 8) | status_regi_get_blocking(2);
    uint16_t tone_c = ((status_regi_get_blocking(5) & 0xF) << 8) | status_regi_get_blocking(4);
    uint8_t noise = status_regi_get_blocking(AY_REGI_NOISE) & 0x1F;

    sprintf(buf, " %.3X %.3X %.3X %.2X ", tone_a, tone_b, tone_c, noise);
    lcd.setCursor(0, 1);
    lcd.print(buf);

    if(midi_in_sts > 0) midi_in_sts--;
}

void disp_rst_home() {
    // redraw the home screen in full
    disp_draw_home_top();
    lcd.flush();
    disp_draw_home_bottom();
    lcd.flush();
}

void disp_tick() {
   if(!lcd.busy())  {
        disp_draw_home_top();
        disp_draw_home_bottom();
   }

   lcd.loop();
}

void disp_begin() {
    inf_log(F("Setup LCD"));
    lcd.init();

    lcd.createChar(CHAR_ICON_IN_OFF, IN_ICON_OFF);
    lcd.flush();
    lcd.createChar(CHAR_ICON_IN_ON, IN_ICON_ON);
    lcd.flush();
    lcd.createChar(CHAR_ICON_CHSWAP, MODE_ICON_ACB);
    lcd.flush();

    lcd.backlight();
    lcd.noCursor();
    lcd.flush();

    disp_intro();
    delay(2000);
}