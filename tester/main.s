

magic = 0x123456789abcdef
count = 2000000000

.text
.global main
.global puts
.global exit
main:
    lea     start_message(%rip), %rdi
    call    puts

    mov     $count, %rbx

carry_on:
    mov     $magic, %rax
    mov     %rax, %rcx
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
    jnz     failure_r15
    cmp     %rax, %r14
    jnz     failure_r14
    cmp     %rax, %r13
    jnz     failure_r13
    cmp     %rax, %r12
    jnz     failure_r12
    cmp     %rax, %r11
    jnz     failure_r11
    cmp     %rax, %r10
    jnz     failure_r10
    cmp     %rax, %r9
    jnz     failure_r9
    cmp     %rax, %r8
    jnz     failure_r8
    cmp     %rax, %rbp
    jnz     failure_rbp
    cmp     %rax, %rdi
    jnz     failure_rdi
    cmp     %rax, %rsi
    jnz     failure_rsi
    cmp     %rax, %rdx
    jnz     failure_rdx
    cmp     %rax, %rcx
    jnz     failure_rcx

    dec     %rbx
    jnz     check_loop
    
    mov     $magic, %rcx
    cmp     %rax, %rcx
    jnz     failure_rax

stop_now:
    lea     ok_message(%rip), %rdi
    call    puts
    mov     $0, %rdi
    call    exit

.text
failure_r15:
    lea     failure_r15_text(%rip), %rdi
    call    puts
    jmp     carry_on
.data
failure_r15_text:
    .ascii "failure r15\n\0"

.text
failure_r14:
    lea     failure_r14_text(%rip), %rdi
    call    puts
    jmp     carry_on
.data
failure_r14_text:
    .ascii "failure r14\n\0"

.text
failure_r13:
    lea     failure_r13_text(%rip), %rdi
    call    puts
    jmp     carry_on
.data
failure_r13_text:
    .ascii "failure r13\n\0"

.text
failure_r12:
    lea     failure_r12_text(%rip), %rdi
    call    puts
    jmp     carry_on
.data
failure_r12_text:
    .ascii "failure r12\n\0"

.text
failure_r11:
    lea     failure_r11_text(%rip), %rdi
    call    puts
    jmp     carry_on
.data
failure_r11_text:
    .ascii "failure r11\n\0"

.text
failure_r10:
    lea     failure_r10_text(%rip), %rdi
    call    puts
    jmp     carry_on
.data
failure_r10_text:
    .ascii "failure r10\n\0"

.text
failure_r9:
    lea     failure_r9_text(%rip), %rdi
    call    puts
    jmp     carry_on
.data
failure_r9_text:
    .ascii "failure r9_\n\0"

.text
failure_r8:
    lea     failure_r8_text(%rip), %rdi
    call    puts
    jmp     carry_on
.data
failure_r8_text:
    .ascii "failure r8_\n\0"

.text
failure_rbp:
    lea     failure_rbp_text(%rip), %rdi
    call    puts
    jmp     carry_on
.data
failure_rbp_text:
    .ascii "failure rbp\n\0"

.text
failure_rdi:
    lea     failure_rdi_text(%rip), %rdi
    call    puts
    jmp     carry_on
.data
failure_rdi_text:
    .ascii "failure rdi\n\0"

.text
failure_rsi:
    lea     failure_rsi_text(%rip), %rdi
    call    puts
    jmp     carry_on
.data
failure_rsi_text:
    .ascii "failure rsi\n\0"

.text
failure_rdx:
    lea     failure_rdx_text(%rip), %rdi
    call    puts
    jmp     carry_on
.data
failure_rdx_text:
    .ascii "failure rdx\n\0"

.text
failure_rcx:
    lea     failure_rcx_text(%rip), %rdi
    call    puts
    jmp     carry_on
.data
failure_rcx_text:
    .ascii "failure rcx\n\0"

.text
failure_rax:
    lea     failure_rax_text(%rip), %rdi
    call    puts
    jmp     stop_now
.data
failure_rax_text:
    .ascii "failure rax\n\0"

start_message:
    .ascii  "Startup message\n\n\0"
ok_message:
    .ascii  "exit\n\n\0"

