#include <MIDI.h>
#include <util.h>
#include <midiif.h>
#include <disp.h>
#include <ay.h>

struct MySettings : public midi::DefaultSettings
{
    static const unsigned SysExMaxSize = MIDIIF_MAX_SYSEX;
    static const unsigned BaudRate = MIDIIF_BAUD;
};

#ifndef USE_SOFTSERIAL
MIDI_CREATE_CUSTOM_INSTANCE(HardwareSerial, Serial, MIDI, MySettings);
#else
#include <SoftwareSerial.h>
SoftwareSerial mySerial(12, 11);
MIDI_CREATE_CUSTOM_INSTANCE(SoftwareSerial, mySerial, MIDI, MySettings);
#endif

static bool chswap_active = false;

inline void remap_channels_in_place(uint8_t* regi, uint8_t* valu);

#ifdef USE_CHAN_TRANSFORM
inline void remap_channels_in_place(uint8_t* regi, uint8_t* valu) {
    if(!chswap_active) return; // ABC data and ABC wiring, nothing to do here

    // In here we need to swap everything related to channel C into channel B and vice-versa
    uint8_t old_regi = *regi;
    switch(old_regi) {
        // tone B -> C
        case AY_REGI_TONE_B_COARSE:
            *regi = AY_REGI_TONE_C_COARSE;
            break;
        case AY_REGI_TONE_B_FINE:
            *regi = AY_REGI_TONE_C_FINE;
            break;
        
        // tone C -> B
        case AY_REGI_TONE_C_COARSE:
            *regi = AY_REGI_TONE_B_COARSE;
            break;
        case AY_REGI_TONE_C_FINE:
            *regi = AY_REGI_TONE_B_FINE;
            break;

        // volume B -> C
        case AY_REGI_LVL_B:
            *regi = AY_REGI_LVL_C;
            break;
        
        // volume C -> B
        case AY_REGI_LVL_C:
            *regi = AY_REGI_LVL_B;
            break;
        
        // mixer is a bit of a special case...
        case AY_REGI_MIXER:
            {
                uint8_t old_valu = *valu;
                *valu = (old_valu & 0b11001001) // removed bits C and B from mixer
                    | ((old_valu & 0b00100100) >> 1) // moved bits C into B
                    | ((old_valu & 0b00010010) << 1); // moved bits B into C
            }
            break;

        default: break;
    }
}
#else
inline void remap_channels_in_place(uint8_t* regi, uint8_t* valu) {
    // Building without channel transforms, do nothing!
}
#endif


void midi_on_sysex(byte* array, unsigned size) {
    // array structure: F0 <vendor code> <rest of data> F7
    if(array[1] != AYYM_SYSEX_VENDOR_CODE) return; // this message is not for us
    inf_log(F("Caught Sysex Size %u"), size);

    disp_midi_light();
    digitalWrite(13,1);

    size_t now_offs = 2;
    while(now_offs < size-1) { // because at the end we also have 0xF7
        byte* ayymidi_pkt = &array[now_offs];
        size_t pkt_ptr = 0;
        ayymidi_cmd_num_t cmd = AYYMIDI_HDR_FETCH_CMD(ayymidi_pkt[pkt_ptr]);

        switch (cmd)
        {
        case ayymidi_cmd_num_t::SET_CLOCK:
            {
                bool is_acb = (ayymidi_pkt[pkt_ptr] & 0b00000001) != 0;

#ifdef USE_CHAN_TRANSFORM
#if !defined(CHANNELS_ABC) && !defined(CHANNELS_ACB)
#error Must select the hardwired channel layout or disable software transform.
#elif defined(CHANNELS_ABC)
                chswap_active = is_acb; // ACB data but ABC wiring
#elif defined(CHANNELS_ACB)
                chswap_active = !is_acb; // ABC data but ACB wiring
#endif
#else
    __UNUSED_ARG(is_acb);
#endif

                disp_ch_swap(chswap_active);

                uint8_t clock_bytes[4] = {
                    (uint8_t) ((ayymidi_pkt[pkt_ptr] & 0b00010000) << 3),
                    (uint8_t) ((ayymidi_pkt[pkt_ptr] & 0b00001000) << 4),
                    (uint8_t) ((ayymidi_pkt[pkt_ptr] & 0b00000100) << 5),
                    (uint8_t) ((ayymidi_pkt[pkt_ptr] & 0b00000010) << 6),
                };
                pkt_ptr++;
                clock_bytes[0] |= ayymidi_pkt[pkt_ptr] & 0x7F;
                pkt_ptr++;
                clock_bytes[1] |= ayymidi_pkt[pkt_ptr] & 0x7F;
                pkt_ptr++;
                clock_bytes[2] |= ayymidi_pkt[pkt_ptr] & 0x7F;
                pkt_ptr++;
                clock_bytes[3] |= ayymidi_pkt[pkt_ptr] & 0x7F;

                uint32_t clock = *((uint32_t*) &clock_bytes);

                ay_set_clock(clock); 
                ay_reset();
                inf_log(F("MIDI I/F SET_CLOCK: Clock= %lu Hz, ACB= %u, ChSwap Active= %u"), clock, is_acb, chswap_active);
            }
            break;

        case ayymidi_cmd_num_t::WRITE_PAIR:
            {
                uint8_t regi = (ayymidi_pkt[pkt_ptr] & 0b00011110) >> 1;
                uint8_t valu = (ayymidi_pkt[pkt_ptr] & 0b00000001) << 7;
                pkt_ptr ++;
                valu |= ayymidi_pkt[pkt_ptr] & 0x7F;

                dbg_log(F("MIDI I/F WRITE_PAIR: Reg=%01x Val=%02x"), regi, valu);
                if(chswap_active) {
                    remap_channels_in_place(&regi, &valu);
                    dbg_log(F("MIDI I/F WRITE_PAIR ReMap: Reg=%01x Val=%02x"), regi, valu);
                }
                ay_out(regi, valu);
            }
            break;
        }

        now_offs += pkt_ptr + 1;
    }

    digitalWrite(13,0);

}

void midi_on_reset() {
    inf_log(F("MIDI reset..."));
    ay_reset();
}

void midi_tick() {
    MIDI.read();
}

void midi_begin() {
    pinMode(13, OUTPUT);
    inf_log(F("MIDI I/F start"));
    MIDI.begin();
    MIDI.turnThruOff();
    MIDI.setHandleSystemExclusive(midi_on_sysex);
    MIDI.setHandleSystemReset(midi_on_reset);
}