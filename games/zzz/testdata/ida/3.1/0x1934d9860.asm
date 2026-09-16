0x1934d9860  56                              push    rsi
0x1934d9861  4883ec20                        sub     rsp, 20h
0x1934d9865  803d0f873bf200                  cmp     cs:byte_185891F7B, 0
0x1934d986c  7427                            jz      short loc_1934D9895
0x1934d986e  488b0d1319f4f1                  mov     rcx, cs:qword_18541B188
0x1934d9875  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x1934d987c  7438                            jz      short loc_1934D98B6
0x1934d987e  ff15d4dbf2f1                    call    cs:qword_185407458
0x1934d9884  89c6                            mov     esi, eax
0x1934d9886  ff15d4dbf2f1                    call    cs:qword_185407460
0x1934d988c  0fafc6                          imul    eax, esi
0x1934d988f  4883c420                        add     rsp, 20h
0x1934d9893  5e                              pop     rsi
0x1934d9894  c3                              retn
0x1934d9895  b98b050400                      mov     ecx, 4058Bh
0x1934d989a  e841fbd9ec                      call    sub_1802793E0
0x1934d989f  c605d5863bf201                  mov     cs:byte_185891F7B, 1
0x1934d98a6  488b0ddb18f4f1                  mov     rcx, cs:qword_18541B188
0x1934d98ad  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x1934d98b4  75c8                            jnz     short loc_1934D987E
0x1934d98b6  e89585d9ec                      call    sub_180271E50
0x1934d98bb  ebc1                            jmp     short loc_1934D987E
