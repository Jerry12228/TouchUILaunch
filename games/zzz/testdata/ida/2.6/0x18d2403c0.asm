0x18d2403c0  56                              push    rsi
0x18d2403c1  4883ec20                        sub     rsp, 20h
0x18d2403c5  89ce                            mov     esi, ecx
0x18d2403c7  803d10b9a8f700                  cmp     cs:byte_184CCBCDE, 0
0x18d2403ce  7430                            jz      short loc_18D240400
0x18d2403d0  803d9fd36bf700                  cmp     cs:byte_1848FD776, 0
0x18d2403d7  7541                            jnz     short loc_18D24041A
0x18d2403d9  85f6                            test    esi, esi
0x18d2403db  7517                            jnz     short loc_18D2403F4
0x18d2403dd  488b0dac0a71f7                  mov     rcx, cs:qword_184950E90
0x18d2403e4  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x18d2403eb  7463                            jz      short loc_18D240450
0x18d2403ed  e8ee37ecfd                      call    sub_18B103BE0
0x18d2403f2  89c6                            mov     esi, eax
0x18d2403f4  83fe01                          cmp     esi, 1
0x18d2403f7  0f94c0                          setz    al
0x18d2403fa  4883c420                        add     rsp, 20h
0x18d2403fe  5e                              pop     rsi
0x18d2403ff  c3                              retn
0x18d240400  b91eda0100                      mov     ecx, 1DA1Eh
0x18d240405  e826b8fff2                      call    sub_18023BC30
0x18d24040a  c605cdb8a8f701                  mov     cs:byte_184CCBCDE, 1
0x18d240411  803d5ed36bf700                  cmp     cs:byte_1848FD776, 0
0x18d240418  74bf                            jz      short loc_18D2403D9
0x18d24041a  488b0df74c74f7                  mov     rcx, cs:qword_184985118
0x18d240421  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x18d240428  742d                            jz      short loc_18D240457
0x18d24042a  488b052f5864f7                  mov     rax, cs:qword_184885C60
0x18d240431  488b8070e40300                  mov     rax, [rax+3E470h]
0x18d240438  488b88d0ff2000                  mov     rcx, [rax+20FFD0h]
0x18d24043f  4885c9                          test    rcx, rcx
0x18d240442  741a                            jz      short loc_18D24045E
0x18d240444  89f2                            mov     edx, esi
0x18d240446  4883c420                        add     rsp, 20h
0x18d24044a  5e                              pop     rsi
0x18d24044b  e9706fe1f9                      jmp     sub_1870573C0
0x18d240450  e81b43fff2                      call    sub_180234770
0x18d240455  eb96                            jmp     short loc_18D2403ED
0x18d240457  e81443fff2                      call    sub_180234770
0x18d24045c  ebcc                            jmp     short loc_18D24042A
0x18d24045e  e8fd9273f3                      call    sub_180979760
