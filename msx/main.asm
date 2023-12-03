HDR
       OUTPUT "prog.bin"
; --> File header
	db	#0fe	; Binary code file
	dw	BEGIN	; Program destination address
	dw	END	; Program end address
	dw	BEGIN	; Program execution address
; ---

 	ORG #b000	 
BEGIN
       LD HL, STR_SEARCHING
       CALL PRINTSTR
 	DI	; Interrupts must be disabled, other ways we can't switch slots in #0000-#3FFF area.
 	JP FM_PROGGRAM
       RET


       INCLUDE "ayymidicore.asm"
       INCLUDE "stdlib.asm"
       INCLUDE "strings.asm"

STRBUF EQU $

END EQU $-1
