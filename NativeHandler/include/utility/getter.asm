SHELL_PTR_ELEMS EQU 5

SHELL_ENDCODE_MAGIC EQU 02BAD4B0BBAADBABEh

PTR_SIZE EQU SIZEOF QWORD

ARG_FPUSAVE_SUPPORT     EQU 0 * PTR_SIZE
ARG_FPUSAVE_BUFFER      EQU 1 * PTR_SIZE
ARG_CALLHOOK            EQU 2 * PTR_SIZE
ARG_CALLORIG            EQU 3 * PTR_SIZE
ARG_SAVED_RSP           EQU 4 * PTR_SIZE

FPU_STATE_SAVING EQU 0
FPU_STATE_RESTORING EQU 1

save_cpu_state_gpr MACRO
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
    pushfq
ENDM


restore_cpu_state_gpr MACRO
    popfq
	pop r15
	pop r14
	pop r13
	pop r12
	pop r11
	pop r10
	pop r9
	pop r8
	pop rbp
	pop rdi
	pop rsi
	pop rdx
	pop rcx
	pop rbx
	pop rax
ENDM

COMMENT @ FXSAVE
https://www.intel.com/content/dam/www/public/us/en/documents/manuals/64-ia-32-architectures-software-developer-vol-1-manual.pdf#G11.53889
@
save_fpu_state_fxsave MACRO
    push r15
    mov r15, qword ptr [args + ARG_FPUSAVE_BUFFER]
    fxsave64 qword ptr [r15]
    pop r15
ENDM

restore_fpu_state_fxsave MACRO
	push r15
	mov r15, qword ptr [args + ARG_FPUSAVE_BUFFER]
	fxrstor64 qword ptr [r15]
	pop r15
ENDM

COMMENT @ XSAVE
https://www.intel.com/content/dam/www/public/us/en/documents/manuals/64-ia-32-architectures-software-developer-vol-1-manual.pdf#G14.51762
@
start_fpu_state_xsave MACRO
    push rax
    push rdx
    mov rax, -1
    mov rdx, -1

    push r15
    mov r15, qword ptr [args + ARG_FPUSAVE_BUFFER] 
ENDM
end_fpu_state_xsave MACRO
	pop r15
	pop rdx
	pop rax
ENDM

save_fpu_state_xsave MACRO
    start_fpu_state_xsave
    xsave64 qword ptr [r15]
    end_fpu_state_xsave
ENDM

restore_fpu_state_xsave MACRO
	start_fpu_state_xsave
	xrstor64 qword ptr [r15]
	end_fpu_state_xsave
ENDM

preserve_fpu_state MACRO restoring
    Local fpu_preserve_fxsave, fpu_preserve_xsave, fpu_preserve_end

    pushfq

    cmp qword ptr [args + ARG_FPUSAVE_SUPPORT], 1

    jl fpu_preserve_end
    je fpu_preserve_fxsave
    jg fpu_preserve_xsave
    
fpu_preserve_fxsave:
    IF restoring EQ 1
		restore_fpu_state_fxsave
	ELSE
		save_fpu_state_fxsave
    ENDIF

	jmp fpu_preserve_end

fpu_preserve_xsave:
    IF restoring EQ 1
        restore_fpu_state_xsave
	ELSE
        save_fpu_state_xsave
	ENDIF

fpu_preserve_end:
    popfq
ENDM


.CODE

jhook_get_r14_address PROC
    mov rax, r14
    ret
jhook_get_r14_address ENDP

jhook_get_r13_address PROC
    mov rax, r13
    ret
jhook_get_r13_address ENDP

jhook_get_rbp_address PROC
    mov rax,rbp
    ret
jhook_get_rbp_address ENDP


jhook_set_r13_address PROC
    mov r13, rcx
    mov [rbp-40h],rcx
    ret
jhook_set_r13_address ENDP

jhook_end_shellcode_magic PROC
    mov rax, SHELL_ENDCODE_MAGIC
    ret
jhook_end_shellcode_magic ENDP

; This is a simple wrapper function to access the constant SHELL_PTR_ELEMS.
jhook_shellcode_numelems PROC
	mov eax, SHELL_PTR_ELEMS
	ret
jhook_shellcode_numelems ENDP

; This is a simple wrapper to get the base function address for the first instruction of the shellcode.
jhook_shellcode_getcode PROC
	mov rax, jhook_shellcode_stub
    lea rax, [rax + SHELL_PTR_ELEMS * PTR_SIZE]
	ret
jhook_shellcode_getcode ENDP

; Naked function, so no prologue or epilogue generated by the compiler
; NOTE: Do not remove the ALIGN directive
ALIGN 16
jhook_shellcode_stub PROC
    ; Dynamic array of values used by the shellcode.
	args QWORD SHELL_PTR_ELEMS DUP(0)

    ; Save FPU state
    preserve_fpu_state FPU_STATE_SAVING

    ; Save all general purpose registers/flags
    save_cpu_state_gpr

    ; Push the 'skip_original_call' argument for the callback function (default to 0)
    ; push rax
    ; mov qword ptr [rsp], 0

    ; Set context pointer with original registers/flags as the first argument to the callback
    mov rcx, rsp

    ; Save (possibly unaligned) stack pointer
    mov qword ptr [args + ARG_SAVED_RSP], rsp

    ; Allocate shadow space for spilled registers
    ; https://github.com/simon-whitehead/assembly-fun/blob/master/windows-x64/README.md#shadow-space
    sub rsp, 28h

    ; Make sure the stack is 16-byte aligned
    and rsp, -16

    ; Invoke callback function
    ; Note that the preserved registers are passed as a context argument and may be freely modified
    ; Any modified registers will be reflected after restoring the CPU state below
    call qword ptr [args + ARG_CALLHOOK]

    ; Restore saved (possibly unaligned) stack pointer
    mov rsp, qword ptr [args + ARG_SAVED_RSP]
    
    ; Restore all general purpose registers/flags
    restore_cpu_state_gpr

    ; Adjust for the additional skip_original_call argument
    add rsp, PTR_SIZE

    ; Restore FPU state
    preserve_fpu_state FPU_STATE_RESTORING

    cmp qword ptr [rsp - PTR_SIZE], 1
    je skip_original_call

    jmp qword ptr [args + ARG_CALLORIG]
    
    
skip_original_call:
    ret

end_shellcode:
    qword SHELL_ENDCODE_MAGIC

jhook_shellcode_stub ENDP


END


