0x190c65030  56                              push    rsi
0x190c65031  4883ec20                        sub     rsp, 20h
0x190c65035  803dc8e176f400                  cmp     cs:byte_1853D3204, 0
0x190c6503c  7427                            jz      short loc_190C65065
0x190c6503e  488b0da37432f4                  mov     rcx, cs:qword_184F8C4E8
0x190c65045  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x190c6504c  7438                            jz      short loc_190C65086
0x190c6504e  ff15a43531f4                    call    cs:qword_184F785F8
0x190c65054  89c6                            mov     esi, eax
0x190c65056  ff15a43531f4                    call    cs:qword_184F78600
0x190c6505c  0fafc6                          imul    eax, esi
0x190c6505f  4883c420                        add     rsp, 20h
0x190c65063  5e                              pop     rsi
0x190c65064  c3                              retn
0x190c65065  b9c4090400                      mov     ecx, 409C4h
0x190c6506a  e8317762ef                      call    sub_18028C7A0
0x190c6506f  c6058ee176f401                  mov     cs:byte_1853D3204, 1
0x190c65076  488b0d6b7432f4                  mov     rcx, cs:qword_184F8C4E8
0x190c6507d  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x190c65084  75c8                            jnz     short loc_190C6504E
0x190c65086  e8650162ef                      call    sub_1802851F0
0x190c6508b  ebc1                            jmp     short loc_190C6504E
