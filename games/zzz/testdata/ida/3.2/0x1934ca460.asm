0x1934ca460  56                              push    rsi
0x1934ca461  57                              push    rdi
0x1934ca462  4883ec38                        sub     rsp, 38h
0x1934ca466  89ce                            mov     esi, ecx
0x1934ca468  803d3327f0f100                  cmp     cs:byte_1853CCBA2, 0
0x1934ca46f  0f84d7000000                    jz      loc_1934CA54C
0x1934ca475  803dc734a8f100                  cmp     cs:byte_184F4D943, 0
0x1934ca47c  0f85e8000000                    jnz     loc_1934CA56A
0x1934ca482  488b0d576faff1                  mov     rcx, cs:qword_184FC13E0
0x1934ca489  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x1934ca490  0f84f3000000                    jz      loc_1934CA589
0x1934ca496  488b05632fa0f1                  mov     rax, cs:qword_184ECD400
0x1934ca49d  488b8068e40200                  mov     rax, [rax+2E468h]
0x1934ca4a4  4885c0                          test    rax, rax
0x1934ca4a7  0f84f8000000                    jz      loc_1934CA5A5
0x1934ca4ad  488bb888000000                  mov     rdi, [rax+88h]
0x1934ca4b4  4885ff                          test    rdi, rdi
0x1934ca4b7  0f84ed000000                    jz      loc_1934CA5AA
0x1934ca4bd  4c8b051444b3f1                  mov     r8, cs:qword_184FFE8D8
0x1934ca4c4  488b07                          mov     rax, [rdi]
0x1934ca4c7  0fb788c6000000                  movzx   ecx, word ptr [rax+0C6h]
0x1934ca4ce  4885c9                          test    rcx, rcx
0x1934ca4d1  741c                            jz      short loc_1934CA4EF
0x1934ca4d3  488b5050                        mov     rdx, [rax+50h]
0x1934ca4d7  48c1e104                        shl     rcx, 4
0x1934ca4db  4531c9                          xor     r9d, r9d
0x1934ca4de  6690                            xchg    ax, ax
0x1934ca4e0  4e39040a                        cmp     [rdx+r9], r8
0x1934ca4e4  7428                            jz      short loc_1934CA50E
0x1934ca4e6  4983c110                        add     r9, 10h
0x1934ca4ea  4c39c9                          cmp     rcx, r9
0x1934ca4ed  75f1                            jnz     short loc_1934CA4E0
0x1934ca4ef  488d4c2428                      lea     rcx, [rsp+48h+var_20]
0x1934ca4f4  4889fa                          mov     rdx, rdi
0x1934ca4f7  41b901000000                    mov     r9d, 1
0x1934ca4fd  e80e80dbec                      call    sub_180282510
0x1934ca502  4c8b4c2428                      mov     r9, [rsp+48h+var_20]
0x1934ca507  4c8b442430                      mov     r8, [rsp+48h+var_18]
0x1934ca50c  eb2e                            jmp     short loc_1934CA53C
0x1934ca50e  428b4c0a08                      mov     ecx, [rdx+r9+8]
0x1934ca513  ffc1                            inc     ecx
0x1934ca515  4863c9                          movsxd  rcx, ecx
0x1934ca518  4c8b8cc8d0000000                mov     r9, [rax+rcx*8+0D0h]
0x1934ca520  4c894c2428                      mov     [rsp+48h+var_20], r9
0x1934ca525  0fb790c4000000                  movzx   edx, word ptr [rax+0C4h]
0x1934ca52c  4801ca                          add     rdx, rcx
0x1934ca52f  4c8b84d0d0000000                mov     r8, [rax+rdx*8+0D0h]
0x1934ca537  4c89442430                      mov     [rsp+48h+var_18], r8
0x1934ca53c  4889f9                          mov     rcx, rdi
0x1934ca53f  89f2                            mov     edx, esi
0x1934ca541  41ffd1                          call    r9
0x1934ca544  90                              nop
0x1934ca545  4883c438                        add     rsp, 38h
0x1934ca549  5f                              pop     rdi
0x1934ca54a  5e                              pop     rsi
0x1934ca54b  c3                              retn
0x1934ca54c  b962a30300                      mov     ecx, 3A362h
0x1934ca551  e84a22dcec                      call    sub_18028C7A0
0x1934ca556  c6054526f0f101                  mov     cs:byte_1853CCBA2, 1
0x1934ca55d  803ddf33a8f100                  cmp     cs:byte_184F4D943, 0
0x1934ca564  0f8418ffffff                    jz      loc_1934CA482
0x1934ca56a  b983290400                      mov     ecx, 42983h
0x1934ca56f  e8ac1766ff                      call    sub_192B2BD20
0x1934ca574  4885c0                          test    rax, rax
0x1934ca577  7436                            jz      short loc_1934CA5AF
0x1934ca579  4889c1                          mov     rcx, rax
0x1934ca57c  89f2                            mov     edx, esi
0x1934ca57e  4883c438                        add     rsp, 38h
0x1934ca582  5f                              pop     rdi
0x1934ca583  5e                              pop     rsi
0x1934ca584  e9f758dafa                      jmp     sub_18E26FE80
0x1934ca589  e862acdbec                      call    sub_1802851F0
0x1934ca58e  488b056b2ea0f1                  mov     rax, cs:qword_184ECD400
0x1934ca595  488b8068e40200                  mov     rax, [rax+2E468h]
0x1934ca59c  4885c0                          test    rax, rax
0x1934ca59f  0f8508ffffff                    jnz     loc_1934CA4AD
0x1934ca5a5  e8b6ad46ed                      call    sub_180935360
0x1934ca5aa  e8b1ad46ed                      call    sub_180935360
0x1934ca5af  e8acad46ed                      call    sub_180935360
