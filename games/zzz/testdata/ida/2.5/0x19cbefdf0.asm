0x19cbefdf0  56                              push    rsi
0x19cbefdf1  4883ec70                        sub     rsp, 70h
0x19cbefdf5  89d0                            mov     eax, edx
0x19cbefdf7  4889ce                          mov     rsi, rcx
0x19cbefdfa  0f57c0                          xorps   xmm0, xmm0
0x19cbefdfd  0f29442450                      movaps  [rsp+78h+var_28], xmm0
0x19cbefe02  0f29442440                      movaps  [rsp+78h+var_38], xmm0
0x19cbefe07  0f29442430                      movaps  [rsp+78h+var_48], xmm0
0x19cbefe0c  0f29442420                      movaps  [rsp+78h+var_58], xmm0
0x19cbefe11  c744246000000000                mov     [rsp+78h+var_18], 0
0x19cbefe19  488d542420                      lea     rdx, [rsp+78h+var_58]
0x19cbefe1e  89c1                            mov     ecx, eax
0x19cbefe20  ff157a4135e8                    call    cs:qword_184F43FA0
0x19cbefe26  8b442460                        mov     eax, [rsp+78h+var_18]
0x19cbefe2a  894640                          mov     [rsi+40h], eax
0x19cbefe2d  0f28442420                      movaps  xmm0, [rsp+78h+var_58]
0x19cbefe32  0f284c2430                      movaps  xmm1, [rsp+78h+var_48]
0x19cbefe37  0f28542440                      movaps  xmm2, [rsp+78h+var_38]
0x19cbefe3c  0f285c2450                      movaps  xmm3, [rsp+78h+var_28]
0x19cbefe41  0f115e30                        movups  xmmword ptr [rsi+30h], xmm3
0x19cbefe45  0f115620                        movups  xmmword ptr [rsi+20h], xmm2
0x19cbefe49  0f114e10                        movups  xmmword ptr [rsi+10h], xmm1
0x19cbefe4d  0f1106                          movups  xmmword ptr [rsi], xmm0
0x19cbefe50  4889f0                          mov     rax, rsi
0x19cbefe53  4883c470                        add     rsp, 70h
0x19cbefe57  5e                              pop     rsi
0x19cbefe58  c3                              retn
