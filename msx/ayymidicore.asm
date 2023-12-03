WRTPSG	equ	#0093
RDSLT   equ #000C
WRSLT   equ #0014
; https://map.grauw.nl/resources/midi/ym2148.php
MIDI_CMD_PRT equ #3ff6
MIDI_READ_PRT equ #3ff5
MIDI_STS_PRT equ #3ff6
MIDI_SLTID equ #8F ; F000EEPP -> F=1, Pri=3, Ext=3

AYYMIDI_VENDOR_ID equ #7A
AYYMIDI_CMD_SETCLOCK equ #03
AYYMIDI_CMD_WRITEPAIR equ #02

PSG_RST
	ld	b,13
PSGini	ld	a,b	; 
	ld	e,0	; 8 least significant bits
	cp	7
	jr	nz,NoR7	; Jump if register different from 7
	ld	e,10111111b	; Bit 7 to 1 and bit 6 to 0
NoR7	call	WRTPSG
	djnz	PSGini	; Loop to initialize registers 
    RET

MIDI_RST
    LD A, MIDI_SLTID
    ld hl, MIDI_CMD_PRT
    ld E, #80 ; reset
    CALL WRSLT
    LD A, MIDI_SLTID
    ld hl, MIDI_CMD_PRT
    ld E, #5 ; rx and tx
    CALL WRSLT
    RET

MIDI_WAIT_DATA ; wait byte and return in A
    PUSH HL
    ld hl, MIDI_STS_PRT
_mwaitlp
    LD A, MIDI_SLTID
    CALL RDSLT
    and 2 ; RxRDY bit
    jp z, _mwaitlp
    ; RxRDY now 1
    ld hl, MIDI_READ_PRT
    LD A, MIDI_SLTID
    CALL RDSLT
    POP HL
    RET

FM_PROGGRAM
    CALL PSG_RST
    CALL MIDI_RST
_wait_sysex_begin
    CALL MIDI_WAIT_DATA
    CP #F0
    JP NZ, _wait_sysex_begin
    ; Now in SYSEX message
    CALL MIDI_WAIT_DATA
    CP A, AYYMIDI_VENDOR_ID
    JP NZ, _wait_sysex_end ; not AYYMIDI, skip
_next_cmd    CALL MIDI_WAIT_DATA
    ; Now have COMMAND byte OR sysex end
    CP #F7
    JP Z, _wait_sysex_begin
    LD C, A
    RRA : RRA : RRA : RRA : RRA : AND #03
    CP A, AYYMIDI_CMD_WRITEPAIR
    JP NZ, _wait_sysex_end ; PSG cannot reclock on MSX, go sleep stupid user
    LD A, C
    RRA : AND #F ; now A = register number
    LD B, A ; now B = register number
    CP #E
    JP Z, _wait_iowrite_end ; No write to IO port
    CP #F
    JP Z, _wait_iowrite_end ; No write to IO port
    CALL MIDI_WAIT_DATA ; receive 7 LSB
    AND #7F ; just in case
    LD D, A ; save 7 LSB to D temporary
    LD A, C ; restore command byte
    RLA : RLA : RLA : RLA : RLA : RLA : RLA : AND #80 ; put MSB from command byt into A
    OR D ; put the rest of LSB into A
    ; now B = register number, A = register value
    LD E, A
    LD A, B
    CALL WRTPSG
    JP _next_cmd
_wait_sysex_end
    CALL MIDI_WAIT_DATA
    CP A, #F7
    JP NZ, _wait_sysex_end
    JP _wait_sysex_begin
_wait_iowrite_end
    ; IOWrite is print string 2 lines
    CALL MIDI_WAIT_DATA ; skip Time In Seconds
    CALL AYYMID_STRING
    CALL AYYMID_STRING
    JP _wait_sysex_end


AYYMID_STRING
    LD HL, STRBUF
_readstr1    CALL MIDI_WAIT_DATA
    LD (HL),A
    INC HL
    CP 0
    JP NZ, _readstr1
    INC HL
    LD (HL), #A
    INC HL
    LD (HL), 0
    LD HL, STRBUF
    JP PRINTSTR