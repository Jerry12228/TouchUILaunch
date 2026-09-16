0x1934d20e0  56                              push    rsi
0x1934d20e1  4883ec30                        sub     rsp, 30h
0x1934d20e5  803da7aaeff100                  cmp     cs:byte_1853CCB93, 0
0x1934d20ec  0f84bf000000                    jz      loc_1934D21B1
0x1934d20f2  488b0de7f2aef1                  mov     rcx, cs:qword_184FC13E0
0x1934d20f9  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x1934d2100  0f84d0000000                    jz      loc_1934D21D6
0x1934d2106  488b05f3b29ff1                  mov     rax, cs:qword_184ECD400
0x1934d210d  488b8068e40200                  mov     rax, [rax+2E468h]
0x1934d2114  4885c0                          test    rax, rax
0x1934d2117  0f84d5000000                    jz      loc_1934D21F2
0x1934d211d  488bb090000000                  mov     rsi, [rax+90h]
0x1934d2124  4885f6                          test    rsi, rsi
0x1934d2127  0f84ca000000                    jz      loc_1934D21F7
0x1934d212d  4c8b05a4c7b2f1                  mov     r8, cs:qword_184FFE8D8
0x1934d2134  488b06                          mov     rax, [rsi]
0x1934d2137  0fb788c6000000                  movzx   ecx, word ptr [rax+0C6h]
0x1934d213e  4885c9                          test    rcx, rcx
0x1934d2141  741c                            jz      short loc_1934D215F
0x1934d2143  488b5050                        mov     rdx, [rax+50h]
0x1934d2147  48c1e104                        shl     rcx, 4
0x1934d214b  4531c9                          xor     r9d, r9d
0x1934d214e  6690                            xchg    ax, ax
0x1934d2150  4e39040a                        cmp     [rdx+r9], r8
0x1934d2154  7425                            jz      short loc_1934D217B
0x1934d2156  4983c110                        add     r9, 10h
0x1934d215a  4c39c9                          cmp     rcx, r9
0x1934d215d  75f1                            jnz     short loc_1934D2150
0x1934d215f  488d4c2420                      lea     rcx, [rsp+38h+var_18]
0x1934d2164  4889f2                          mov     rdx, rsi
0x1934d2167  4531c9                          xor     r9d, r9d
0x1934d216a  e8a103dbec                      call    sub_180282510
0x1934d216f  4c8b442420                      mov     r8, [rsp+38h+var_18]
0x1934d2174  488b542428                      mov     rdx, [rsp+38h+var_10]
0x1934d2179  eb29                            jmp     short loc_1934D21A4
0x1934d217b  4a634c0a08                      movsxd  rcx, dword ptr [rdx+r9+8]
0x1934d2180  4c8b84c8d0000000                mov     r8, [rax+rcx*8+0D0h]
0x1934d2188  4c89442420                      mov     [rsp+38h+var_18], r8
0x1934d218d  0fb790c4000000                  movzx   edx, word ptr [rax+0C4h]
0x1934d2194  4801ca                          add     rdx, rcx
0x1934d2197  488b94d0d0000000                mov     rdx, [rax+rdx*8+0D0h]
0x1934d219f  4889542428                      mov     [rsp+38h+var_10], rdx
0x1934d21a4  4889f1                          mov     rcx, rsi
0x1934d21a7  41ffd0                          call    r8
0x1934d21aa  90                              nop
0x1934d21ab  4883c430                        add     rsp, 30h
0x1934d21af  5e                              pop     rsi
0x1934d21b0  c3                              retn
0x1934d21b1  b953a30300                      mov     ecx, 3A353h
0x1934d21b6  e8e5a5dbec                      call    sub_18028C7A0
0x1934d21bb  c605d1a9eff101                  mov     cs:byte_1853CCB93, 1
0x1934d21c2  488b0d17f2aef1                  mov     rcx, cs:qword_184FC13E0
0x1934d21c9  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x1934d21d0  0f8530ffffff                    jnz     loc_1934D2106
0x1934d21d6  e81530dbec                      call    sub_1802851F0
0x1934d21db  488b051eb29ff1                  mov     rax, cs:qword_184ECD400
0x1934d21e2  488b8068e40200                  mov     rax, [rax+2E468h]
0x1934d21e9  4885c0                          test    rax, rax
0x1934d21ec  0f852bffffff                    jnz     loc_1934D211D
0x1934d21f2  e8693146ed                      call    sub_180935360
0x1934d21f7  e8643146ed                      call    sub_180935360
