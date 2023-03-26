# AYYMIDI

*Ayy MIDI*

A project to allow listening to ZX Spectrum music (AY/YM) from a MIDI player such as a Roland Sound Brush or Casio FD-1.

## Hardware

### BOM

* Arduino Nano (328)
* YMF2149 or AY-3-8910 SSG
* 1602 LCD + I2C expander shield
* SN7407 buffer
* 6N138 photocoupler
* 10k 1/6W resistor: 1pcs
* 1k 1/6W resistor: 3pcs
* 2.2k resistor: 4pcs
* 220 ohm resistor: 3pcs (1pcs if not installing MIDI THRU)
* 10uF electrolytic capacitor: 2pcs

### Schematic

* [Schematic in DipTrace format](hw/schematic.dch)
* [Schematic in PNG format](hw/schematic.png)

### PCB

I reused the PCB from my old SD based player with some wires and hacks on top of it, so I didn't design one anew. 
Feel free to do whatever you feel comfortable to wire things together.

## Software

### Protocol

Protocol description is in a [separate file](PROTOCOL.md)

### Firmware

Currently there is a firmware for Arduino Nano based on Atmega328 in the [firm](firm) folder.

### Converter

There are some conversion tools to create AYYMIDI data from various file formats in the [converter](converter) directory.

* [VGM2AYYMID](converter/vgm2ayymid.py): convert VGM files containing AY-3-8910 dumps to AYYMIDI
* [PSG2AYYMID](converter/psg2ayymid.py): convert PSG files (e.g. from ZXTune or Bulba's player) to AYYMIDI

## References

This project is heavily based around [Arduino ZX Spectrum AY Player](https://habr.com/ru/post/392625/) by Z80A.

For Async I2C it's using [asynci2cmaster](https://github.com/cskarai/asynci2cmaster) by cskarai.

Also special thanks for all sorts of help and support:

* [goutsune](https://github.com/goutsune)
* [裕之](http://d4.princess.ne.jp/)

----

by akasaka, 2023
