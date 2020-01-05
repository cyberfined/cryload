.extern main
.globl _start
.text
_start:
    movq (%rsp), %rdi
    movq %rsp, %rsi
    addq $8, %rsi
    call main
