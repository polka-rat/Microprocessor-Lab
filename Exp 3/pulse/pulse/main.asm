.include "m8def.inc"

LDI R16 , 0xFF ; All bits set as 0 to make port as output
OUT DDRD, R16 ; DDRB = Data Direction Register for PORT B
again:
LDI R16, 0x01
OUT PORTD, R16	;LED output is HIGH

LDI R17, 0xF9 ; Creating a Delay Cycle to give a pulse width of 0.5 sec
LOOP4:LDI R18, 0xC8
LOOP5:LDI R19, 0x0A
LOOP6:DEC R19
BRNE LOOP6
DEC R18
BRNE LOOP5
DEC R17
BRNE LOOP4

LDI R16, 0x00
OUT PORTD, R16	;LED output is LOW

LDI R17, 0xF9	;Creating another Delay Cycle
LOOP1:LDI R18, 0xC8
LOOP2:LDI R19, 0x0A
LOOP3:DEC R19
BRNE LOOP3
DEC R18
BRNE LOOP2
DEC R17
BRNE LOOP1

RJMP again	; Creating an infinite loop to generate continuous pulses



