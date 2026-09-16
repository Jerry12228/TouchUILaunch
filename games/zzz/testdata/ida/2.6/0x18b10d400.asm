0x18b10d400  56                              push    rsi
0x18b10d401  57                              push    rdi
0x18b10d402  4883ec38                        sub     rsp, 38h
0x18b10d406  803d2ae9bbf900                  cmp     cs:byte_184CCBD37, 0
0x18b10d40d  0f84bf000000                    jz      loc_18B10D4D2
0x18b10d413  488b0d763a84f9                  mov     rcx, cs:qword_184950E90
0x18b10d41a  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x18b10d421  0f84d0000000                    jz      loc_18B10D4F7
0x18b10d427  488b05328877f9                  mov     rax, cs:qword_184885C60
0x18b10d42e  488b80a8950200                  mov     rax, [rax+295A8h]
0x18b10d435  4885c0                          test    rax, rax
0x18b10d438  0f84d5000000                    jz      loc_18B10D513
0x18b10d43e  488b7870                        mov     rdi, [rax+70h]
0x18b10d442  4885ff                          test    rdi, rdi
0x18b10d445  0f84cd000000                    jz      loc_18B10D518
0x18b10d44b  4c8b05f61b85f9                  mov     r8, cs:qword_18495F048
0x18b10d452  488b07                          mov     rax, [rdi]
0x18b10d455  0fb788c6000000                  movzx   ecx, word ptr [rax+0C6h]
0x18b10d45c  4885c9                          test    rcx, rcx
0x18b10d45f  741e                            jz      short loc_18B10D47F
0x18b10d461  488b5030                        mov     rdx, [rax+30h]
0x18b10d465  48c1e104                        shl     rcx, 4
0x18b10d469  31f6                            xor     esi, esi
0x18b10d46b  0f1f440000                      nop     dword ptr [rax+rax+00h]
0x18b10d470  4c390432                        cmp     [rdx+rsi], r8
0x18b10d474  7425                            jz      short loc_18B10D49B
0x18b10d476  4883c610                        add     rsi, 10h
0x18b10d47a  4839f1                          cmp     rcx, rsi
0x18b10d47d  75f1                            jnz     short loc_18B10D470
0x18b10d47f  488d4c2428                      lea     rcx, [rsp+48h+var_20]
0x18b10d484  4889fa                          mov     rdx, rdi
0x18b10d487  4531c9                          xor     r9d, r9d
0x18b10d48a  e8614512f5                      call    sub_1802319F0
0x18b10d48f  4c8b442428                      mov     r8, [rsp+48h+var_20]
0x18b10d494  488b542430                      mov     rdx, [rsp+48h+var_18]
0x18b10d499  eb29                            jmp     short loc_18B10D4C4
0x18b10d49b  48634c3208                      movsxd  rcx, dword ptr [rdx+rsi+8]
0x18b10d4a0  4c8b84c8d0000000                mov     r8, [rax+rcx*8+0D0h]
0x18b10d4a8  4c89442428                      mov     [rsp+48h+var_20], r8
0x18b10d4ad  0fb790c2000000                  movzx   edx, word ptr [rax+0C2h]
0x18b10d4b4  4801ca                          add     rdx, rcx
0x18b10d4b7  488b94d0d0000000                mov     rdx, [rax+rdx*8+0D0h]
0x18b10d4bf  4889542430                      mov     [rsp+48h+var_18], rdx
0x18b10d4c4  4889f9                          mov     rcx, rdi
0x18b10d4c7  41ffd0                          call    r8
0x18b10d4ca  90                              nop
0x18b10d4cb  4883c438                        add     rsp, 38h
0x18b10d4cf  5f                              pop     rdi
0x18b10d4d0  5e                              pop     rsi
0x18b10d4d1  c3                              retn
0x18b10d4d2  b977da0100                      mov     ecx, 1DA77h
0x18b10d4d7  e854e712f5                      call    sub_18023BC30
0x18b10d4dc  c60554e8bbf901                  mov     cs:byte_184CCBD37, 1
0x18b10d4e3  488b0da63984f9                  mov     rcx, cs:qword_184950E90
0x18b10d4ea  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x18b10d4f1  0f8530ffffff                    jnz     loc_18B10D427
0x18b10d4f7  e8747212f5                      call    sub_180234770
0x18b10d4fc  488b055d8777f9                  mov     rax, cs:qword_184885C60
0x18b10d503  488b80a8950200                  mov     rax, [rax+295A8h]
0x18b10d50a  4885c0                          test    rax, rax
0x18b10d50d  0f852bffffff                    jnz     loc_18B10D43E
0x18b10d513  e848c286f5                      call    sub_180979760
0x18b10d518  e843c286f5                      call    sub_180979760
