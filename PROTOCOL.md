# AYYMIDI protocol

* 2023/02/28: draft v1.0
* 2023/03/14: draft v1.0.1: remove sequential write commands as they don't give any size or speed benifit over repeated pair write commands

## Abstract

The AY registers are sent in a timely manner by a MIDI host to the AYYMIDI device via SysEx messages. The device itself doesn't do any timing work, nor does it accept any other kinds of messages.

The Manufacturer ID for SysEx messages is chosen to be `0x7A` (according to the [list](https://www.midi.org/specifications-old/item/manufacturer-id-numbers), it is unused and reserved for "other use"... I think a hobby project is "other" enough :P)

All of the following SysEx examples do not show the Manufacturer ID prefix and the common MIDI prefix/suffix.

## Setting the chip clock rate

* 5 bytes payload
    * First byte:
        * 1 bit: forbidden
        * 2 bits: command (`SETCLOCK` == 0x3 == 0b11)
        * 4 bits: MS bits of the following 4 bytes
        * 1 bit: 1 if ACB stereo, 0 if ABC stereo
    * Second byte onwards:
        * Respective byte of the clock value in Little-Endian AND 0x7F
* Cannot be followed by any other command

## Writing to a specific register

* 2 bytes payload
    * First byte:
        * 1 bit: forbidden
        * 2 bits: command (`WRITEPAIR` == 0x2 == 0b10)
        * 4 bits: AY register number
        * 1 bit: MS bit of the register value
    * Second byte:
        * Register value AND 0x7F
* Can be followed by any other command
