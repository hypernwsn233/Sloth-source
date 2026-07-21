# ============================================================
# Gerado pelo compilador Unbit
# Alvo: x86-64 Linux (ELF, System V AMD64)
# ============================================================

    .data
flag: .byte 1
outro: .byte 0
__fmt_int_print: .asciz "%d\n"
__bool_true: .asciz "true"
__bool_false: .asciz "false"

    .extern printf
    .extern puts
    .extern scanf

    .text
.globl main
.type main, @function
main:
    pushq %rbp
    movq %rsp, %rbp
    movzbl flag(%rip), %eax
    leaq __bool_false(%rip), %r10
    leaq __bool_true(%rip), %r11
    testl %eax, %eax
    cmovne %r11, %r10
    movq %r10, %rdi
    call puts
    movzbl outro(%rip), %eax
    leaq __bool_false(%rip), %r10
    leaq __bool_true(%rip), %r11
    testl %eax, %eax
    cmovne %r11, %r10
    movq %r10, %rdi
    call puts
    movl $0, %eax
    popq %rbp
    ret
    .section .note.GNU-stack,"",@progbits
