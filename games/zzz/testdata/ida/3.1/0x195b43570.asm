0x195b43570  4883ec28                        sub     rsp, 28h
0x195b43574  803dc11fd5ef00                  cmp     cs:byte_18589553C, 0
0x195b4357b  744a                            jz      short loc_195B435C7
0x195b4357d  803dd47186ef00                  cmp     cs:byte_1853AA758, 0
0x195b43584  755b                            jnz     short loc_195B435E1
0x195b43586  488b0d23d990ef                  mov     rcx, cs:qword_185450EB0
0x195b4358d  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x195b43594  7466                            jz      short loc_195B435FC
0x195b43596  e8052d0000                      call    sub_195B462A0
0x195b4359b  488b0d0ed990ef                  mov     rcx, cs:qword_185450EB0
0x195b435a2  0fb691cb000000                  movzx   edx, byte ptr [rcx+0CBh]
0x195b435a9  85c0                            test    eax, eax
0x195b435ab  740d                            jz      short loc_195B435BA
0x195b435ad  84d2                            test    dl, dl
0x195b435af  7452                            jz      short loc_195B43603
0x195b435b1  4883c428                        add     rsp, 28h
0x195b435b5  e9e62c0000                      jmp     sub_195B462A0
0x195b435ba  84d2                            test    dl, dl
0x195b435bc  7454                            jz      short loc_195B43612
0x195b435be  4883c428                        add     rsp, 28h
0x195b435c2  e9b92b0000                      jmp     sub_195B46180
0x195b435c7  b94c3b0400                      mov     ecx, 43B4Ch
0x195b435cc  e80f5e73ea                      call    sub_1802793E0
0x195b435d1  c605641fd5ef01                  mov     cs:byte_18589553C, 1
0x195b435d8  803d797186ef00                  cmp     cs:byte_1853AA758, 0
0x195b435df  74a5                            jz      short loc_195B43586
0x195b435e1  b9a8d40000                      mov     ecx, 0D4A8h
0x195b435e6  e875a163fa                      call    sub_19017D760
0x195b435eb  4885c0                          test    rax, rax
0x195b435ee  7431                            jz      short loc_195B43621
0x195b435f0  4889c1                          mov     rcx, rax
0x195b435f3  4883c428                        add     rsp, 28h
0x195b435f7  e9f46967f5                      jmp     sub_18B1B9FF0
0x195b435fc  e84fe872ea                      call    sub_180271E50
0x195b43601  eb93                            jmp     short loc_195B43596
0x195b43603  e848e872ea                      call    sub_180271E50
0x195b43608  90                              nop
0x195b43609  4883c428                        add     rsp, 28h
0x195b4360d  e98e2c0000                      jmp     sub_195B462A0
0x195b43612  e839e872ea                      call    sub_180271E50
0x195b43617  90                              nop
0x195b43618  4883c428                        add     rsp, 28h
0x195b4361c  e95f2b0000                      jmp     sub_195B46180
0x195b43621  e8fa5af8ea                      call    sub_180AC9120
