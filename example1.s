
.section .init
.org 0x000
.global _super_start
.global _tos_syscall
.global main
.global __uClibc_main
.global _end
_super_start:
    # %rdi is argc from loader
    # %rsi is argv from loader
    jmp     uclibc_main_launch

.org 0x010
syscall_address:
    .quad   0
fini_address:
    .quad   fini
end_address:
    .quad   _end
init:
    retq

.section .fini
fini:
    retq

.text
_tos_syscall:
    mov     %rdx, %rcx      # syscall arg 3 ->  Linux arg 4
    mov     %rsi, %rdx      # syscall arg 2 ->  Linux arg 3
    mov     %rdi, %rsi      # syscall arg 1 ->  Linux arg 2
    mov     %rax, %rdi      # syscall number -> Linux arg 1
    jmpq    *syscall_address(%rip)

uclibc_main_launch:
    // patch entry point to cause a crash if reached again
    mov     halt_code(%rip), %rax
    mov     %rax, _super_start(%rip)
    // launch uclibc start
    # xor     %rbp, %rbp              # zero ebp
    # andq    $~15, %rsp              # align stack
    mov     %rsi, %rdx              # arg 3 (argv) from loader
    mov     %rdi, %rsi              # arg 2 (argc) from loader
    lea     main(%rip), %rdi        # arg 1 location of main
    lea     init(%rip), %rcx       # arg 4 app_init
    lea     fini(%rip), %r8        # arg 5 app_fini
    mov     $0, %r9                 # arg 6 rtld_fini
    pushq   %r9                     # arg 7 stack_end
    call    __uClibc_main

    // should not return
halt_code:
    hlt
    hlt
    hlt
    hlt
    hlt
    hlt
    hlt

