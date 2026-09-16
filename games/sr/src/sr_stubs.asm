; SR UI worker.  It owns no game object lifetime and only writes the resolved
; UI state while the child process remains alive.
option casemap:none
.code
PUBLIC SrUiBegin, SrUiEnd

SrUiBegin PROC
    push rbx
    sub rsp, 20h
    mov rbx, rcx
sr_loop:
    cmp dword ptr [rbx+16], 0
    jne sr_return
    mov rax, [rbx]
    mov dword ptr [rax], 2
    mov dword ptr [rbx+20], 1
    mov ecx, 500
    call qword ptr [rbx+8]
    jmp sr_loop
sr_return:
    xor eax, eax
    add rsp, 20h
    pop rbx
    ret
SrUiBegin ENDP
SrUiEnd LABEL BYTE
END
