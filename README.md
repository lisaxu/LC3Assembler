# LC3Assembler
an assembler for the LC3 assembly language in C

## Purpose
Translates assembly language statement to code (translate ADD R1,R2,R3 to the hex code 1283)

LC3 assembly language the syntax:
```
[optional label] opcode operand(s) [; optional end of line comment]
```

## Main functions
### asm_pass_one()
* Verify that each line is syntactly correct. 
* Whenever a line contains a label, insert it and its address into the symbol table. This is required so that the PCoffset for the LD/ST/LDI/STI/BR/JSR/LEA opcodes can be computed in the second pass.

### asm_pass_two()
* generate the machine code for each instruction. 
* The first step is to copy the prototype from the format field of the information on this line into a variable that will contain the final machine code. 
* The next step is to insert the operand(s) into the correct locations of the machine code. 
