.CODE

jhook_get_r14_address PROC
    mov rax, r14
    ret
jhook_get_r14_address ENDP

jhook_get_r13_address PROC
    mov rax, r13
    ret
jhook_get_r13_address ENDP

jhook_set_r13_address PROC
    mov r13, rcx
    mov [rbp-38h],rcx
    ret
jhook_set_r13_address ENDP

END
