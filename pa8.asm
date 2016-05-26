;; File:        pa8.asm
;; Description: Solution assembly code for PA8, cs270
;; Author:      Chris Wilcox
;; Date:        11/4/2014

           .ORIG   x3000
            BR     MAIN              ; Branch to main 

;; Data declarations
PARAM      .FILL   x1234             ; Input parameter

;; Program entry point
MAIN        LD    R6,STACK           ; Initialize stack pointer
            LD    R0,PARAM           ; Load parameter
            PUSH  R0                 ; Push parameter
            JSR   PRINT              ; Call print
            ADD   R6,R6,#1           ; Cleanup stack
            HALT                     ; Program complete

;; Print a positive decimal number
PRINT       ;; stack entry
            PUSH  R7                 ; Push return address
            PUSH  R5                 ; Push frame pointer
            ADD   R5,R6,#0           ; Setup frame pointer

            ;; function body
BODY        LDR   R0,R5,#2           ; Load parameter
            ADD   R1,R0,#-10         ; Subtract ten
            BRn   NORECURSE          ; Skip recursion

            ;; divide by 10
RECURSE      PUSH R0                 ; Push numerator
             LD R1,TEN               ; Load 10
             PUSH R1                 ; Push 10
             JSR DIVIDE              ; Call divide
             POP R0                  ; Load result
             ADD R6,R6,#2            ; Cleanup stack

            ;; recursive call
             PUSH R0                 ; Push result
             JSR PRINT               ; Recursive call
             ADD R6,R6,#1            ; Cleanup stack
           
            ;; print digit
NORECURSE   LDR   R0,R5,#2           ; Reload parameter

            ; Modulo 10
            LD    R1,TEN             ; Load 10
            NOT   R1,R1              ; 2's complement
            ADD   R1,R1,#1           ;
LOOP0       ADD   R0,R0,R1           ; Subtract 10
            BRzp  LOOP0
            LD    R1,TEN             ; Load 10
            ADD   R0,R0,R1           ; Add 10

            PUSH R0                  ; Push digit
            JSR OUTPUT               ; Call output
            ADD R6,R6,#1             ; Cleanup stack

            ;; stack exit
DONE        POP   R5                 ; Pop frame pointer
            POP   R7                 ; Pop return address
            RET                      ; Return

;; Output a single hexadecimal digit
OUTPUT      ;; stack entry
            PUSH R7                  ; Push return address
            PUSH R5                  ; Push frame pointer
            ADD R5,R6,#0             ; Setup frame pointer

            ;; function body
            LDR   R1,R5,#2           ; Load parameter
            LD    R0,DIGIT           ; Load offset
            ADD   R0,R1,R0           ; Digit conversion
            TRAP  x21                ; Output digit

CONTINUE    ;; stack exit
            POP R5                   ; Pop frame pointer
            POP R7                   ; Pop return address
            RET                      ; Return

;; Divide function
DIVIDE      ;; stack entry
            ADD   R6,R6,#-1          ; Allocate return value
            PUSH  R7                 ; Push return address
            PUSH  R5                 ; Push frame pointer
            ADD   R5,R6,#0           ; Setup frame pointer

            ;; function body
            LDR   R0,R5,#4           ; Load numerator
            LDR   R1,R5,#3           ; Load denominator
            NOT   R1,R1              ; 2's complement (negate)
            ADD   R1,R1,#1           ; 2's complement (increment)
            AND   R2,R2,#0           ; Initialize result
LOOP1       ADD   R0,R0,R1           ; Subtract denominator
            BRn   EXIT               ; Loop complete?
            ADD   R2,R2,#1           ; Increment result
            BR    LOOP1              ; Continue processing

EXIT        ;; stack exit
            STR   R2,R5,#2           ; Store return value
            POP   R5                 ; Pop frame pointer
            POP   R7                 ; Pop return address
            RET                      ; Return

;; Constants
TEN       .FILL   x000A
DIGIT     .FILL   x0030
STACK     .FILL   x4000

          .END

