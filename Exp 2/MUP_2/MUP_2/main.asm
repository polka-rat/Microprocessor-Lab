.CSEG ; define memory space to hold program - code
 LDI ZL , LOW ( NUM <<1) ; load byte addrss of LSB of word
 LDI ZH , HIGH ( NUM <<1) ; load byte addrss of MSB of word
 LDI XL ,0x60 ; load SRAM LSB of 16 - bit address in X -
 LDI XH ,0x00 ; above ’s MSB | word address can be line number
 LDI R16 ,00 ; clear R16 , used to hold answer
 LPM R0 , Z+ ; Z now follows byte addrssng. points MSB of NUM
LPM R1 , Z ; Get second number ( LSB of NUM addr ) into R1 ,
 CP R0 , R1 ; Compare R0 and R1 , result in carry flag

 BRCC abc ; jump if no carry ,
 LDI R16 ,0x01 ; else make register holding answer
abc : ST X + , R0 ; store result in given address in SRAM ie

 ST X , R16 ; store answer in next location 0 x61
 NOP ; End of program , No operation
NUM : .db 0x03 ,0x5F 