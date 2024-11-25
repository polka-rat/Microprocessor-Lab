.CSEG ; define memory space to hold program - code

 LDI ZL , LOW ( NUM <<1) ; load byte addrss of LSB of word
 LDI ZH , HIGH ( NUM <<1) ; load byte addrss of MSB of word
 LDI XL ,0x60 ; load SRAM LSB of 16 - bit address in X -
 LDI XH ,0x00 ; above ’s MSB | word address can be line number
 LDI R16 ,00 ; clear R16 , used to hold answer
 LPM R0 , Z+ ; Z now follows byte addrssng. points MSB of NUM
LPM R1 , Z+ ; Get second number ( LSB of NUM addr ) into R1 ,
LPM R2, Z+;
LPM R3, Z;
 CP R0 , R1 ; Compare R0 and R1 , result in carry flag
BRCC abc ; jump if no carry ,
 MOV R0,R1 ; else make register holding answer
abc: CP R0,R2;
BRCC bcd ; jump if no carry ,
 MOV R0,R2 ; else make register holding answer
 BRCC bcd ; jump if no carry ,
 MOV R0,R2 ; else make register holding answer
bcd: CP R0,R3
BRCC cde;
MOV R0,R3;
cde: ST X + , R0 ; store result in given address in SRAM ie
 NOP ; End of program , No operation

NUM : .db 0x04 ,0x05, 0x6F,0xFF