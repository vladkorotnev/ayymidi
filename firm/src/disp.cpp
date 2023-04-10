#include <Arduino.h>
#include <AsyncCrystal_I2C.h>
#include <util.h>
#include <status.h>
#include <disp.h>
#include <ay.h>

AsyncCrystal_I2C lcd(0x27, 16, 2); // Default address of most PCF8574 modules, change according

const char CHAR_ICON_CHSWAP = 0;
PROGMEM const char MODE_ICON_ACB[] = {
    0b00100,
    0b00010,
    0b11111,
    0b00000,
    0b11111,
    0b01000,
    0b00100,
    0b00000,
};
const char CHAR_ICON_NO_CHSWAP = ' ';

const char CHAR_ICON_BAR_START = 2;
PROGMEM const char ICON_BARS[] = {
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b01110,
    0b01110,

    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b01110,
    0b01110,
    0b01110,
    0b01110,

    0b00000,
    0b00000,
    0b01110,
    0b01110,
    0b01110,
    0b01110,
    0b01110,
    0b01110,

    0b01110,
    0b01110,
    0b01110,
    0b01110,
    0b01110,
    0b01110,
    0b01110,
    0b01110,
};

const char CHAR_ICON_BAR_LUT[] = {
    ' ',  // 0
    CHAR_ICON_BAR_START, // 1
    CHAR_ICON_BAR_START, // 2
    CHAR_ICON_BAR_START+1, // 3
    CHAR_ICON_BAR_START+1, // 4
    CHAR_ICON_BAR_START+2, // 5
    CHAR_ICON_BAR_START+2, // 6
    CHAR_ICON_BAR_START+3, // 7
    CHAR_ICON_BAR_START+3 // 8
};

static bool is_chswap = false;
static uint8_t midi_in_sts = 0;
static uint16_t msg_dur = 0;
unsigned long last_millis = 0;


const char CHAR_SPINNER = 1;
const size_t SPINNER_PHASES_MAX = 7;
static int spinner_phase = 0;
static int old_spinner_phase = 0;
PROGMEM const char ICON_SPINNER_FRAMES[] = { 
    0b00000,
    0b10100,
    0b00000,
    0b10001,
    0b00000,
    0b10101,
    0b00000,
    0b00000,

    0b00000,
    0b10101,
    0b00000,
    0b10000,
    0b00000,
    0b10101,
    0b00000,
    0b00000,

    0b00000,
    0b10101,
    0b00000,
    0b10001,
    0b00000,
    0b10100,
    0b00000,
    0b00000,

    0b00000,
    0b10101,
    0b00000,
    0b10001,
    0b00000,
    0b10001,
    0b00000,
    0b00000,

    0b00000,
    0b10101,
    0b00000,
    0b10001,
    0b00000,
    0b00101,
    0b00000,
    0b00000,

    0b00000,
    0b10101,
    0b00000,
    0b00001,
    0b00000,
    0b10101,
    0b00000,
    0b00000,

    0b00000,
    0b00101,
    0b00000,
    0b10001,
    0b00000,
    0b10101,
    0b00000,
    0b00000,

    0b00000,
    0b10001,
    0b00000,
    0b10001,
    0b00000,
    0b10101,
    0b00000,
    0b00000,
 };

void disp_spinner_update(bool sync) {
    if(spinner_phase >= SPINNER_PHASES_MAX) spinner_phase = 0;
    lcd.createChar(CHAR_SPINNER, &ICON_SPINNER_FRAMES[spinner_phase * 8]);
    if(sync) lcd.flush();
    old_spinner_phase = spinner_phase;
}

static uint8_t a_bar = 0;
static uint8_t b_bar = 0;
static uint8_t c_bar = 0;

void disp_sus_upd_ms(uint16_t time) {
    msg_dur = time / DISP_UPD_INTERVAL;
}

void disp_show_msg(uint16_t time, const char * top_line, const char * bottom_line) {
    lcd.setCursor(0, 0);
    lcd.print(top_line);
    lcd.setCursor(0, 1);
    lcd.print(bottom_line);
    disp_sus_upd_ms(time);
}

void disp_stop_msg() {
    disp_sus_upd_ms(0);
}

void disp_intro() {
    lcd.setCursor(0, 0);
    lcd.print(F("Genjitsu Labs    "));
    lcd.flush();
    lcd.setCursor(0, 1);
    lcd.print(F("  AYYMIDI   v1.1"));
    lcd.flush();
    disp_sus_upd_ms(1600);
}

void disp_midi_light() {
    if(old_spinner_phase != spinner_phase) return; // too early to change, the old one wasn't shown yet

    if(spinner_phase == SPINNER_PHASES_MAX) spinner_phase = 0;
    else spinner_phase++;
}

void disp_ch_swap(bool is_swap) {
    is_chswap = is_swap;
}

void disp_draw_home_top() {
    static char buf[17] = { 0 };
    uint16_t env = (status_regi_get_blocking(AY_REGI_ENV_ROUGH) << 8) | status_regi_get_blocking(AY_REGI_ENV_FINE);
    uint8_t lv_a = status_regi_get_blocking(AY_REGI_LVL_A) & 0x0F;
    uint8_t lv_b = status_regi_get_blocking(AY_REGI_LVL_B) & 0x0F;
    uint8_t lv_c = status_regi_get_blocking(AY_REGI_LVL_C) & 0x0F;

    char vol_a_top = (a_bar > 8) ? CHAR_ICON_BAR_LUT[a_bar - 8] : CHAR_ICON_BAR_LUT[0];
    char vol_b_top = (b_bar > 8) ? CHAR_ICON_BAR_LUT[b_bar - 8] : CHAR_ICON_BAR_LUT[0];
    char vol_c_top = (c_bar > 8) ? CHAR_ICON_BAR_LUT[c_bar - 8] : CHAR_ICON_BAR_LUT[0];

    sprintf(buf, "%c%.1X  %c%.1X  %c%1.X %.4X", vol_a_top, lv_a, vol_b_top, lv_b, vol_c_top, lv_c, env);
    lcd.setCursor(0, 0);
    lcd.print(buf);
    lcd.write(CHAR_SPINNER);
}

void disp_draw_home_bottom() {
    static char buf[17] = { 0 };
    uint16_t tone_a = ((status_regi_get_blocking(1) & 0xF) << 8) | status_regi_get_blocking(0);
    uint16_t tone_b = ((status_regi_get_blocking(3) & 0xF) << 8) | status_regi_get_blocking(2);
    uint16_t tone_c = ((status_regi_get_blocking(5) & 0xF) << 8) | status_regi_get_blocking(4);
    uint8_t noise = status_regi_get_blocking(AY_REGI_NOISE) & 0x1F;

    char vol_a_bot = (a_bar <= 8) ? CHAR_ICON_BAR_LUT[a_bar] : CHAR_ICON_BAR_LUT[8];
    char vol_b_bot = (b_bar <= 8) ? CHAR_ICON_BAR_LUT[b_bar] : CHAR_ICON_BAR_LUT[8];
    char vol_c_bot = (c_bar <= 8) ? CHAR_ICON_BAR_LUT[c_bar] : CHAR_ICON_BAR_LUT[8];

    sprintf(buf, "%c%.3X%c%.3X%c%.3X %.2X", vol_a_bot, tone_a, vol_b_bot, tone_b, vol_c_bot, tone_c, noise);
    lcd.setCursor(0, 1);
    lcd.print(buf);
    lcd.write(is_chswap ? CHAR_ICON_CHSWAP : CHAR_ICON_NO_CHSWAP);
}

void disp_tick() {
    unsigned long time = millis();
   if(!lcd.busy() && time % 16 == 0)  {
        uint8_t lv_a = status_regi_get_blocking(AY_REGI_LVL_A) & 0x0F;
        if(a_bar < lv_a) a_bar ++; 
        else if(a_bar > lv_a) a_bar --;

        uint8_t lv_b = status_regi_get_blocking(AY_REGI_LVL_B) & 0x0F;
        if(b_bar < lv_b) b_bar ++; 
        else if(b_bar > lv_b) b_bar --;

        uint8_t lv_c = status_regi_get_blocking(AY_REGI_LVL_C) & 0x0F;
        if(c_bar < lv_c) c_bar ++; 
        else if(c_bar > lv_c) c_bar --;

        if(msg_dur == 0) {
            if(time != last_millis) disp_spinner_update(false);
            disp_draw_home_top();
            disp_draw_home_bottom();
        } else if (time != last_millis) {
            msg_dur--;
        }

        if(midi_in_sts > 0) midi_in_sts--;
        last_millis = time;
   }

   lcd.loop();
}

void disp_begin() {
    inf_log(F("Setup LCD"));
    lcd.init();
    #ifdef ASYNCCRYSTAL_VFD
        lcd.vfd_brightness(BRIGHT_50); // seems to be enough to be super bright as is
    #endif
    disp_intro();

    disp_spinner_update(true);

    lcd.createChar(CHAR_ICON_CHSWAP, MODE_ICON_ACB);
    lcd.flush();
    for(int i = 0; i < 5; i++) {
        lcd.createChar(CHAR_ICON_BAR_START + i, &ICON_BARS[i * 8]);
        lcd.flush();
    }

    lcd.backlight();
    lcd.noCursor();
    lcd.flush();
}