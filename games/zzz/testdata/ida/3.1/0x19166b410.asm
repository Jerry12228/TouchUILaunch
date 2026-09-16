0x19166b410  56                              push    rsi
0x19166b411  57                              push    rdi
0x19166b412  4883ec28                        sub     rsp, 28h
0x19166b416  4889ce                          mov     rsi, rcx
0x19166b419  803de52dd9f300                  cmp     cs:byte_1853FE205, 0
0x19166b420  7516                            jnz     short loc_19166B438
0x19166b422  ff15f8f8d9f3                    call    cs:qword_18540AD20
0x19166b428  f30f2ac0                        cvtsi2ss xmm0, eax
0x19166b42c  f30f114634                      movss   dword ptr [rsi+34h], xmm0
0x19166b431  4883c428                        add     rsp, 28h
0x19166b435  5f                              pop     rdi
0x19166b436  5e                              pop     rsi
0x19166b437  c3                              retn
0x19166b438  4889d7                          mov     rdi, rdx
0x19166b43b  b9550f0600                      mov     ecx, 60F55h
0x19166b440  e81b23b1fe                      call    sub_19017D760
0x19166b445  4885c0                          test    rax, rax
0x19166b448  7414                            jz      short loc_19166B45E
0x19166b44a  4889c1                          mov     rcx, rax
0x19166b44d  4889f2                          mov     rdx, rsi
0x19166b450  4989f8                          mov     r8, rdi
0x19166b453  4883c428                        add     rsp, 28h
0x19166b457  5f                              pop     rdi
0x19166b458  5e                              pop     rsi
0x19166b459  e982ecb1f9                      jmp     sub_18B18A0E0
0x19166b45e  e8bddc45ef                      call    sub_180AC9120
