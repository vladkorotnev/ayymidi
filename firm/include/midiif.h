#ifndef MIDI_H_
#define MIDI_H_
#include <util.h>

#define AYYM_SYSEX_VENDOR_CODE 0x7A
#define MIDIIF_PRIORITY 0
#define MIDIIF_STACKSZ 256
#define MIDIIF_MAX_SYSEX 64
#define MIDIIF_BAUD 31250

typedef enum ayymidi_cmd_num {
    WRITE_PAIR = 0b10,
    SET_CLOCK = 0b11
} ayymidi_cmd_num_t;
#define AYYMIDI_HDR_FETCH_CMD(x) (ayymidi_cmd_num_t)(((x & 0b01100000) >> 5) & 0b11)

void midi_begin();
void midi_tick();

#endif