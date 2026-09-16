0x18786fc80  4883ec28                        sub     rsp, 28h
0x18786fc84  803d86a9aafd00                  cmp     cs:byte_18531A611, 0
0x18786fc8b  744a                            jz      short loc_18786FCD7
0x18786fc8d  803d9f7a69fd00                  cmp     cs:byte_184F07733, 0
0x18786fc94  755b                            jnz     short loc_18786FCF1
0x18786fc96  488b0d038371fd                  mov     rcx, cs:qword_184F87FA0
0x18786fc9d  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x18786fca4  747e                            jz      short loc_18786FD24
0x18786fca6  e86523ffff                      call    sub_187862010
0x18786fcab  488b0dee8271fd                  mov     rcx, cs:qword_184F87FA0
0x18786fcb2  0fb691cb000000                  movzx   edx, byte ptr [rcx+0CBh]
0x18786fcb9  85c0                            test    eax, eax
0x18786fcbb  740d                            jz      short loc_18786FCCA
0x18786fcbd  84d2                            test    dl, dl
0x18786fcbf  746d                            jz      short loc_18786FD2E
0x18786fcc1  4883c428                        add     rsp, 28h
0x18786fcc5  e94623ffff                      jmp     sub_187862010
0x18786fcca  84d2                            test    dl, dl
0x18786fccc  746f                            jz      short loc_18786FD3D
0x18786fcce  4883c428                        add     rsp, 28h
0x18786fcd2  e989feffff                      jmp     sub_18786FB60
0x18786fcd7  b9417f0300                      mov     ecx, 37F41h
0x18786fcdc  e86fb69df8                      call    sub_18024B350
0x18786fce1  c60529a9aafd01                  mov     cs:byte_18531A611, 1
0x18786fce8  803d447a69fd00                  cmp     cs:byte_184F07733, 0
0x18786fcef  74a5                            jz      short loc_18786FC96
0x18786fcf1  488b0d48a974fd                  mov     rcx, cs:qword_184FBA640
0x18786fcf8  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x18786fcff  744b                            jz      short loc_18786FD4C
0x18786fd01  488b05403e62fd                  mov     rax, cs:qword_184E93B48
0x18786fd08  488b80a0240500                  mov     rax, [rax+524A0h]
0x18786fd0f  488b88b8fb0d00                  mov     rcx, [rax+0DFBB8h]
0x18786fd16  4885c9                          test    rcx, rcx
0x18786fd19  7438                            jz      short loc_18786FD53
0x18786fd1b  4883c428                        add     rsp, 28h
0x18786fd1f  e95c685800                      jmp     sub_187DF6580
0x18786fd24  e8c7419df8                      call    sub_180243EF0
0x18786fd29  e978ffffff                      jmp     loc_18786FCA6
0x18786fd2e  e8bd419df8                      call    sub_180243EF0
0x18786fd33  90                              nop
0x18786fd34  4883c428                        add     rsp, 28h
0x18786fd38  e9d322ffff                      jmp     sub_187862010
0x18786fd3d  e8ae419df8                      call    sub_180243EF0
0x18786fd42  90                              nop
0x18786fd43  4883c428                        add     rsp, 28h
0x18786fd47  e914feffff                      jmp     sub_18786FB60
0x18786fd4c  e89f419df8                      call    sub_180243EF0
0x18786fd51  ebae                            jmp     short loc_18786FD01
0x18786fd53  e8e8162ef9                      call    sub_180B51440
