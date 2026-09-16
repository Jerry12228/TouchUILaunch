0x187862130  56                              push    rsi
0x187862131  57                              push    rdi
0x187862132  53                              push    rbx
0x187862133  4883ec30                        sub     rsp, 30h
0x187862137  89cb                            mov     ebx, ecx
0x187862139  803d7084abfd00                  cmp     cs:byte_18531A5B0, 0
0x187862140  0f84d5000000                    jz      loc_18786221B
0x187862146  803d79556afd00                  cmp     cs:byte_184F076C6, 0
0x18786214d  0f85e6000000                    jnz     loc_187862239
0x187862153  488b0d465e72fd                  mov     rcx, cs:qword_184F87FA0
0x18786215a  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x187862161  0f840a010000                    jz      loc_187862271
0x187862167  488b05da1963fd                  mov     rax, cs:qword_184E93B48
0x18786216e  488b80a0d90300                  mov     rax, [rax+3D9A0h]
0x187862175  4885c0                          test    rax, rax
0x187862178  0f840f010000                    jz      loc_18786228D
0x18786217e  488b7868                        mov     rdi, [rax+68h]
0x187862182  4885ff                          test    rdi, rdi
0x187862185  0f8407010000                    jz      loc_187862292
0x18786218b  4c8b05b66e76fd                  mov     r8, cs:qword_184FC9048
0x187862192  488b07                          mov     rax, [rdi]
0x187862195  0fb788c2000000                  movzx   ecx, word ptr [rax+0C2h]
0x18786219c  4885c9                          test    rcx, rcx
0x18786219f  741e                            jz      short loc_1878621BF
0x1878621a1  488b5018                        mov     rdx, [rax+18h]
0x1878621a5  48c1e104                        shl     rcx, 4
0x1878621a9  31f6                            xor     esi, esi
0x1878621ab  0f1f440000                      nop     dword ptr [rax+rax+00h]
0x1878621b0  4c390432                        cmp     [rdx+rsi], r8
0x1878621b4  7428                            jz      short loc_1878621DE
0x1878621b6  4883c610                        add     rsi, 10h
0x1878621ba  4839f1                          cmp     rcx, rsi
0x1878621bd  75f1                            jnz     short loc_1878621B0
0x1878621bf  488d4c2420                      lea     rcx, [rsp+48h+var_28]
0x1878621c4  4889fa                          mov     rdx, rdi
0x1878621c7  41b901000000                    mov     r9d, 1
0x1878621cd  e88eef9df8                      call    sub_180241160
0x1878621d2  4c8b442420                      mov     r8, [rsp+48h+var_28]
0x1878621d7  488b742428                      mov     rsi, [rsp+48h+var_20]
0x1878621dc  eb2d                            jmp     short loc_18786220B
0x1878621de  8b4c3208                        mov     ecx, [rdx+rsi+8]
0x1878621e2  ffc1                            inc     ecx
0x1878621e4  4863c9                          movsxd  rcx, ecx
0x1878621e7  488bb4c8d0000000                mov     rsi, [rax+rcx*8+0D0h]
0x1878621ef  4889742428                      mov     [rsp+48h+var_20], rsi
0x1878621f4  0fb790c6000000                  movzx   edx, word ptr [rax+0C6h]
0x1878621fb  4801ca                          add     rdx, rcx
0x1878621fe  4c8b84d0d0000000                mov     r8, [rax+rdx*8+0D0h]
0x187862206  4c89442420                      mov     [rsp+48h+var_28], r8
0x18786220b  4889f9                          mov     rcx, rdi
0x18786220e  89da                            mov     edx, ebx
0x187862210  ffd6                            call    rsi
0x187862212  90                              nop
0x187862213  4883c430                        add     rsp, 30h
0x187862217  5b                              pop     rbx
0x187862218  5f                              pop     rdi
0x187862219  5e                              pop     rsi
0x18786221a  c3                              retn
0x18786221b  b9e07e0300                      mov     ecx, 37EE0h
0x187862220  e82b919ef8                      call    sub_18024B350
0x187862225  c6058483abfd01                  mov     cs:byte_18531A5B0, 1
0x18786222c  803d93546afd00                  cmp     cs:byte_184F076C6, 0
0x187862233  0f841affffff                    jz      loc_187862153
0x187862239  488b0d008475fd                  mov     rcx, cs:qword_184FBA640
0x187862240  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x187862247  744e                            jz      short loc_187862297
0x187862249  488b05f81863fd                  mov     rax, cs:qword_184E93B48
0x187862250  488b80a0240500                  mov     rax, [rax+524A0h]
0x187862257  488b8850f80d00                  mov     rcx, [rax+0DF850h]
0x18786225e  4885c9                          test    rcx, rcx
0x187862261  743b                            jz      short loc_18786229E
0x187862263  89da                            mov     edx, ebx
0x187862265  4883c430                        add     rsp, 30h
0x187862269  5b                              pop     rbx
0x18786226a  5f                              pop     rdi
0x18786226b  5e                              pop     rsi
0x18786226c  e9bfe15900                      jmp     sub_187E00430
0x187862271  e87a1c9ef8                      call    sub_180243EF0
0x187862276  488b05cb1863fd                  mov     rax, cs:qword_184E93B48
0x18786227d  488b80a0d90300                  mov     rax, [rax+3D9A0h]
0x187862284  4885c0                          test    rax, rax
0x187862287  0f85f1feffff                    jnz     loc_18786217E
0x18786228d  e8aef12ef9                      call    sub_180B51440
0x187862292  e8a9f12ef9                      call    sub_180B51440
0x187862297  e8541c9ef8                      call    sub_180243EF0
0x18786229c  ebab                            jmp     short loc_187862249
0x18786229e  e89df12ef9                      call    sub_180B51440
