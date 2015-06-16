
.global go_shim
# Calling conventions for args
#  Windows:  rcx  rdx  r8   r9             
#  Linux:    rdi  rsi  rdx  rcx  r8  r9    callee-save: rbp, rbx, r12-r15
#
go_shim:
    # args: p1, argc, argv

    mov     $0, %rdi
    mov     %edx, %edi      # Win arg 2 -> argc -> Linux arg 1
    mov     %r8, %rsi       # Win arg 3 -> argv -> Linux arg 2
    jmpq    *%rcx           # Win arg 1 -> entry point

.global syscall_shim
syscall_shim:
    mov     %rdx, %r8       # Linux arg 3 -> Win arg 3
    mov     %rcx, %r9       # Linux arg 4 -> Win arg 4
    mov     %rdi, %rcx      # Linux arg 1 -> Win arg 1
    mov     %rsi, %rdx      # Linux arg 2 -> Win arg 2
    pushq   %rbp
    pushq   %rbx
    pushq   %r12
    pushq   %r13
    pushq   %r14
    pushq   %r15
    callq   syscall_handler
    popq    %r15
    popq    %r14
    popq    %r13
    popq    %r12
    popq    %rbx
    popq    %rbp
    retq

