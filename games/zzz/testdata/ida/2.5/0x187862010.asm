0x187862010  56                              push    rsi
0x187862011  57                              push    rdi
0x187862012  4883ec38                        sub     rsp, 38h
0x187862016  803d9285abfd00                  cmp     cs:byte_18531A5AF, 0
0x18786201d  0f84bf000000                    jz      loc_1878620E2
0x187862023  488b0d765f72fd                  mov     rcx, cs:qword_184F87FA0
0x18786202a  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x187862031  0f84d0000000                    jz      loc_187862107
0x187862037  488b050a1b63fd                  mov     rax, cs:qword_184E93B48
0x18786203e  488b80a0d90300                  mov     rax, [rax+3D9A0h]
0x187862045  4885c0                          test    rax, rax
0x187862048  0f84d5000000                    jz      loc_187862123
0x18786204e  488b7868                        mov     rdi, [rax+68h]
0x187862052  4885ff                          test    rdi, rdi
0x187862055  0f84cd000000                    jz      loc_187862128
0x18786205b  4c8b05e66f76fd                  mov     r8, cs:qword_184FC9048
0x187862062  488b07                          mov     rax, [rdi]
0x187862065  0fb788c2000000                  movzx   ecx, word ptr [rax+0C2h]
0x18786206c  4885c9                          test    rcx, rcx
0x18786206f  741e                            jz      short loc_18786208F
0x187862071  488b5018                        mov     rdx, [rax+18h]
0x187862075  48c1e104                        shl     rcx, 4
0x187862079  31f6                            xor     esi, esi
0x18786207b  0f1f440000                      nop     dword ptr [rax+rax+00h]
0x187862080  4c390432                        cmp     [rdx+rsi], r8
0x187862084  7425                            jz      short loc_1878620AB
0x187862086  4883c610                        add     rsi, 10h
0x18786208a  4839f1                          cmp     rcx, rsi
0x18786208d  75f1                            jnz     short loc_187862080
0x18786208f  488d4c2428                      lea     rcx, [rsp+48h+var_20]
0x187862094  4889fa                          mov     rdx, rdi
0x187862097  4531c9                          xor     r9d, r9d
0x18786209a  e8c1f09df8                      call    sub_180241160
0x18786209f  488b542428                      mov     rdx, [rsp+48h+var_20]
0x1878620a4  4c8b442430                      mov     r8, [rsp+48h+var_18]
0x1878620a9  eb29                            jmp     short loc_1878620D4
0x1878620ab  48634c3208                      movsxd  rcx, dword ptr [rdx+rsi+8]
0x1878620b0  4c8b84c8d0000000                mov     r8, [rax+rcx*8+0D0h]
0x1878620b8  4c89442430                      mov     [rsp+48h+var_18], r8
0x1878620bd  0fb790c6000000                  movzx   edx, word ptr [rax+0C6h]
0x1878620c4  4801ca                          add     rdx, rcx
0x1878620c7  488b94d0d0000000                mov     rdx, [rax+rdx*8+0D0h]
0x1878620cf  4889542428                      mov     [rsp+48h+var_20], rdx
0x1878620d4  4889f9                          mov     rcx, rdi
0x1878620d7  41ffd0                          call    r8
0x1878620da  90                              nop
0x1878620db  4883c438                        add     rsp, 38h
0x1878620df  5f                              pop     rdi
0x1878620e0  5e                              pop     rsi
0x1878620e1  c3                              retn
0x1878620e2  b9df7e0300                      mov     ecx, 37EDFh
0x1878620e7  e864929ef8                      call    sub_18024B350
0x1878620ec  c605bc84abfd01                  mov     cs:byte_18531A5AF, 1
0x1878620f3  488b0da65e72fd                  mov     rcx, cs:qword_184F87FA0
0x1878620fa  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x187862101  0f8530ffffff                    jnz     loc_187862037
0x187862107  e8e41d9ef8                      call    sub_180243EF0
0x18786210c  488b05351a63fd                  mov     rax, cs:qword_184E93B48
0x187862113  488b80a0d90300                  mov     rax, [rax+3D9A0h]
0x18786211a  4885c0                          test    rax, rax
0x18786211d  0f852bffffff                    jnz     loc_18786204E
0x187862123  e818f32ef9                      call    sub_180B51440
0x187862128  e813f32ef9                      call    sub_180B51440
