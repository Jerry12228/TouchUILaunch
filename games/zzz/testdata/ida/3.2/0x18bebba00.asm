0x18bebba00  56                              push    rsi
0x18bebba01  57                              push    rdi
0x18bebba02  4883ec58                        sub     rsp, 58h
0x18bebba06  0f29742440                      movaps  [rsp+68h+var_28], xmm6
0x18bebba0b  0f28f2                          movaps  xmm6, xmm2
0x18bebba0e  4889d6                          mov     rsi, rdx
0x18bebba11  4889cf                          mov     rdi, rcx
0x18bebba14  0f57c0                          xorps   xmm0, xmm0
0x18bebba17  0f29442420                      movaps  [rsp+68h+var_48], xmm0
0x18bebba1c  ff15ce040cf9                    call    cs:qword_184F7BEF0
0x18bebba22  c644242001                      mov     byte ptr [rsp+68h+var_48], 1
0x18bebba27  f30f11742424                    movss   dword ptr [rsp+68h+var_48+4], xmm6
0x18bebba2d  89442428                        mov     dword ptr [rsp+68h+var_48+8], eax
0x18bebba31  4889742430                      mov     [rsp+68h+var_38], rsi
0x18bebba36  488b4f10                        mov     rcx, [rdi+10h]
0x18bebba3a  4885c9                          test    rcx, rcx
0x18bebba3d  7416                            jz      short loc_18BEBBA55
0x18bebba3f  488d542420                      lea     rdx, [rsp+68h+var_48]
0x18bebba44  e8f75f0104                      call    sub_18FED1A40
0x18bebba49  0f28742440                      movaps  xmm6, [rsp+68h+var_28]
0x18bebba4e  4883c458                        add     rsp, 58h
0x18bebba52  5f                              pop     rdi
0x18bebba53  5e                              pop     rsi
0x18bebba54  c3                              retn
0x18bebba55  e80699a7f4                      call    sub_180935360
