0x18786fec0  56                              push    rsi
0x18786fec1  4883ec20                        sub     rsp, 20h
0x18786fec5  89ce                            mov     esi, ecx
0x18786fec7  803d45a7aafd00                  cmp     cs:byte_18531A613, 0
0x18786fece  7430                            jz      short loc_18786FF00
0x18786fed0  803d5e7869fd00                  cmp     cs:byte_184F07735, 0
0x18786fed7  7541                            jnz     short loc_18786FF1A
0x18786fed9  85f6                            test    esi, esi
0x18786fedb  7517                            jnz     short loc_18786FEF4
0x18786fedd  488b0dbc8071fd                  mov     rcx, cs:qword_184F87FA0
0x18786fee4  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x18786feeb  7463                            jz      short loc_18786FF50
0x18786feed  e88efdffff                      call    sub_18786FC80
0x18786fef2  89c6                            mov     esi, eax
0x18786fef4  83fe02                          cmp     esi, 2
0x18786fef7  0f94c0                          setz    al
0x18786fefa  4883c420                        add     rsp, 20h
0x18786fefe  5e                              pop     rsi
0x18786feff  c3                              retn
0x18786ff00  b9437f0300                      mov     ecx, 37F43h
0x18786ff05  e846b49df8                      call    sub_18024B350
0x18786ff0a  c60502a7aafd01                  mov     cs:byte_18531A613, 1
0x18786ff11  803d1d7869fd00                  cmp     cs:byte_184F07735, 0
0x18786ff18  74bf                            jz      short loc_18786FED9
0x18786ff1a  488b0d1fa774fd                  mov     rcx, cs:qword_184FBA640
0x18786ff21  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x18786ff28  742d                            jz      short loc_18786FF57
0x18786ff2a  488b05173c62fd                  mov     rax, cs:qword_184E93B48
0x18786ff31  488b80a0240500                  mov     rax, [rax+524A0h]
0x18786ff38  488b88c8fb0d00                  mov     rcx, [rax+0DFBC8h]
0x18786ff3f  4885c9                          test    rcx, rcx
0x18786ff42  741a                            jz      short loc_18786FF5E
0x18786ff44  89f2                            mov     edx, esi
0x18786ff46  4883c420                        add     rsp, 20h
0x18786ff4a  5e                              pop     rsi
0x18786ff4b  e9b0175800                      jmp     sub_187DF1700
0x18786ff50  e89b3f9df8                      call    sub_180243EF0
0x18786ff55  eb96                            jmp     short loc_18786FEED
0x18786ff57  e8943f9df8                      call    sub_180243EF0
0x18786ff5c  ebcc                            jmp     short loc_18786FF2A
0x18786ff5e  e8dd142ef9                      call    sub_180B51440
