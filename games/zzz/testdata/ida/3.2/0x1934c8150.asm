0x1934c8150  4883ec28                        sub     rsp, 28h
0x1934c8154  803d3c4af0f100                  cmp     cs:byte_1853CCB97, 0
0x1934c815b  744a                            jz      short loc_1934C81A7
0x1934c815d  803dd457a8f100                  cmp     cs:byte_184F4D938, 0
0x1934c8164  755b                            jnz     short loc_1934C81C1
0x1934c8166  488b0d7392aff1                  mov     rcx, cs:qword_184FC13E0
0x1934c816d  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x1934c8174  7466                            jz      short loc_1934C81DC
0x1934c8176  e815fcffff                      call    sub_1934C7D90
0x1934c817b  488b0d5e92aff1                  mov     rcx, cs:qword_184FC13E0
0x1934c8182  0fb691cb000000                  movzx   edx, byte ptr [rcx+0CBh]
0x1934c8189  85c0                            test    eax, eax
0x1934c818b  740d                            jz      short loc_1934C819A
0x1934c818d  84d2                            test    dl, dl
0x1934c818f  7452                            jz      short loc_1934C81E3
0x1934c8191  4883c428                        add     rsp, 28h
0x1934c8195  e9f6fbffff                      jmp     sub_1934C7D90
0x1934c819a  84d2                            test    dl, dl
0x1934c819c  7454                            jz      short loc_1934C81F2
0x1934c819e  4883c428                        add     rsp, 28h
0x1934c81a2  e939200000                      jmp     sub_1934CA1E0
0x1934c81a7  b957a30300                      mov     ecx, 3A357h
0x1934c81ac  e8ef45dcec                      call    sub_18028C7A0
0x1934c81b1  c605df49f0f101                  mov     cs:byte_1853CCB97, 1
0x1934c81b8  803d7957a8f100                  cmp     cs:byte_184F4D938, 0
0x1934c81bf  74a5                            jz      short loc_1934C8166
0x1934c81c1  b978290400                      mov     ecx, 42978h
0x1934c81c6  e8553b66ff                      call    sub_192B2BD20
0x1934c81cb  4885c0                          test    rax, rax
0x1934c81ce  7431                            jz      short loc_1934C8201
0x1934c81d0  4889c1                          mov     rcx, rax
0x1934c81d3  4883c428                        add     rsp, 28h
0x1934c81d7  e9b485ddfa                      jmp     sub_18E2A0790
0x1934c81dc  e80fd0dbec                      call    sub_1802851F0
0x1934c81e1  eb93                            jmp     short loc_1934C8176
0x1934c81e3  e808d0dbec                      call    sub_1802851F0
0x1934c81e8  90                              nop
0x1934c81e9  4883c428                        add     rsp, 28h
0x1934c81ed  e99efbffff                      jmp     sub_1934C7D90
0x1934c81f2  e8f9cfdbec                      call    sub_1802851F0
0x1934c81f7  90                              nop
0x1934c81f8  4883c428                        add     rsp, 28h
0x1934c81fc  e9df1f0000                      jmp     sub_1934CA1E0
0x1934c8201  e85ad146ed                      call    sub_180935360
