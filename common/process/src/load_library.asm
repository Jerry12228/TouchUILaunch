; Generic remote LoadLibraryW helper.  The full 64-bit HMODULE is written back
; to the caller-owned context; it is never passed through a thread exit code.
option casemap:none
.code
PUBLIC UiLoadBegin, UiLoadEnd

UiLoadBegin PROC
    push rbx
    sub rsp, 20h
    mov rbx, rcx
    mov rcx, [rbx]
    call qword ptr [rbx+8]
    mov [rbx+16], rax
    xor eax, eax
    add rsp, 20h
    pop rbx
    ret
UiLoadBegin ENDP
UiLoadEnd LABEL BYTE
END
