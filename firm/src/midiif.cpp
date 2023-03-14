#include <Arduino_FreeRTOS.h>
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

MIDI_CREATE_CUSTOM_INSTANCE(HardwareSerial, Serial, MIDI, MySettings);

void midi_on_sysex(byte* array, unsigned size) {
    if(size < 3) return; // no valid messages for DRAFTv1.0 are less than 2bytes+1
    if(array[0] != AYYM_SYSEX_VENDOR_CODE) return; // this message is not for us

    disp_midi_light();

    // Todo: parse packet, swap channels if needed, put register writes onto queue

    size_t now_offs = 1;
    while(now_offs < size) { // because WRITEPAIR can be followed by another command
        byte* ayymidi_pkt = array+now_offs;
        size_t pkt_ptr = 0;
        ayymidi_cmd_num_t cmd = AYYMIDI_HDR_FETCH_CMD(ayymidi_pkt[pkt_ptr]);

        switch (cmd)
        {
        case ayymidi_cmd_num_t::SET_CLOCK:
            {
                bool is_acb = (ayymidi_pkt[pkt_ptr] & 0b00000001) != 0;

                uint8_t clock_bytes[4] = {
                    (uint8_t) ((ayymidi_pkt[pkt_ptr] & 0b00010000) << 3),
                    (uint8_t) ((ayymidi_pkt[pkt_ptr] & 0b00001000) << 4),
                    (uint8_t) ((ayymidi_pkt[pkt_ptr] & 0b00000100) << 5),
                    (uint8_t) ((ayymidi_pkt[pkt_ptr] & 0b00000010) << 6),
                };
                clock_bytes[0] |= ayymidi_pkt[++pkt_ptr] & 0x7F;
                clock_bytes[1] |= ayymidi_pkt[++pkt_ptr] & 0x7F;
                clock_bytes[2] |= ayymidi_pkt[++pkt_ptr] & 0x7F;
                clock_bytes[3] |= ayymidi_pkt[++pkt_ptr] & 0x7F;

                uint32_t clock = *((uint32_t*) &clock_bytes);

                // TODO: set frequency and modes
                ay_set_clock(); __UNUSED_ARG(clock); __UNUSED_ARG(is_acb);
                ay_reset();

            }
            break;

        case ayymidi_cmd_num_t::WRITE_PAIR:
            {
                uint8_t regi = (ayymidi_pkt[pkt_ptr] & 0b00011110) >> 1;
                uint8_t valu = (ayymidi_pkt[pkt_ptr] & 0b00000001) << 7;
                pkt_ptr += 1;
                valu |= ayymidi_pkt[pkt_ptr] & 0x7F;

                ay_out(regi, valu);
            }
            break;
        }

        now_offs += pkt_ptr;
    }
}

void midi_on_reset() {
    ay_reset();
}

void _midi_task(void *pvParameters) {
    __UNUSED_ARG(pvParameters);
    for(;;)
        MIDI.read();
}

void midi_begin() {
    MIDI.begin();
    MIDI.setHandleSystemExclusive(midi_on_sysex);
    MIDI.setHandleSystemReset(midi_on_reset);

    xTaskCreate(
    _midi_task
    ,  "MIDIIF"
    ,  MIDIIF_STACKSZ
    ,  NULL
    ,  MIDIIF_PRIORITY
    ,  NULL );
}