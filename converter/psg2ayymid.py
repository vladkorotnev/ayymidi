#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os, sys, pdb, struct
from argparse import ArgumentParser
from ayymidi.mid_writer import AyyMidiWriter

# PSG uses 20ms delays or multiples of 80ms delays
# So let's make one tick 10 ms

# TICK = 60000 / (BPM * PPQ)
# let BPM=120, PPQ=50
# TICK = 60000 / (50*120) = 10ms

PSG_TIMEBASE = 50
PSG_TEMPO = 120
PSG_CLOCK = 1750000
TICKS_IN_FF_COMMAND = 2
TICKS_IN_MULTIPLE_COMMAND = 8

parser = ArgumentParser(prog='PSG2AYYMidi', description="Converts PSG of AY-3-8910 to MID for playback with FD-1 + AYYMIDI")
parser.add_argument('--acb', dest='is_acb', action='store_true', default=False, help='Write ACB stereo flag (instead of ABC)')
parser.add_argument('-t', '--ignore-tone', dest='ignore_tone', nargs='+', type=int, help="Don't generate Note events for set channels")
parser.add_argument('infile')
parser.add_argument('outfile')
args = parser.parse_args()

WRITER=AyyMidiWriter(PSG_TIMEBASE, PSG_TEMPO)
if args.ignore_tone is not None:
    WRITER.setVisIgnoreToneIndexes(args.ignore_tone)
WRITER.setClock(PSG_CLOCK, args.is_acb)

INFILE=open(args.infile, 'rb').read()
if INFILE[0:4] != b"PSG\x1A":
    print("Not PSG file")
    os.exit(1)

if INFILE[4] != 0x0:
    print("New PSG format, todo: read freq, abort!")
    os.exit(1)

start_pos = 4
while INFILE[start_pos] != 0xFF:
    start_pos += 1

INDAT=INFILE[start_pos::]
pos = 0
timestamp = 0
eos = False
accum = []

def put_accum():
    global accum
    if len(accum) >= 2:
        WRITER.writeRegisterChangeAccumulator(accum, timestamp)
        accum = []

while not eos and pos < len(INDAT):
    cmd = INDAT[pos]
    if cmd == 0xFD:
        put_accum()
        eos = True
    elif cmd == 0xFF:
        put_accum()
        timestamp += TICKS_IN_FF_COMMAND
    elif cmd == 0xFE:
        put_accum()
        pos += 1
        count = INDAT[pos]
        timestamp += TICKS_IN_MULTIPLE_COMMAND * count
    else:
        regi = cmd
        pos += 1
        valu = INDAT[pos]
        accum.append(regi)
        accum.append(valu)
    pos += 1


WRITER.writeFile(args.outfile)