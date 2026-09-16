0x19ef67730  56                              push    rsi
0x19ef67731  4883ec70                        sub     rsp, 70h
0x19ef67735  89d0                            mov     eax, edx
0x19ef67737  4889ce                          mov     rsi, rcx
0x19ef6773a  0f57c0                          xorps   xmm0, xmm0
0x19ef6773d  0f29442450                      movaps  [rsp+78h+var_28], xmm0
0x19ef67742  0f29442440                      movaps  [rsp+78h+var_38], xmm0
0x19ef67747  0f29442430                      movaps  [rsp+78h+var_48], xmm0
0x19ef6774c  0f29442420                      movaps  [rsp+78h+var_58], xmm0
0x19ef67751  c744246000000000                mov     [rsp+78h+var_18], 0
0x19ef67759  488d542420                      lea     rdx, [rsp+78h+var_58]
0x19ef6775e  89c1                            mov     ecx, eax
0x19ef67760  ff15627801e6                    call    cs:qword_184F7EFC8
0x19ef67766  8b442460                        mov     eax, [rsp+78h+var_18]
0x19ef6776a  894640                          mov     [rsi+40h], eax
0x19ef6776d  0f28442420                      movaps  xmm0, [rsp+78h+var_58]
0x19ef67772  0f284c2430                      movaps  xmm1, [rsp+78h+var_48]
0x19ef67777  0f28542440                      movaps  xmm2, [rsp+78h+var_38]
0x19ef6777c  0f285c2450                      movaps  xmm3, [rsp+78h+var_28]
0x19ef67781  0f115e30                        movups  xmmword ptr [rsi+30h], xmm3
0x19ef67785  0f115620                        movups  xmmword ptr [rsi+20h], xmm2
0x19ef67789  0f114e10                        movups  xmmword ptr [rsi+10h], xmm1
0x19ef6778d  0f1106                          movups  xmmword ptr [rsi], xmm0
0x19ef67790  4889f0                          mov     rax, rsi
0x19ef67793  4883c470                        add     rsp, 70h
0x19ef67797  5e                              pop     rsi
0x19ef67798  c3                              retn
