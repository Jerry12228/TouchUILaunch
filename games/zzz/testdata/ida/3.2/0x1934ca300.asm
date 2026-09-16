0x1934ca300  56                              push    rsi
0x1934ca301  57                              push    rdi
0x1934ca302  4883ec38                        sub     rsp, 38h
0x1934ca306  89ce                            mov     esi, ecx
0x1934ca308  803d4d28f0f100                  cmp     cs:byte_1853CCB5C, 0
0x1934ca30f  0f84d7000000                    jz      loc_1934CA3EC
0x1934ca315  803dda35a8f100                  cmp     cs:byte_184F4D8F6, 0
0x1934ca31c  0f85e8000000                    jnz     loc_1934CA40A
0x1934ca322  488b0db770aff1                  mov     rcx, cs:qword_184FC13E0
0x1934ca329  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x1934ca330  0f84f3000000                    jz      loc_1934CA429
0x1934ca336  488b05c330a0f1                  mov     rax, cs:qword_184ECD400
0x1934ca33d  488b8068e40200                  mov     rax, [rax+2E468h]
0x1934ca344  4885c0                          test    rax, rax
0x1934ca347  0f84f8000000                    jz      loc_1934CA445
0x1934ca34d  488bb890000000                  mov     rdi, [rax+90h]
0x1934ca354  4885ff                          test    rdi, rdi
0x1934ca357  0f84ed000000                    jz      loc_1934CA44A
0x1934ca35d  4c8b057445b3f1                  mov     r8, cs:qword_184FFE8D8
0x1934ca364  488b07                          mov     rax, [rdi]
0x1934ca367  0fb788c6000000                  movzx   ecx, word ptr [rax+0C6h]
0x1934ca36e  4885c9                          test    rcx, rcx
0x1934ca371  741c                            jz      short loc_1934CA38F
0x1934ca373  488b5050                        mov     rdx, [rax+50h]
0x1934ca377  48c1e104                        shl     rcx, 4
0x1934ca37b  4531c9                          xor     r9d, r9d
0x1934ca37e  6690                            xchg    ax, ax
0x1934ca380  4e39040a                        cmp     [rdx+r9], r8
0x1934ca384  7428                            jz      short loc_1934CA3AE
0x1934ca386  4983c110                        add     r9, 10h
0x1934ca38a  4c39c9                          cmp     rcx, r9
0x1934ca38d  75f1                            jnz     short loc_1934CA380
0x1934ca38f  488d4c2428                      lea     rcx, [rsp+48h+var_20]
0x1934ca394  4889fa                          mov     rdx, rdi
0x1934ca397  41b901000000                    mov     r9d, 1
0x1934ca39d  e86e81dbec                      call    sub_180282510
0x1934ca3a2  4c8b4c2428                      mov     r9, [rsp+48h+var_20]
0x1934ca3a7  4c8b442430                      mov     r8, [rsp+48h+var_18]
0x1934ca3ac  eb2e                            jmp     short loc_1934CA3DC
0x1934ca3ae  428b4c0a08                      mov     ecx, [rdx+r9+8]
0x1934ca3b3  ffc1                            inc     ecx
0x1934ca3b5  4863c9                          movsxd  rcx, ecx
0x1934ca3b8  4c8b8cc8d0000000                mov     r9, [rax+rcx*8+0D0h]
0x1934ca3c0  4c894c2428                      mov     [rsp+48h+var_20], r9
0x1934ca3c5  0fb790c4000000                  movzx   edx, word ptr [rax+0C4h]
0x1934ca3cc  4801ca                          add     rdx, rcx
0x1934ca3cf  4c8b84d0d0000000                mov     r8, [rax+rdx*8+0D0h]
0x1934ca3d7  4c89442430                      mov     [rsp+48h+var_18], r8
0x1934ca3dc  4889f9                          mov     rcx, rdi
0x1934ca3df  89f2                            mov     edx, esi
0x1934ca3e1  41ffd1                          call    r9
0x1934ca3e4  90                              nop
0x1934ca3e5  4883c438                        add     rsp, 38h
0x1934ca3e9  5f                              pop     rdi
0x1934ca3ea  5e                              pop     rsi
0x1934ca3eb  c3                              retn
0x1934ca3ec  b91ca30300                      mov     ecx, 3A31Ch
0x1934ca3f1  e8aa23dcec                      call    sub_18028C7A0
0x1934ca3f6  c6055f27f0f101                  mov     cs:byte_1853CCB5C, 1
0x1934ca3fd  803df234a8f100                  cmp     cs:byte_184F4D8F6, 0
0x1934ca404  0f8418ffffff                    jz      loc_1934CA322
0x1934ca40a  b936290400                      mov     ecx, 42936h
0x1934ca40f  e80c1966ff                      call    sub_192B2BD20
0x1934ca414  4885c0                          test    rax, rax
0x1934ca417  7436                            jz      short loc_1934CA44F
0x1934ca419  4889c1                          mov     rcx, rax
0x1934ca41c  89f2                            mov     edx, esi
0x1934ca41e  4883c438                        add     rsp, 38h
0x1934ca422  5f                              pop     rdi
0x1934ca423  5e                              pop     rsi
0x1934ca424  e9575adafa                      jmp     sub_18E26FE80
0x1934ca429  e8c2addbec                      call    sub_1802851F0
0x1934ca42e  488b05cb2fa0f1                  mov     rax, cs:qword_184ECD400
0x1934ca435  488b8068e40200                  mov     rax, [rax+2E468h]
0x1934ca43c  4885c0                          test    rax, rax
0x1934ca43f  0f8508ffffff                    jnz     loc_1934CA34D
0x1934ca445  e816af46ed                      call    sub_180935360
0x1934ca44a  e811af46ed                      call    sub_180935360
0x1934ca44f  e80caf46ed                      call    sub_180935360
