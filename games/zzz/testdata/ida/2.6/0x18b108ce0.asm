0x18b108ce0  56                              push    rsi
0x18b108ce1  57                              push    rdi
0x18b108ce2  4883ec38                        sub     rsp, 38h
0x18b108ce6  803d2830bcf900                  cmp     cs:byte_184CCBD15, 0
0x18b108ced  0f84bf000000                    jz      loc_18B108DB2
0x18b108cf3  488b0d968184f9                  mov     rcx, cs:qword_184950E90
0x18b108cfa  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x18b108d01  0f84d0000000                    jz      loc_18B108DD7
0x18b108d07  488b0552cf77f9                  mov     rax, cs:qword_184885C60
0x18b108d0e  488b80a8950200                  mov     rax, [rax+295A8h]
0x18b108d15  4885c0                          test    rax, rax
0x18b108d18  0f84d5000000                    jz      loc_18B108DF3
0x18b108d1e  488b7860                        mov     rdi, [rax+60h]
0x18b108d22  4885ff                          test    rdi, rdi
0x18b108d25  0f84cd000000                    jz      loc_18B108DF8
0x18b108d2b  4c8b05166385f9                  mov     r8, cs:qword_18495F048
0x18b108d32  488b07                          mov     rax, [rdi]
0x18b108d35  0fb788c6000000                  movzx   ecx, word ptr [rax+0C6h]
0x18b108d3c  4885c9                          test    rcx, rcx
0x18b108d3f  741e                            jz      short loc_18B108D5F
0x18b108d41  488b5030                        mov     rdx, [rax+30h]
0x18b108d45  48c1e104                        shl     rcx, 4
0x18b108d49  31f6                            xor     esi, esi
0x18b108d4b  0f1f440000                      nop     dword ptr [rax+rax+00h]
0x18b108d50  4c390432                        cmp     [rdx+rsi], r8
0x18b108d54  7425                            jz      short loc_18B108D7B
0x18b108d56  4883c610                        add     rsi, 10h
0x18b108d5a  4839f1                          cmp     rcx, rsi
0x18b108d5d  75f1                            jnz     short loc_18B108D50
0x18b108d5f  488d4c2428                      lea     rcx, [rsp+48h+var_20]
0x18b108d64  4889fa                          mov     rdx, rdi
0x18b108d67  4531c9                          xor     r9d, r9d
0x18b108d6a  e8818c12f5                      call    sub_1802319F0
0x18b108d6f  4c8b442428                      mov     r8, [rsp+48h+var_20]
0x18b108d74  488b542430                      mov     rdx, [rsp+48h+var_18]
0x18b108d79  eb29                            jmp     short loc_18B108DA4
0x18b108d7b  48634c3208                      movsxd  rcx, dword ptr [rdx+rsi+8]
0x18b108d80  4c8b84c8d0000000                mov     r8, [rax+rcx*8+0D0h]
0x18b108d88  4c89442428                      mov     [rsp+48h+var_20], r8
0x18b108d8d  0fb790c2000000                  movzx   edx, word ptr [rax+0C2h]
0x18b108d94  4801ca                          add     rdx, rcx
0x18b108d97  488b94d0d0000000                mov     rdx, [rax+rdx*8+0D0h]
0x18b108d9f  4889542430                      mov     [rsp+48h+var_18], rdx
0x18b108da4  4889f9                          mov     rcx, rdi
0x18b108da7  41ffd0                          call    r8
0x18b108daa  90                              nop
0x18b108dab  4883c438                        add     rsp, 38h
0x18b108daf  5f                              pop     rdi
0x18b108db0  5e                              pop     rsi
0x18b108db1  c3                              retn
0x18b108db2  b955da0100                      mov     ecx, 1DA55h
0x18b108db7  e8742e13f5                      call    sub_18023BC30
0x18b108dbc  c605522fbcf901                  mov     cs:byte_184CCBD15, 1
0x18b108dc3  488b0dc68084f9                  mov     rcx, cs:qword_184950E90
0x18b108dca  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x18b108dd1  0f8530ffffff                    jnz     loc_18B108D07
0x18b108dd7  e894b912f5                      call    sub_180234770
0x18b108ddc  488b057dce77f9                  mov     rax, cs:qword_184885C60
0x18b108de3  488b80a8950200                  mov     rax, [rax+295A8h]
0x18b108dea  4885c0                          test    rax, rax
0x18b108ded  0f852bffffff                    jnz     loc_18B108D1E
0x18b108df3  e8680987f5                      call    sub_180979760
0x18b108df8  e8630987f5                      call    sub_180979760
