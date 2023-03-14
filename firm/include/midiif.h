#ifndef MIDI_H_
#define MIDI_H_
#include <util.h>

#define AYYM_SYSEX_VENDOR_CODE 0x7A
#define MIDIIF_PRIORITY PRIORITY_MAX
#define MIDIIF_STACKSZ 128
#define MIDIIF_MAX_SYSEX 128
#define MIDIIF_BAUD 31250

#ifdef USE_CHAN_TRANSFORM
#if !defined(CHANNELS_ABC) && !defined(CHANNELS_ACB)
#error Must select the hardwired channel layout or disable software transform.
#elif defined(CHANNELS_ABC)
// todo: ACB to ABC transform function
#elif defined(CHANNELS_ACB)
// todo: ABC to ACB transform function
#endif
#endif


typedef enum ayymidi_cmd_num {
    WRITE_PAIR = 0b10,
    SET_CLOCK = 0b11
} ayymidi_cmd_num_t;
#define AYYMIDI_HDR_FETCH_CMD(x) (ayymidi_cmd_num_t)(((x & 0b01100000) >> 5) & 0b11)

void midi_begin();

#endif