0x19e956490  85c9                            test    ecx, ecx
0x19e956492  7417                            jz      short loc_19E9564AB
0x19e956494  f30f2ac9                        cvtsi2ss xmm1, ecx
0x19e956498  f30f1005dcdadae3                movss   xmm0, cs:flt_182703F7C
0x19e9564a0  f30f5ec1                        divss   xmm0, xmm1
0x19e9564a4  48ff258d48abe6                  jmp     cs:qword_18540AD38
0x19e9564ab  0f57c0                          xorps   xmm0, xmm0
0x19e9564ae  48ff258348abe6                  jmp     cs:qword_18540AD38
