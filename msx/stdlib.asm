CHPUT equ #00A2


PRINTSTR:
	ld	a,(hl)		; Load the byte from memory at address indicated by HL to A.
	or	a		; Same as CP 0 but faster.
	ret	z		; Back behind the call print if A = 0
	call CHPUT		; Call the routine to display a character.
	inc	hl		; Increment the HL value.
	jr	PRINTSTR		; Relative jump to the address in the label Print.