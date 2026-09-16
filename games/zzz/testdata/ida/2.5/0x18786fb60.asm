0x18786fb60  56                              push    rsi
0x18786fb61  57                              push    rdi
0x18786fb62  4883ec38                        sub     rsp, 38h
0x18786fb66  803dd2aaaafd00                  cmp     cs:byte_18531A63F, 0
0x18786fb6d  0f84bf000000                    jz      loc_18786FC32
0x18786fb73  488b0d268471fd                  mov     rcx, cs:qword_184F87FA0
0x18786fb7a  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x18786fb81  0f84d0000000                    jz      loc_18786FC57
0x18786fb87  488b05ba3f62fd                  mov     rax, cs:qword_184E93B48
0x18786fb8e  488b80a0d90300                  mov     rax, [rax+3D9A0h]
0x18786fb95  4885c0                          test    rax, rax
0x18786fb98  0f84d5000000                    jz      loc_18786FC73
0x18786fb9e  488b7870                        mov     rdi, [rax+70h]
0x18786fba2  4885ff                          test    rdi, rdi
0x18786fba5  0f84cd000000                    jz      loc_18786FC78
0x18786fbab  4c8b05969475fd                  mov     r8, cs:qword_184FC9048
0x18786fbb2  488b07                          mov     rax, [rdi]
0x18786fbb5  0fb788c2000000                  movzx   ecx, word ptr [rax+0C2h]
0x18786fbbc  4885c9                          test    rcx, rcx
0x18786fbbf  741e                            jz      short loc_18786FBDF
0x18786fbc1  488b5018                        mov     rdx, [rax+18h]
0x18786fbc5  48c1e104                        shl     rcx, 4
0x18786fbc9  31f6                            xor     esi, esi
0x18786fbcb  0f1f440000                      nop     dword ptr [rax+rax+00h]
0x18786fbd0  4c390432                        cmp     [rdx+rsi], r8
0x18786fbd4  7425                            jz      short loc_18786FBFB
0x18786fbd6  4883c610                        add     rsi, 10h
0x18786fbda  4839f1                          cmp     rcx, rsi
0x18786fbdd  75f1                            jnz     short loc_18786FBD0
0x18786fbdf  488d4c2428                      lea     rcx, [rsp+48h+var_20]
0x18786fbe4  4889fa                          mov     rdx, rdi
0x18786fbe7  4531c9                          xor     r9d, r9d
0x18786fbea  e871159df8                      call    sub_180241160
0x18786fbef  488b542428                      mov     rdx, [rsp+48h+var_20]
0x18786fbf4  4c8b442430                      mov     r8, [rsp+48h+var_18]
0x18786fbf9  eb29                            jmp     short loc_18786FC24
0x18786fbfb  48634c3208                      movsxd  rcx, dword ptr [rdx+rsi+8]
0x18786fc00  4c8b84c8d0000000                mov     r8, [rax+rcx*8+0D0h]
0x18786fc08  4c89442430                      mov     [rsp+48h+var_18], r8
0x18786fc0d  0fb790c6000000                  movzx   edx, word ptr [rax+0C6h]
0x18786fc14  4801ca                          add     rdx, rcx
0x18786fc17  488b94d0d0000000                mov     rdx, [rax+rdx*8+0D0h]
0x18786fc1f  4889542428                      mov     [rsp+48h+var_20], rdx
0x18786fc24  4889f9                          mov     rcx, rdi
0x18786fc27  41ffd0                          call    r8
0x18786fc2a  90                              nop
0x18786fc2b  4883c438                        add     rsp, 38h
0x18786fc2f  5f                              pop     rdi
0x18786fc30  5e                              pop     rsi
0x18786fc31  c3                              retn
0x18786fc32  b96f7f0300                      mov     ecx, 37F6Fh
0x18786fc37  e814b79df8                      call    sub_18024B350
0x18786fc3c  c605fca9aafd01                  mov     cs:byte_18531A63F, 1
0x18786fc43  488b0d568371fd                  mov     rcx, cs:qword_184F87FA0
0x18786fc4a  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x18786fc51  0f8530ffffff                    jnz     loc_18786FB87
0x18786fc57  e894429df8                      call    sub_180243EF0
0x18786fc5c  488b05e53e62fd                  mov     rax, cs:qword_184E93B48
0x18786fc63  488b80a0d90300                  mov     rax, [rax+3D9A0h]
0x18786fc6a  4885c0                          test    rax, rax
0x18786fc6d  0f852bffffff                    jnz     loc_18786FB9E
0x18786fc73  e8c8172ef9                      call    sub_180B51440
0x18786fc78  e8c3172ef9                      call    sub_180B51440
