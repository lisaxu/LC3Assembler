; PA7 Assignment
; Author: Fritz Sieker and Chris Wilcox
; Date:   3/10/2014
; Email:  fsieker,wilcox@cs.colostate.edu
; Class:  CS270
;
; Description: Implements the manipulation of half precision (16-bit) floating point numbers

;------------------------------------------------------------------------------
; Begin reserved section: do not change ANYTHING in reserved section!

                .ORIG x3000
                BR MAIN

; A jump table defined as an array of addresses
Functions       .FILL flt16_add      ; (option 0)
                .FILL flt16_sub      ; (option 1)
                .FILL flt16_get_sign ; (option 2)
                .FILL flt16_get_exp  ; (option 3)
                .FILL flt16_get_val  ; (option 4)
                .FILL flt16_abs      ; (option 5)
                .FILL flt16_neg      ; (option 6)
                .FILL left_most1     ; (option 7)
                .FILL left_shift     ; (option 8)
                .FILL right_shift    ; (option 9)
                .FILL binary_or      ; (option 10)

Main            LEA R0,Functions     ; get base of jump table
                LD  R1,Option        ; get option to use, no error checking
                ADD R0,R0,R1         ; add index of array
                LDR R0,R0,#0         ; get address of function
                JSRR R0              ; call selected function
                HALT

; Parameters and return values for all functions
Option          .BLKW 1              ; which function to call
Param1          .BLKW 1              ; space to specify first parameter
Param2          .BLKW 1              ; space to specify second parameter
Result          .BLKW 1              ; space to store result

; You may add variables and functions after here as you see fit. You may use JSR
; within the code of flt16_add or other functions. However, this requires that
; you save and restore return addresses, otherwise a function will not be able
; to use JSR to call another subroutine. See flt16_add for an example of how to
; save and restore a return address. When will this scheme NOT work?

; Here are some useful constants:

SIGN_MASK       .FILL x8000          ; 1 in left most bit 
EXP_MASK        .FILL x001F          ; exactly 5 bits     
VAL_MASK        .FILL x03FF          ; exactly 10 bits    
IMPL1_MASK      .FILL x0400          ; 1 in the 11 bit    
ONE             .FILL #1             ; the number 1       
TEN             .FILL #10            ; the number 10      
SIXTEEN         .FILL #16            ; the value 16

; End reserved section: do not change ANYTHING in reserved section!
;------------------------------------------------------------------------------

; Local variables, this is how you will be tested for PA6
; Do not change the names!
X               .BLKW 1              ; X parameter
Y               .BLKW 1              ; Y parameter
signX           .BLKW 1              ; sign of X
expX            .BLKW 1              ; exponent of X
valX            .BLKW 1              ; mantissa of X
signY           .BLKW 1              ; sign of Y
expY            .BLKW 1              ; exponent of Y
valY            .BLKW 1              ; mantissa of Y
signSum         .BLKW 1              ; sign of sum
expSum          .BLKW 1              ; exponent of sum
valSum          .BLKW 1              ; mantissa of sum
left1           .BLKW 1              ; location of left most 1
 tempSum			.BLKW 1				;place to store temp sum when combining the parts
flt16_add_ra    .BLKW 1              ; return address

flt16_add       ST R7,flt16_add_ra   ; save return address

                LD R0,Param1         ; read X parameter
                ST R0,X              ; store X parameter
                LD R0,Param2         ; read Y parameter
                ST R0,Y              ; store Y parameter

                ; STEP ONE) Extract fields from operands---------------------------------------------------------
				JSR flt16_get_sign		;get sign of x, param1 still has the x value
				LD R0,Result
				ST R0,signX
				ST R0,Param2			;parepare for value call
				
				JSR flt16_get_val 
				LD R0,Result			;
				ST R0,valX				
				
				JSR flt16_get_exp 
				LD R0,Result
				ST R0,expX

				LD R0,Y
				ST R0,Param1
				JSR flt16_get_sign		;get sign of y, param1 now has the y value
				LD R0,Result
				ST R0,signY
				ST R0,Param2
				
				JSR flt16_get_val 
				LD R0,Result
				ST R0,valY	
				
				JSR flt16_get_exp 
				LD R0,Result
				ST R0,expY
	
                ; STEP TWO) Equalize operand exponents (or copy exponent)
				ST R0,expSum			;copy y's exponent to sum's exp
                ; STEP THREE) Convert operands to 2's complement(already did in the getVal step)
                ; STEP FOUR) Add mantissas
				LD R0,valX
				LD R1,valY
				ADD R3,R0,R1			;add mantissa		
				ST R3,valSum			;valsum is now unabs,unnormalized
				BRz sum_zero_exit
				
                ; STEP FIVE) Convert sum from 2's complement
				ST R3,Param1			;prepare to call get sign
				JSR flt16_get_sign
				LD R3,valSum			;reload mentissa
				LD R0,Result			;get sign
				BRz sum_pos				;if sign is zero, continue to step six
				NOT R3,R3				;else, convert neg sumval to pos 
				ADD R3,R3,#1
				LD R1,ONE
				ST R1,signSum			;set sum sign to one			
sum_pos			ST R3,valSum			;valSum is now absolute						
                
				; STEP SIX) Normalize sum
				ST R3,Param1			;prepare to call left_most1, param1 is the abs sum mentissa
				JSR left_most1 
				LD R3,valSum			;reload mentissa
				LD R0,Result			;left most i should be at 10th bit
				ADD R0,R0,#-10			;R0 - 10
				BRz sum_val_normalized	; if it's already normalized, go to next step
				BRp mantissa_overflow	;if mentissa overflowed, shift it right
				;else, R0 is negative, shift it left
				LD R1,expSum
				ADD R5,R0,R1			;decrement exponent
				ST R5,expSum			;store updated exp
				NOT R0,R0
				ADD R0,R0,#1
				ST R0,Param2			;prepare to call left shift
				JSR left_shift
				LD R3,Result			;fetch result
				BR sum_val_normalized 
mantissa_overflow	
				ST R0,Param2			;prepare to call right shift r0 bits
				LD R1,expSum
				ADD R0,R0,R1			;increment exponent
				ST R0,expSum			;store updated exp
				JSR right_shift
				LD R3,Result			;fetch the shifted mentissa
sum_val_normalized 
				ST R3,valSum                
				; STEP SEVEN) Compose sum from fields
				LD R0,signSum			;prepare to call left shift to place sign
				ST R0,Param1
				AND R1,R1,#0
				ADD R1,R1,#15
				ST R1,Param2
				JSR left_shift
				LD R3,Result
				ST R3,tempSum
				
				LD R0,expSum			;prepare to call left shift to place exponent
				ST R0,Param1
				AND R1,R1,#0
				ADD R1,R1,#10
				ST R1,Param2
				JSR left_shift
				LD R0,Result
				LD R3,tempSum
				ADD R3,R3,R0
				
				LD R0,valSum
				LD R1,IMPL1_MASK
				NOT R1,R1
				AND R0,R0,R1			;Mask off the implicit one
				ADD R3,R3,R0
				
sum_zero_exit	
				ST R3,Result
				LD R7,flt16_add_ra   ; restore return address
				
                RET
;------------------------------------------------------------------------------
flt16_sub_ra    .BLKW 1              ; return address
 
flt16_sub                            ; Result is Param1 minus Param2
                ST R7,flt16_sub_ra   ; save return address
                LD R0,Param1
				ST R0,tempSum
				LD R0,Param2
				ST R0,Param1
				JSR flt16_neg
				LD R0,Result
				ST R0,Param2
				LD R0,tempSum
				ST R0,Param1
				JSR flt16_add
				
                LD R7,flt16_sub_ra   ; restore return address
                RET
;------------------------------------------------------------------------------
flt16_get_sign                       ; Result is 0 if Param1 is positive, 1 otherwise

                AND R0,R0,#0         ; initialize result
                LD R1,Param1         ; load parameter
                LD R2,SIGN_MASK      ; load sign mask   
                AND R3,R2,R1         ; sign bit set?
                BRz return_sign      ; not set, return 0
                ADD R0,R0,#1         ; set, return 1
return_sign     ST R0,Result         ; save result
                RET
;------------------------------------------------------------------------------
flt16_exp_ra    .BLKW 1              ; return address

flt16_get_exp                        ; Result is biased exponent from Param1
                ST R7,flt16_exp_ra   ; save return address
               
		LD R0,TEN		;right shift param1 by 10 bits
		ST R0,Param2		;prepare args to pass
		JSR right_shift		;call right shift fcn
		LD R1,Result		;get result
		LD R0,EXP_MASK		;prepare mask
		AND R1,R1,R0		;truncate off extra bit
		ST R1,Result                         

                LD R7,flt16_exp_ra   ; restore return address
                RET
;------------------------------------------------------------------------------
flt16_get_val                        ; Result is mantissa from Param1 plus implicit 1, if Param2 (sign) is one (p), 2's comp it
        LD R0,Param1		;load number
		LD R2,VAL_MASK		;load value mask, 10 bits
		LD R3,IMPL1_MASK	;load implicit 1 mask
		AND R0,R0,R2		;Mask off extra bits,leave the bottom 10 bits
		BRz valExit			;if the result is zero, do not add in implicit one
		ADD R0,R0,R3		;add implicit one
		LD R4, Param2		;load sign 
		BRz valExit			;if sign is zero, i.e. sign bit is 0, positive, exit
		NOT R0,R0			;else: take 2's complement. ~R0 + 1
		ADD R0,R0,#1

valExit		ST R0,Result
                RET
;------------------------------------------------------------------------------
flt16_abs                            ; Result is absolute value of Param1
                LD R0,Param1		; load number
		LD R2,SIGN_MASK		;load mask to R2
		NOT R2,R2			;inverse of mask
		AND R0,R2,R0		;clear first bit
		ST R0,Result
                RET
;------------------------------------------------------------------------------
flt16_neg                            ; Result is negative value of Param1
		LD R0,Param1		; load number
		BRz neg1		;return parameter if its zero
		LD R1,SIGN_MASK		;load sign mask 8000
		ADD R0,R0,R1		;add 1 to the first bit, 0 if overflow 1+1

neg1	ST R0,Result
                RET
;------------------------------------------------------------------------------
left_most1                           ; Result is bit position of leftmost 1 in Param1
                                     ; Algorithm: check bits from right to left
		 LD R0,Param1					;
		 LD R1,SIGN_MASK	;load mask: leftmost position
		 ;LD R2,#15		;load counter to indicate position CANNOT load value directly
		 LD R2,SIXTEEN
		 ADD R2,R2,#-1
loop		AND R5,R0,R1		;Check if this bit[15] is set
		BRn leftExit		;if   bit 15 is 1, then the number is negative,break out of loop
		ADD R0,R0,R0		;else Shift param1 left one bit
		ADD R2,R2,#-1		;Decrement counter
		BRzp loop		;while counter>=0
		 
leftExit	ST R2,Result	
                RET
;------------------------------------------------------------------------------
left_shift                           ; Result is Param1 shifted left by Param2 bits
                                     ; Algorithm: shift left by doubling
				LD R1,Param1		;get parameter
				LD R2,Param2		;get the bits to shift, also is the counter for loop
				BRnz leftShiftExit	;if shift by 0 or neg bit, just return the parameter
LoopLeft			ADD R1,R1,R1		;Left shift one bit
				ADD R2,R2,#-1		;decrement counter
				BRp LoopLeft			;while(counter > 0)
				
leftShiftExit	ST R1,Result		;Result = R1
                RET
;------------------------------------------------------------------------------
right_shift                          ; Result is Param1 shifted right by Param2 bits
                                     ; Algorithm: walk source and destination bit
                LD R0,Param1		; R0 = param1
		LD R2,Param1		;prapare for zero bit shift
		LD R1,Param2		; R1 = bits to shift by
		BRnz rightExit		; if shift by 0 bit or neg bits, just return Param1
		
		AND R2,R2,0		; clear R2 to store results		
		LD R3,ONE		; R3 = Source mask
		LD R4,ONE		; R4 = Dest mask

rs_LoopR	ADD R3,R3,R3		;prepare source mask:
		ADD R1,R1,#-1		;move source mask to the last bit( - to be) after shift
		BRp rs_LoopR
		;LD R1,Param2		;restore counter
		
rs_LoopR2	AND R5,R0,R3		;Check if this bit is set
		BRz rs_LoopR3		;if this bit is not set, move on to the next bit
		ADD R2,R2,R4		;else add dext mask to result

rs_LoopR3	ADD R4,R4,R4		;Shift dest mask as well
		ADD R3,R3,R3		;shift source mask, Move onto the next bit
		BRnp rs_LoopR2		;continue until the source mask is still not zero
		
rightExit	ST R2,Result 
                RET
;------------------------------------------------------------------------------
binary_or                            ; Result is a bitwise OR of Param1 and Param2
                                     ; Algorithm: De Morgan's Law: a | b = ~(~a & ~b)
        	LD R0,Param1		;load number
		LD R1,Param2		;load number
                NOT R0,R0		;~a
		NOT R1,R1		;~b
		AND R5,R0,R1		;~a & ~b
		NOT R5,R5		;~(~a & ~b)=a | b
		ST R5,Result		
                RET
;------------------------------------------------------------------------------
                .END


