; Generic remote LoadLibraryW helper.  The full 64-bit HMODULE is written back
; to the caller-owned context; it is never passed through a thread exit code.
option casemap:none
.code
PUBLIC UiLoadBegin, UiLoadEnd
PUBLIC UiDiagnosticLoadBegin, UiDiagnosticLoadEnd

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

; Diagnostic loader used for an owned, normally started child. Capture the
; error on this same thread, immediately after a failed LoadLibraryW call.
; Context offsets are asserted in load_library_diagnostic.hpp.
UiDiagnosticLoadBegin PROC
    push rbx
    sub rsp, 20h
    mov rbx, rcx
    mov dword ptr [rbx+36], 1
    mov rcx, [rbx]
    call qword ptr [rbx+8]
    mov [rbx+24], rax
    test rax, rax
    jnz diagnostic_done
    call qword ptr [rbx+16]
    mov [rbx+32], eax
diagnostic_done:
    mov dword ptr [rbx+36], 2
    xor eax, eax
    add rsp, 20h
    pop rbx
    ret
UiDiagnosticLoadBegin ENDP
UiDiagnosticLoadEnd LABEL BYTE
END
