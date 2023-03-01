#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os, sys, pdb, struct
from argparse import ArgumentParser
from lib.mid_writer import AyyMidiWriter

VGM_TIMEBASE = 16000
VGM_TEMPO = 165

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

WRITER=AyyMidiWriter(VGM_TIMEBASE, VGM_TEMPO)
WRITER.setClock(AY_RATE, AY_IS_ACB)
if args.ignore_tone is not None:
    WRITER.setVisIgnoreToneIndexes(args.ignore_tone)

fpos=0
nowtick=0
eos=False
accum=[]
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
            WRITER.writeRegisterChangeAccumulator(accum, nowtick)
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

WRITER.writeFile(OUTFNAME)