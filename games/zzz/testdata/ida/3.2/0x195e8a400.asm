0x195e8a400  56                              push    rsi
0x195e8a401  57                              push    rdi
0x195e8a402  4883ec58                        sub     rsp, 58h
0x195e8a406  0f29742440                      movaps  [rsp+68h+var_28], xmm6
0x195e8a40b  0f28f2                          movaps  xmm6, xmm2
0x195e8a40e  4889d6                          mov     rsi, rdx
0x195e8a411  4889cf                          mov     rdi, rcx
0x195e8a414  0f57c0                          xorps   xmm0, xmm0
0x195e8a417  0f29442420                      movaps  [rsp+68h+var_48], xmm0
0x195e8a41c  ff15ce1a0fef                    call    cs:qword_184F7BEF0
0x195e8a422  c644242001                      mov     byte ptr [rsp+68h+var_48], 1
0x195e8a427  f30f11742424                    movss   dword ptr [rsp+68h+var_48+4], xmm6
0x195e8a42d  89442428                        mov     dword ptr [rsp+68h+var_48+8], eax
0x195e8a431  4889742430                      mov     [rsp+68h+var_38], rsi
0x195e8a436  488b4f20                        mov     rcx, [rdi+20h]
0x195e8a43a  4885c9                          test    rcx, rcx
0x195e8a43d  7416                            jz      short loc_195E8A455
0x195e8a43f  488d542420                      lea     rdx, [rsp+68h+var_48]
0x195e8a444  e8f77504fa                      call    sub_18FED1A40
0x195e8a449  0f28742440                      movaps  xmm6, [rsp+68h+var_28]
0x195e8a44e  4883c458                        add     rsp, 58h
0x195e8a452  5f                              pop     rdi
0x195e8a453  5e                              pop     rsi
0x195e8a454  c3                              retn
0x195e8a455  e806afaaea                      call    sub_180935360
