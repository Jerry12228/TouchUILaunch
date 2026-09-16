0x18b103be0  4883ec28                        sub     rsp, 28h
0x18b103be4  803d6681bcf900                  cmp     cs:byte_184CCBD51, 0
0x18b103beb  744a                            jz      short loc_18B103C37
0x18b103bed  803d029c7ff900                  cmp     cs:byte_1848FD7F6, 0
0x18b103bf4  755b                            jnz     short loc_18B103C51
0x18b103bf6  488b0d93d284f9                  mov     rcx, cs:qword_184950E90
0x18b103bfd  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x18b103c04  747e                            jz      short loc_18B103C84
0x18b103c06  e8f5970000                      call    sub_18B10D400
0x18b103c0b  488b0d7ed284f9                  mov     rcx, cs:qword_184950E90
0x18b103c12  0fb691cb000000                  movzx   edx, byte ptr [rcx+0CBh]
0x18b103c19  85c0                            test    eax, eax
0x18b103c1b  740d                            jz      short loc_18B103C2A
0x18b103c1d  84d2                            test    dl, dl
0x18b103c1f  746d                            jz      short loc_18B103C8E
0x18b103c21  4883c428                        add     rsp, 28h
0x18b103c25  e9d6970000                      jmp     sub_18B10D400
0x18b103c2a  84d2                            test    dl, dl
0x18b103c2c  746f                            jz      short loc_18B103C9D
0x18b103c2e  4883c428                        add     rsp, 28h
0x18b103c32  e9a9500000                      jmp     sub_18B108CE0
0x18b103c37  b991da0100                      mov     ecx, 1DA91h
0x18b103c3c  e8ef7f13f5                      call    sub_18023BC30
0x18b103c41  c6050981bcf901                  mov     cs:byte_184CCBD51, 1
0x18b103c48  803da79b7ff900                  cmp     cs:byte_1848FD7F6, 0
0x18b103c4f  74a5                            jz      short loc_18B103BF6
0x18b103c51  488b0dc01488f9                  mov     rcx, cs:qword_184985118
0x18b103c58  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x18b103c5f  744b                            jz      short loc_18B103CAC
0x18b103c61  488b05f81f78f9                  mov     rax, cs:qword_184885C60
0x18b103c68  488b8070e40300                  mov     rax, [rax+3E470h]
0x18b103c6f  488b88d0032100                  mov     rcx, [rax+2103D0h]
0x18b103c76  4885c9                          test    rcx, rcx
0x18b103c79  7438                            jz      short loc_18B103CB3
0x18b103c7b  4883c428                        add     rsp, 28h
0x18b103c7f  e92c90f5fb                      jmp     sub_18705CCB0
0x18b103c84  e8e70a13f5                      call    sub_180234770
0x18b103c89  e978ffffff                      jmp     loc_18B103C06
0x18b103c8e  e8dd0a13f5                      call    sub_180234770
0x18b103c93  90                              nop
0x18b103c94  4883c428                        add     rsp, 28h
0x18b103c98  e963970000                      jmp     sub_18B10D400
0x18b103c9d  e8ce0a13f5                      call    sub_180234770
0x18b103ca2  90                              nop
0x18b103ca3  4883c428                        add     rsp, 28h
0x18b103ca7  e934500000                      jmp     sub_18B108CE0
0x18b103cac  e8bf0a13f5                      call    sub_180234770
0x18b103cb1  ebae                            jmp     short loc_18B103C61
0x18b103cb3  e8a85a87f5                      call    sub_180979760
