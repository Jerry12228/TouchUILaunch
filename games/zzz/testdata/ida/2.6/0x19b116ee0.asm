0x19b116ee0  56                              push    rsi
0x19b116ee1  4883ec70                        sub     rsp, 70h
0x19b116ee5  89d0                            mov     eax, edx
0x19b116ee7  4889ce                          mov     rsi, rcx
0x19b116eea  0f57c0                          xorps   xmm0, xmm0
0x19b116eed  0f29442450                      movaps  [rsp+78h+var_28], xmm0
0x19b116ef2  0f29442440                      movaps  [rsp+78h+var_38], xmm0
0x19b116ef7  0f29442430                      movaps  [rsp+78h+var_48], xmm0
0x19b116efc  0f29442420                      movaps  [rsp+78h+var_58], xmm0
0x19b116f01  c744246000000000                mov     [rsp+78h+var_18], 0
0x19b116f09  488d542420                      lea     rdx, [rsp+78h+var_58]
0x19b116f0e  89c1                            mov     ecx, eax
0x19b116f10  ff1572fb7fe9                    call    cs:qword_184916A88
0x19b116f16  8b442460                        mov     eax, [rsp+78h+var_18]
0x19b116f1a  894640                          mov     [rsi+40h], eax
0x19b116f1d  0f28442420                      movaps  xmm0, [rsp+78h+var_58]
0x19b116f22  0f284c2430                      movaps  xmm1, [rsp+78h+var_48]
0x19b116f27  0f28542440                      movaps  xmm2, [rsp+78h+var_38]
0x19b116f2c  0f285c2450                      movaps  xmm3, [rsp+78h+var_28]
0x19b116f31  0f115e30                        movups  xmmword ptr [rsi+30h], xmm3
0x19b116f35  0f115620                        movups  xmmword ptr [rsi+20h], xmm2
0x19b116f39  0f114e10                        movups  xmmword ptr [rsi+10h], xmm1
0x19b116f3d  0f1106                          movups  xmmword ptr [rsi], xmm0
0x19b116f40  4889f0                          mov     rax, rsi
0x19b116f43  4883c470                        add     rsp, 70h
0x19b116f47  5e                              pop     rsi
0x19b116f48  c3                              retn
