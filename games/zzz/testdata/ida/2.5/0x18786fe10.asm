0x18786fe10  56                              push    rsi
0x18786fe11  4883ec20                        sub     rsp, 20h
0x18786fe15  89ce                            mov     esi, ecx
0x18786fe17  803df4a7aafd00                  cmp     cs:byte_18531A612, 0
0x18786fe1e  7430                            jz      short loc_18786FE50
0x18786fe20  803d0d7969fd00                  cmp     cs:byte_184F07734, 0
0x18786fe27  7541                            jnz     short loc_18786FE6A
0x18786fe29  85f6                            test    esi, esi
0x18786fe2b  7517                            jnz     short loc_18786FE44
0x18786fe2d  488b0d6c8171fd                  mov     rcx, cs:qword_184F87FA0
0x18786fe34  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x18786fe3b  7463                            jz      short loc_18786FEA0
0x18786fe3d  e83efeffff                      call    sub_18786FC80
0x18786fe42  89c6                            mov     esi, eax
0x18786fe44  83fe01                          cmp     esi, 1
0x18786fe47  0f94c0                          setz    al
0x18786fe4a  4883c420                        add     rsp, 20h
0x18786fe4e  5e                              pop     rsi
0x18786fe4f  c3                              retn
0x18786fe50  b9427f0300                      mov     ecx, 37F42h
0x18786fe55  e8f6b49df8                      call    sub_18024B350
0x18786fe5a  c605b1a7aafd01                  mov     cs:byte_18531A612, 1
0x18786fe61  803dcc7869fd00                  cmp     cs:byte_184F07734, 0
0x18786fe68  74bf                            jz      short loc_18786FE29
0x18786fe6a  488b0dcfa774fd                  mov     rcx, cs:qword_184FBA640
0x18786fe71  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x18786fe78  742d                            jz      short loc_18786FEA7
0x18786fe7a  488b05c73c62fd                  mov     rax, cs:qword_184E93B48
0x18786fe81  488b80a0240500                  mov     rax, [rax+524A0h]
0x18786fe88  488b88c0fb0d00                  mov     rcx, [rax+0DFBC0h]
0x18786fe8f  4885c9                          test    rcx, rcx
0x18786fe92  741a                            jz      short loc_18786FEAE
0x18786fe94  89f2                            mov     edx, esi
0x18786fe96  4883c420                        add     rsp, 20h
0x18786fe9a  5e                              pop     rsi
0x18786fe9b  e960185800                      jmp     sub_187DF1700
0x18786fea0  e84b409df8                      call    sub_180243EF0
0x18786fea5  eb96                            jmp     short loc_18786FE3D
0x18786fea7  e844409df8                      call    sub_180243EF0
0x18786feac  ebcc                            jmp     short loc_18786FE7A
0x18786feae  e88d152ef9                      call    sub_180B51440
