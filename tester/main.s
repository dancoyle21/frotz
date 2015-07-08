

magic = 0x123456789abcdef
count = 1000000000

.text
.global main
.global puts
.global exit
main:
    lea     start_message(%rip), %rdi
    call    puts

    mov     $count, %rcx

    mov     $magic, %rax
    mov     %rax, %rbx
    mov     %rax, %rdx
    mov     %rax, %rsi
    mov     %rax, %rdi
    mov     %rax, %rbp
    mov     %rax, %r8
    mov     %rax, %r9
    mov     %rax, %r10
    mov     %rax, %r11
    mov     %rax, %r12
    mov     %rax, %r13
    mov     %rax, %r14
    mov     %rax, %r15

check_loop:
    cmp     %rax, %r15
    jnz     failure
    cmp     %rax, %r14
    jnz     failure
    cmp     %rax, %r13
    jnz     failure
    cmp     %rax, %r12
    jnz     failure
    cmp     %rax, %r11
    jnz     failure
    cmp     %rax, %r10
    jnz     failure
    cmp     %rax, %r9
    jnz     failure
    cmp     %rax, %r8
    jnz     failure
    cmp     %rax, %rbp
    jnz     failure
    cmp     %rax, %rdi
    jnz     failure
    cmp     %rax, %rsi
    jnz     failure
    cmp     %rax, %rdx
    jnz     failure
    cmp     %rax, %rbx
    jnz     failure

    dec     %rcx
    jnz     check_loop
    
    mov     $magic, %rbx
    cmp     %rax, %rbx
    jnz     failure

    lea     ok_message(%rip), %rdi
    call    puts
    mov     $0, %rdi
    call    exit

failure:
    lea     error_message(%rip), %rdi
    call    puts
    mov     $1, %rdi
    call    exit

.data
start_message:
    .ascii  "Startup message\n\n\0"
ok_message:
    .ascii  "OK\n\n\0"
error_message:
    .ascii  "FAILURE\n\n\0"

