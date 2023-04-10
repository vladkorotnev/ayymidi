# AYYMIDI protocol

* 2023/02/28: draft v1.0
* 2023/03/14: draft v1.0.1: remove sequential write commands as they don't give any size or speed benefit over repeated pair write commands
* 2023/04/07: use port 0xE (IO-A) for text display

## Abstract

The AY registers are sent in a timely manner by a MIDI host to the AYYMIDI device via SysEx messages. The device itself doesn't do any timing work, nor does it accept any other kinds of messages.

The Manufacturer ID for SysEx messages is chosen to be `0x7A` (according to the [list](https://www.midi.org/specifications-old/item/manufacturer-id-numbers), it is unused and reserved for "other use"... I think a hobby project is "other" enough :P)

All of the following SysEx examples do not show the Manufacturer ID prefix and the common MIDI prefix/suffix.

## Set the chip clock rate and reset the chip

### Data

* 5 bytes payload
    * First byte:
        * 1 bit: forbidden
        * 2 bits: command (`SETCLOCK` == 0x3 == 0b11)
        * 4 bits: MS bits of the following 4 bytes
        * 1 bit: 1 if ACB stereo, 0 if ABC stereo
    * Second byte onwards:
        * Respective byte of the clock value in Little-Endian AND 0x7F
* Can be followed by any other command

### Implementation

Upon receiving `SETCLOCK`, the device must clock the target SSG chip with the specified frequency and issue a reset command to it.

## Writing to a specific register

### Data

* 2 bytes payload
    * First byte:
        * 1 bit: forbidden
        * 2 bits: command (`WRITEPAIR` == 0x2 == 0b10)
        * 4 bits: AY register number
        * 1 bit: MS bit of the register value
    * Second byte:
        * Register value AND 0x7F
* Can be followed by any other command

### Special case: IO-A

* Dynamic payload
    * First byte:
        * 1 bit: forbidden
        * 2 bits: command (`WRITEPAIR` == 0x2 == 0b10)
        * 4 bits: AY register number == 0xE (IO-A)
        * 1 bit: MS bit of the (Time In Seconds)
    * Second byte:
        * (Time In Seconds) value AND 0x7F
    * Third byte... Nth byte
        * String for top line of screen (NUL included)
    * N+1th byte... Xth byte
        * String for bottom line of screen (NUL included)
* Can be followed by any other command

### Implementation

Upon receiving `WRITEPAIR` the device must write the provided register value into the provided register number of the SSG, unless the write is for the IO port A.
In such case, the device must parse the packet and display the text strings if it has a display unit.
