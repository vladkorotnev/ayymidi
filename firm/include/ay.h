#ifndef AY_H
#define AY_H
#include <stdint.h>

void ay_reset();
void ay_out(uint8_t port, uint8_t data);
void ay_set_clock(uint32_t clock);

#define AY_CH_A 0
#define AY_CH_B 1
#define AY_CH_C 2

void sidfx_start(uint8_t channel);
void sidfx_stop();

#define AY_REGI_TONE_A_FINE 0x0
#define AY_REGI_TONE_A_COARSE 0x1
#define AY_REGI_TONE_B_FINE 0x2
#define AY_REGI_TONE_B_COARSE 0x3
#define AY_REGI_TONE_C_FINE 0x4
#define AY_REGI_TONE_C_COARSE 0x5
#define AY_REGI_NOISE 0x6
#define AY_REGI_MIXER 0x7
#define AY_REGI_LVL_A 0x8
#define AY_REGI_LVL_B 0x9
#define AY_REGI_LVL_C 0xA
#define AY_REGI_ENV_FINE 0xB
#define AY_REGI_ENV_ROUGH 0xC
#define AY_REGI_ENV_SHAPE 0xD
#define AY_REGI_IO_A 0xE
#define AY_REGI_IO_B 0xF

#endif