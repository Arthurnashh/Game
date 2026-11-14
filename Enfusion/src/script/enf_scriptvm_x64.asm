; Generic native-call bridge for the Windows x64 ABI.
; EnfWin64NativeCall layout: Function=0, Count=8, Values=16,
; FloatFlags=24, IntegerResult=32, FloatResult=40.
.code
EnfCallWin64 PROC FRAME
    push rbx
    .pushreg rbx
    push rsi
    .pushreg rsi
    push rdi
    .pushreg rdi
    push rbp
    .pushreg rbp
    sub rsp, 208h
    .allocstack 208h
    .endprolog

    mov rbx, rcx
    mov rsi, [rbx+16]
    mov rdi, [rbx+24]
    xor ebp, ebp

arg_loop:
    cmp ebp, [rbx+8]
    jae args_ready
    mov rax, [rsi+rbp*8]
    cmp ebp, 4
    jae stack_arg
    cmp ebp, 0
    je arg0
    cmp ebp, 1
    je arg1
    cmp ebp, 2
    je arg2
    jmp arg3

arg0:
    cmp byte ptr [rdi+rbp], 0
    jne float0
    mov rcx, rax
    jmp next_arg
float0:
    movd xmm0, eax
    jmp next_arg
arg1:
    cmp byte ptr [rdi+rbp], 0
    jne float1
    mov rdx, rax
    jmp next_arg
float1:
    movd xmm1, eax
    jmp next_arg
arg2:
    cmp byte ptr [rdi+rbp], 0
    jne float2
    mov r8, rax
    jmp next_arg
float2:
    movd xmm2, eax
    jmp next_arg
arg3:
    cmp byte ptr [rdi+rbp], 0
    jne float3
    mov r9, rax
    jmp next_arg
float3:
    movd xmm3, eax
    jmp next_arg

stack_arg:
    mov r10d, ebp
    sub r10d, 4
    mov [rsp+20h+r10*8], rax
next_arg:
    inc ebp
    jmp arg_loop

args_ready:
    mov rax, [rbx]
    call rax
    mov [rbx+32], rax
    movss dword ptr [rbx+40], xmm0

    add rsp, 208h
    pop rbp
    pop rdi
    pop rsi
    pop rbx
    ret
EnfCallWin64 ENDP
END
