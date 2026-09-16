0x18d838640  56                              push    rsi
0x18d838641  57                              push    rdi
0x18d838642  4883ec58                        sub     rsp, 58h
0x18d838646  0f29742440                      movaps  [rsp+68h+var_28], xmm6
0x18d83864b  0f28f2                          movaps  xmm6, xmm2
0x18d83864e  4889d6                          mov     rsi, rdx
0x18d838651  4889cf                          mov     rdi, rcx
0x18d838654  0f57c0                          xorps   xmm0, xmm0
0x18d838657  0f29442420                      movaps  [rsp+68h+var_48], xmm0
0x18d83865c  ff15be26bdf7                    call    cs:qword_18540AD20
0x18d838662  c644242001                      mov     byte ptr [rsp+68h+var_48], 1
0x18d838667  f30f11742424                    movss   dword ptr [rsp+68h+var_48+4], xmm6
0x18d83866d  89442428                        mov     dword ptr [rsp+68h+var_48+8], eax
0x18d838671  4889742430                      mov     [rsp+68h+var_38], rsi
0x18d838676  488b4f10                        mov     rcx, [rdi+10h]
0x18d83867a  4885c9                          test    rcx, rcx
0x18d83867d  7416                            jz      short loc_18D838695
0x18d83867f  488d542420                      lea     rdx, [rsp+68h+var_48]
0x18d838684  e8e784920a                      call    sub_198160B70
0x18d838689  0f28742440                      movaps  xmm6, [rsp+68h+var_28]
0x18d83868e  4883c458                        add     rsp, 58h
0x18d838692  5f                              pop     rdi
0x18d838693  5e                              pop     rsi
0x18d838694  c3                              retn
0x18d838695  e8860a29f3                      call    sub_180AC9120
