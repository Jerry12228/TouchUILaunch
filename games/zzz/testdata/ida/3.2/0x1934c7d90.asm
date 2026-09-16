0x1934c7d90  56                              push    rsi
0x1934c7d91  4883ec30                        sub     rsp, 30h
0x1934c7d95  803da74df0f100                  cmp     cs:byte_1853CCB43, 0
0x1934c7d9c  0f84bf000000                    jz      loc_1934C7E61
0x1934c7da2  488b0d3796aff1                  mov     rcx, cs:qword_184FC13E0
0x1934c7da9  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x1934c7db0  0f84d0000000                    jz      loc_1934C7E86
0x1934c7db6  488b054356a0f1                  mov     rax, cs:qword_184ECD400
0x1934c7dbd  488b8068e40200                  mov     rax, [rax+2E468h]
0x1934c7dc4  4885c0                          test    rax, rax
0x1934c7dc7  0f84d5000000                    jz      loc_1934C7EA2
0x1934c7dcd  488bb098000000                  mov     rsi, [rax+98h]
0x1934c7dd4  4885f6                          test    rsi, rsi
0x1934c7dd7  0f84ca000000                    jz      loc_1934C7EA7
0x1934c7ddd  4c8b05f46ab3f1                  mov     r8, cs:qword_184FFE8D8
0x1934c7de4  488b06                          mov     rax, [rsi]
0x1934c7de7  0fb788c6000000                  movzx   ecx, word ptr [rax+0C6h]
0x1934c7dee  4885c9                          test    rcx, rcx
0x1934c7df1  741c                            jz      short loc_1934C7E0F
0x1934c7df3  488b5050                        mov     rdx, [rax+50h]
0x1934c7df7  48c1e104                        shl     rcx, 4
0x1934c7dfb  4531c9                          xor     r9d, r9d
0x1934c7dfe  6690                            xchg    ax, ax
0x1934c7e00  4e39040a                        cmp     [rdx+r9], r8
0x1934c7e04  7425                            jz      short loc_1934C7E2B
0x1934c7e06  4983c110                        add     r9, 10h
0x1934c7e0a  4c39c9                          cmp     rcx, r9
0x1934c7e0d  75f1                            jnz     short loc_1934C7E00
0x1934c7e0f  488d4c2420                      lea     rcx, [rsp+38h+var_18]
0x1934c7e14  4889f2                          mov     rdx, rsi
0x1934c7e17  4531c9                          xor     r9d, r9d
0x1934c7e1a  e8f1a6dbec                      call    sub_180282510
0x1934c7e1f  4c8b442420                      mov     r8, [rsp+38h+var_18]
0x1934c7e24  488b542428                      mov     rdx, [rsp+38h+var_10]
0x1934c7e29  eb29                            jmp     short loc_1934C7E54
0x1934c7e2b  4a634c0a08                      movsxd  rcx, dword ptr [rdx+r9+8]
0x1934c7e30  4c8b84c8d0000000                mov     r8, [rax+rcx*8+0D0h]
0x1934c7e38  4c89442420                      mov     [rsp+38h+var_18], r8
0x1934c7e3d  0fb790c4000000                  movzx   edx, word ptr [rax+0C4h]
0x1934c7e44  4801ca                          add     rdx, rcx
0x1934c7e47  488b94d0d0000000                mov     rdx, [rax+rdx*8+0D0h]
0x1934c7e4f  4889542428                      mov     [rsp+38h+var_10], rdx
0x1934c7e54  4889f1                          mov     rcx, rsi
0x1934c7e57  41ffd0                          call    r8
0x1934c7e5a  90                              nop
0x1934c7e5b  4883c430                        add     rsp, 30h
0x1934c7e5f  5e                              pop     rsi
0x1934c7e60  c3                              retn
0x1934c7e61  b903a30300                      mov     ecx, 3A303h
0x1934c7e66  e83549dcec                      call    sub_18028C7A0
0x1934c7e6b  c605d14cf0f101                  mov     cs:byte_1853CCB43, 1
0x1934c7e72  488b0d6795aff1                  mov     rcx, cs:qword_184FC13E0
0x1934c7e79  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x1934c7e80  0f8530ffffff                    jnz     loc_1934C7DB6
0x1934c7e86  e865d3dbec                      call    sub_1802851F0
0x1934c7e8b  488b056e55a0f1                  mov     rax, cs:qword_184ECD400
0x1934c7e92  488b8068e40200                  mov     rax, [rax+2E468h]
0x1934c7e99  4885c0                          test    rax, rax
0x1934c7e9c  0f852bffffff                    jnz     loc_1934C7DCD
0x1934c7ea2  e8b9d446ed                      call    sub_180935360
0x1934c7ea7  e8b4d446ed                      call    sub_180935360
