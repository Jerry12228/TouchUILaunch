0x19dc778b0  85c9                            test    ecx, ecx
0x19dc778b2  7417                            jz      short loc_19DC778CB
0x19dc778b4  f30f2ac9                        cvtsi2ss xmm1, ecx
0x19dc778b8  f30f1005ecc26de4                movss   xmm0, cs:flt_182353BAC
0x19dc778c0  f30f5ec1                        divss   xmm0, xmm1
0x19dc778c4  48ff253d4630e7                  jmp     cs:qword_184F7BF08
0x19dc778cb  0f57c0                          xorps   xmm0, xmm0
0x19dc778ce  48ff25334630e7                  jmp     cs:qword_184F7BF08
