; UI-only adaptation of NullName's MIT-licensed GI/SR UI routines.
; Position-independent blocks copied by mobile_runtime.hpp. All external calls
; use addresses in the context; no FPS, verification or power-save code.
option casemap:none
.code
PUBLIC GiUiBegin, GiUiContext, GiUiEnd, SrUiBegin, SrUiEnd, UiLoadBegin, UiLoadEnd

GiUiBegin PROC
    push rbx
    sub rsp, 0A0h
    mov [rsp+20h], rcx
    mov [rsp+28h], rdx
    mov [rsp+30h], r8
    mov [rsp+38h], r9
    movdqu [rsp+40h], xmm0
    movdqu [rsp+50h], xmm1
    movdqu [rsp+60h], xmm2
    movdqu [rsp+70h], xmm3
    mov rbx, qword ptr [GiUiContext]
    mov dword ptr [rsp+88h], 0
    xor eax, eax
    mov edx, 1
    lock cmpxchg dword ptr [rbx+80], edx
    jne gi_wait
    mov dword ptr [rsp+88h], 1
    ; Launcher keeps this small target range writable until this restoration.
    mov rax, [rbx]
    movdqu xmm0, [rbx+40]
    movdqu [rax], xmm0
    mov rcx, -1
    mov rdx, rax
    mov r8d, 16
    call qword ptr [rbx+64] ; FlushInstructionCache
    test eax, eax
    jnz gi_protect
    or dword ptr [rbx+84], 8
gi_protect:
    mov rcx, [rbx]
    mov edx, 16
    mov r8d, [rbx+72]
    lea r9, [rbx+76]
    call qword ptr [rbx+56] ; VirtualProtect
    test eax, eax
    jnz gi_restored
    or dword ptr [rbx+84], 4
gi_restored:
    mov dword ptr [rbx+80], 2
gi_wait:
    cmp dword ptr [rbx+80], 2
    je gi_original
    pause
    jmp gi_wait
gi_original:
    mov rcx, [rsp+20h]
    mov rdx, [rsp+28h]
    mov r8, [rsp+30h]
    mov r9, [rsp+38h]
    movdqu xmm0, [rsp+40h]
    movdqu xmm1, [rsp+50h]
    movdqu xmm2, [rsp+60h]
    movdqu xmm3, [rsp+70h]
    call qword ptr [rbx]
    mov [rsp+80h], rax
    movdqu [rsp+90h], xmm0
    cmp dword ptr [rsp+88h], 0
    je gi_return
    mov rax, [rbx+24]
    mov rax, [rax]
    test rax, rax
    jz gi_missing
    mov edx, [rbx+32]
    mov rcx, [rax+rdx]
    test rcx, rcx
    jz gi_missing
    mov edx, [rbx+36]
    mov rax, [rax+rdx]
    test rax, rax
    jz gi_missing
    mov [rsp+20h], rax
    xor edx, edx
    mov r8d, 1
    call qword ptr [rbx+8]  ; UI setter(object, 0, true)
    mov rcx, [rsp+20h]
    xor edx, edx
    xor r8d, r8d
    call qword ptr [rbx+16] ; Input setter(object, 0, null)
    or dword ptr [rbx+84], 1
    jmp gi_return
gi_missing:
    or dword ptr [rbx+84], 2
gi_return:
    mov rax, [rsp+80h]
    movdqu xmm0, [rsp+90h]
    add rsp, 0A0h
    pop rbx
    ret
GiUiContext DQ 0
GiUiBegin ENDP
GiUiEnd LABEL BYTE

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
    call qword ptr [rbx+8] ; Sleep, independent of FPS
    jmp sr_loop
sr_return:
    xor eax, eax
    add rsp, 20h
    pop rbx
    ret
SrUiBegin ENDP
SrUiEnd LABEL BYTE

UiLoadBegin PROC
    push rbx
    sub rsp, 20h
    mov rbx, rcx
    mov rcx, [rbx]
    call qword ptr [rbx+8] ; LoadLibraryW
    mov [rbx+16], rax      ; Full 64-bit HMODULE, not a thread exit code
    xor eax, eax
    add rsp, 20h
    pop rbx
    ret
UiLoadBegin ENDP
UiLoadEnd LABEL BYTE
END
