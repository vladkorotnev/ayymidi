import freq_note_converter, struct
from ayymidi.sysex_proto import *
from midiutil import MIDIFile
from midiutil.MidiFile import NoteOn, NoteOff

############ AY to Note On-Off Events ##############
# This is used for the front panel of the Casio FD-1, so all events should
# land on CH4 after conversion to SMF0

TONE_VOL_TBL = [
    0,
    16,
    16,
    32,
    32,
    64,
    64,
    64,
    80,
    80,
    80,
    88,
    96,
    104,
    112,
    120,
    127
]

class AyState():
    def __init__(
        self,
        tones=[1,1,1],
        mixers=[False, False, False],
        levels=[0,0,0],
        tone_channels=[3,3,3],
        ignore_tones=[],
        frequency=1750000
    ):
        self.tones = tones
        self.mixers= mixers
        self.levels= levels
        self.notes = [None, None, None]
        self.tone_channels = tone_channels
        self.ignored_tones = ignore_tones
        self.clock = frequency

    def changeRegister(self, register, valu):
        if register == 0:
            self.tones[0] = (self.tones[0] & 0xF00) | valu & 0xFF
        elif register == 1:
            self.tones[0] = (self.tones[0] & 0xFF) | ((valu & 0xF) << 8)
        elif register == 2:
            self.tones[1] = (self.tones[1] & 0xF00) | valu & 0xFF
        elif register == 3:
            self.tones[1] = (self.tones[1] & 0xFF) | ((valu & 0xF) << 8)
        elif register == 4:
            self.tones[2] = (self.tones[2] & 0xF00) | valu & 0xFF
        elif register == 5:
            self.tones[2] = (self.tones[2] & 0xFF) | ((valu & 0xF) << 8)
        elif register == 7:
            self.mixers = [((valu & 0x1) > 0), ((valu & 0x2) > 0), ((valu & 0x4) > 0)]
        elif register == 8 or register == 9 or register == 0xA:
            self.levels[register - 8] = valu & 0x7

    def copy(self):
        n = AyState(tones=self.tones.copy(), mixers=self.mixers.copy(), levels=self.levels.copy(), tone_channels=self.tone_channels, ignore_tones=self.ignored_tones, frequency=self.clock)
        n.notes = self.notes.copy()
        return n

    def getMidiEventsForTransitionFrom(self, origin, timestamp, origin_timestamp):
        rslt = []
        for tone_no in range(3):
            if tone_no in self.ignored_tones:
                continue
            tone_chn = self.tone_channels[tone_no]
            this_tone = self.tones[tone_no]
            if this_tone == 0:
                this_tone = 1
            origin_tone = origin.tones[tone_no]
            if origin_tone == 0:
                origin_tone = 1

            this_osc_frq = self.clock / (16 * this_tone)
            origin_osc_frq = self.clock / (16 * origin_tone)
            this_note = freq_note_converter.from_freq(this_osc_frq)
            origin_note = freq_note_converter.from_freq(origin_osc_frq)
            this_mix = self.mixers[tone_no]
            origin_mix = origin.mixers[tone_no]
            this_vol = self.levels[tone_no]
            origin_vol = origin.levels[tone_no]

            if this_note.note != origin_note.note:
                if self.notes[tone_no] is None or abs((this_note.note_index + this_note.offset_from_note) - (self.notes[tone_no].note_index + self.notes[tone_no].offset_from_note)) > 0.7:
                    # Turn on new note
                    rslt.append(NoteOn(tone_chn, this_note.note_index, timestamp, 1, TONE_VOL_TBL[this_vol]))
                    # Turn off old note
                    if self.notes[tone_no] is not None:
                        rslt.append(NoteOff(tone_chn, self.notes[tone_no].note_index, timestamp, 64))
                    self.notes[tone_no] = this_note

        return rslt

##### Actual File Writing Logic

class AyyMidiWriter():
    def __init__(self, timebase, tempo):
        self.state = AyState()
        self.file = MIDIFile(
            numTracks=5,
            removeDuplicates=False,
            deinterleave=False,
            adjust_origin=True,
            file_format=1,
            ticks_per_quarternote=timebase,
            eventtime_is_ticks=True
        )
        self.file.addTempo(0, 0, tempo)
        self.lastTimestamp = 0

    def setVisIgnoreToneIndexes(self, indexes):
        self.state.ignored_tones = indexes

    def setClock(self, clock, is_acb):
        self.file.addSysEx(0, 0, MFGID_AYYM_MSG, clock_to_sysex_payload(clock, is_acb))
        self.state.clock = clock

    def writeRegisterChangeAccumulator(self, accum, timestamp):
        nextstate = self.state.copy()
        for reg, val in zip(accum[::2], accum[1::2]):
            nextstate.changeRegister(reg, val)
        evts = nextstate.getMidiEventsForTransitionFrom(self.state, timestamp, self.lastTimestamp)
        self.state = nextstate
        self.file.tracks[3].eventList.extend(evts)
        payload = regi_accum_to_sysex_payload(accum)
        self.file.addSysEx(0, timestamp, MFGID_AYYM_MSG, payload)
        self.lastTimestamp = timestamp

    def writeFile(self, fname):
        with open(fname, 'wb') as outf:
            self.file.writeFile(outf)
