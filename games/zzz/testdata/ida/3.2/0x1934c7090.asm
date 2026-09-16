0x1934c7090  56                              push    rsi
0x1934c7091  57                              push    rdi
0x1934c7092  4883ec38                        sub     rsp, 38h
0x1934c7096  89ce                            mov     esi, ecx
0x1934c7098  803d9c5af0f100                  cmp     cs:byte_1853CCB3B, 0
0x1934c709f  0f84d7000000                    jz      loc_1934C717C
0x1934c70a5  803d2868a8f100                  cmp     cs:byte_184F4D8D4, 0
0x1934c70ac  0f85e8000000                    jnz     loc_1934C719A
0x1934c70b2  488b0d27a3aff1                  mov     rcx, cs:qword_184FC13E0
0x1934c70b9  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x1934c70c0  0f84f3000000                    jz      loc_1934C71B9
0x1934c70c6  488b053363a0f1                  mov     rax, cs:qword_184ECD400
0x1934c70cd  488b8068e40200                  mov     rax, [rax+2E468h]
0x1934c70d4  4885c0                          test    rax, rax
0x1934c70d7  0f84f8000000                    jz      loc_1934C71D5
0x1934c70dd  488bb898000000                  mov     rdi, [rax+98h]
0x1934c70e4  4885ff                          test    rdi, rdi
0x1934c70e7  0f84ed000000                    jz      loc_1934C71DA
0x1934c70ed  4c8b05e477b3f1                  mov     r8, cs:qword_184FFE8D8
0x1934c70f4  488b07                          mov     rax, [rdi]
0x1934c70f7  0fb788c6000000                  movzx   ecx, word ptr [rax+0C6h]
0x1934c70fe  4885c9                          test    rcx, rcx
0x1934c7101  741c                            jz      short loc_1934C711F
0x1934c7103  488b5050                        mov     rdx, [rax+50h]
0x1934c7107  48c1e104                        shl     rcx, 4
0x1934c710b  4531c9                          xor     r9d, r9d
0x1934c710e  6690                            xchg    ax, ax
0x1934c7110  4e39040a                        cmp     [rdx+r9], r8
0x1934c7114  7428                            jz      short loc_1934C713E
0x1934c7116  4983c110                        add     r9, 10h
0x1934c711a  4c39c9                          cmp     rcx, r9
0x1934c711d  75f1                            jnz     short loc_1934C7110
0x1934c711f  488d4c2428                      lea     rcx, [rsp+48h+var_20]
0x1934c7124  4889fa                          mov     rdx, rdi
0x1934c7127  41b901000000                    mov     r9d, 1
0x1934c712d  e8deb3dbec                      call    sub_180282510
0x1934c7132  4c8b4c2428                      mov     r9, [rsp+48h+var_20]
0x1934c7137  4c8b442430                      mov     r8, [rsp+48h+var_18]
0x1934c713c  eb2e                            jmp     short loc_1934C716C
0x1934c713e  428b4c0a08                      mov     ecx, [rdx+r9+8]
0x1934c7143  ffc1                            inc     ecx
0x1934c7145  4863c9                          movsxd  rcx, ecx
0x1934c7148  4c8b8cc8d0000000                mov     r9, [rax+rcx*8+0D0h]
0x1934c7150  4c894c2428                      mov     [rsp+48h+var_20], r9
0x1934c7155  0fb790c4000000                  movzx   edx, word ptr [rax+0C4h]
0x1934c715c  4801ca                          add     rdx, rcx
0x1934c715f  4c8b84d0d0000000                mov     r8, [rax+rdx*8+0D0h]
0x1934c7167  4c89442430                      mov     [rsp+48h+var_18], r8
0x1934c716c  4889f9                          mov     rcx, rdi
0x1934c716f  89f2                            mov     edx, esi
0x1934c7171  41ffd1                          call    r9
0x1934c7174  90                              nop
0x1934c7175  4883c438                        add     rsp, 38h
0x1934c7179  5f                              pop     rdi
0x1934c717a  5e                              pop     rsi
0x1934c717b  c3                              retn
0x1934c717c  b9fba20300                      mov     ecx, 3A2FBh
0x1934c7181  e81a56dcec                      call    sub_18028C7A0
0x1934c7186  c605ae59f0f101                  mov     cs:byte_1853CCB3B, 1
0x1934c718d  803d4067a8f100                  cmp     cs:byte_184F4D8D4, 0
0x1934c7194  0f8418ffffff                    jz      loc_1934C70B2
0x1934c719a  b914290400                      mov     ecx, 42914h
0x1934c719f  e87c4b66ff                      call    sub_192B2BD20
0x1934c71a4  4885c0                          test    rax, rax
0x1934c71a7  7436                            jz      short loc_1934C71DF
0x1934c71a9  4889c1                          mov     rcx, rax
0x1934c71ac  89f2                            mov     edx, esi
0x1934c71ae  4883c438                        add     rsp, 38h
0x1934c71b2  5f                              pop     rdi
0x1934c71b3  5e                              pop     rsi
0x1934c71b4  e9c78cdafa                      jmp     sub_18E26FE80
0x1934c71b9  e832e0dbec                      call    sub_1802851F0
0x1934c71be  488b053b62a0f1                  mov     rax, cs:qword_184ECD400
0x1934c71c5  488b8068e40200                  mov     rax, [rax+2E468h]
0x1934c71cc  4885c0                          test    rax, rax
0x1934c71cf  0f8508ffffff                    jnz     loc_1934C70DD
0x1934c71d5  e886e146ed                      call    sub_180935360
0x1934c71da  e881e146ed                      call    sub_180935360
0x1934c71df  e87ce146ed                      call    sub_180935360
