#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os, sys, freq_note_converter, pdb, struct
from argparse import ArgumentParser
from sysex_proto import *
from midiutil import MIDIFile
from midiutil.MidiFile import NoteOn, NoteOff

# Todo: extract the midi writing logic, build PSG reader

parser = ArgumentParser(prog='Vgm2AYYMidi', description="Converts VGM of AY-3-8910 to MID for playback with FD-1 + AYYMIDI")
parser.add_argument('--acb', dest='is_acb', action='store_true', default=False, help='Write ACB stereo flag (instead of ABC)')
parser.add_argument('-t', '--ignore-tone', dest='ignore_tone', nargs='+', type=int, help="Don't generate Note events for set channels")
parser.add_argument('infile')
parser.add_argument('outfile')
args = parser.parse_args()

INFILE=open(args.infile, 'rb').read()
if INFILE[0:4] != b'Vgm ':
    print("Not VGM file")
    os.exit(1)

rate = INFILE[0x74:0x78]
AY_RATE = struct.unpack("<I", rate)[0] & 0x3FFFFFFF
AY_IS_ACB = args.is_acb
if AY_RATE <= 0:
    print("Not AY")
    os.exit(1)
start_offs = struct.unpack("<I", INFILE[0x34:0x38])[0] + 0x34
end_offs = struct.unpack("<I", INFILE[0x04:0x08])[0] + 4
INDAT = INFILE[start_offs:end_offs]

OUTFNAME=args.outfile

OUTFILE=MIDIFile(
    numTracks=5,
    removeDuplicates=False,
    deinterleave=False,
    adjust_origin=True,
    file_format=1,
    ticks_per_quarternote=16000,
    eventtime_is_ticks=True
)
OUTFILE.addTempo(0, 0, 165)
OUTFILE.addSysEx(0, 0, MFGID_AYYM_MSG, clock_to_sysex_payload(AY_RATE, AY_IS_ACB))

TONE_CHN_ASSIGN = [3, 3, 3]
TONE_IGNORE_IDX = args.ignore_tone
if TONE_IGNORE_IDX is None:
    TONE_IGNORE_IDX = []
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
        levels=[0,0,0]
    ):
        self.tones = tones
        self.mixers= mixers
        self.levels= levels
        self.notes = [None, None, None]

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
        n = AyState(tones=self.tones.copy(), mixers=self.mixers.copy(), levels=self.levels.copy())
        n.notes = self.notes.copy()
        return n

    def getMidiEventsForTransitionFrom(self, origin, timestamp, origin_timestamp):
        rslt = []
        for tone_no in range(3):
            if tone_no in TONE_IGNORE_IDX:
                continue
            tone_chn = TONE_CHN_ASSIGN[tone_no]
            this_tone = self.tones[tone_no]
            if this_tone == 0:
                this_tone = 1
            origin_tone = origin.tones[tone_no]
            if origin_tone == 0:
                origin_tone = 1

            this_osc_frq = AY_RATE / (16 * this_tone)
            origin_osc_frq = AY_RATE / (16 * origin_tone)
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


fpos=0
nowtick=0
eos=False
accum=[]


tones = [0, 0, 0]
tonethresh = 4
lasttick = 0
laststate = AyState()

LAST_REGI = 99
REGI = [1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1]
STATISTIC = {
    'inc': 0,
    'dec': 0,
    'zero': 0,
    'total': 0,
    'seq_fwd': 0,
    'seq_bwd': 0
}

while fpos < (end_offs - start_offs) and not eos:
    cmd = INDAT[fpos]
    if cmd == 0xA0:
        regi = INDAT[fpos + 1]
        valu = INDAT[fpos + 2]
        fpos += 2
        if regi < 0x80:
            accum.append(regi)
            accum.append(valu)
            STATISTIC['total'] += 1
            if REGI[regi] - valu == 1:
                STATISTIC['dec'] += 1
            elif valu - REGI[regi] == 1:
                STATISTIC['inc'] += 1
            elif valu == 0:
                STATISTIC['zero'] += 1
            
            if regi - LAST_REGI == 1:
                STATISTIC['seq_fwd'] += 1
            elif LAST_REGI - regi == 1:
                STATISTIC['seq_bwd'] += 1
            LAST_REGI = regi
    else:
        if len(accum) > 0:
            nextstate = laststate.copy()
            for reg, val in zip(accum[::2], accum[1::2]):
                nextstate.changeRegister(reg, val)
            evts = nextstate.getMidiEventsForTransitionFrom(laststate, nowtick, lasttick)
            laststate = nextstate
            OUTFILE.tracks[3].eventList.extend(evts)
            payload = regi_accum_to_sysex_payload(accum)
            print("TICK=", nowtick, "ACCUM=", ([hex(i) for i in payload]))
            OUTFILE.addSysEx(0, nowtick, MFGID_AYYM_MSG, payload)
            lasttick = nowtick
            accum=[]
        if cmd == 0x66:
            eos = True
            break
        elif (cmd & 0xF0) == 0x70:
            ticks = (cmd & 0x0F) + 1
            nowtick += ticks
        elif cmd == 0x63:
            nowtick += 882
        elif cmd == 0x62:
            nowtick += 735
        elif cmd == 0x61:
            ticks = struct.unpack("<H", INDAT[fpos+1:fpos+3])[0]
            nowtick += ticks
            fpos += 2
    fpos += 1

with open(OUTFNAME, 'wb') as outf:
    OUTFILE.writeFile(outf)
