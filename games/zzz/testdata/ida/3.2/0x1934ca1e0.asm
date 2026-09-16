0x1934ca1e0  56                              push    rsi
0x1934ca1e1  4883ec30                        sub     rsp, 30h
0x1934ca1e5  803dd729f0f100                  cmp     cs:byte_1853CCBC3, 0
0x1934ca1ec  0f84bf000000                    jz      loc_1934CA2B1
0x1934ca1f2  488b0de771aff1                  mov     rcx, cs:qword_184FC13E0
0x1934ca1f9  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x1934ca200  0f84d0000000                    jz      loc_1934CA2D6
0x1934ca206  488b05f331a0f1                  mov     rax, cs:qword_184ECD400
0x1934ca20d  488b8068e40200                  mov     rax, [rax+2E468h]
0x1934ca214  4885c0                          test    rax, rax
0x1934ca217  0f84d5000000                    jz      loc_1934CA2F2
0x1934ca21d  488bb088000000                  mov     rsi, [rax+88h]
0x1934ca224  4885f6                          test    rsi, rsi
0x1934ca227  0f84ca000000                    jz      loc_1934CA2F7
0x1934ca22d  4c8b05a446b3f1                  mov     r8, cs:qword_184FFE8D8
0x1934ca234  488b06                          mov     rax, [rsi]
0x1934ca237  0fb788c6000000                  movzx   ecx, word ptr [rax+0C6h]
0x1934ca23e  4885c9                          test    rcx, rcx
0x1934ca241  741c                            jz      short loc_1934CA25F
0x1934ca243  488b5050                        mov     rdx, [rax+50h]
0x1934ca247  48c1e104                        shl     rcx, 4
0x1934ca24b  4531c9                          xor     r9d, r9d
0x1934ca24e  6690                            xchg    ax, ax
0x1934ca250  4e39040a                        cmp     [rdx+r9], r8
0x1934ca254  7425                            jz      short loc_1934CA27B
0x1934ca256  4983c110                        add     r9, 10h
0x1934ca25a  4c39c9                          cmp     rcx, r9
0x1934ca25d  75f1                            jnz     short loc_1934CA250
0x1934ca25f  488d4c2420                      lea     rcx, [rsp+38h+var_18]
0x1934ca264  4889f2                          mov     rdx, rsi
0x1934ca267  4531c9                          xor     r9d, r9d
0x1934ca26a  e8a182dbec                      call    sub_180282510
0x1934ca26f  4c8b442420                      mov     r8, [rsp+38h+var_18]
0x1934ca274  488b542428                      mov     rdx, [rsp+38h+var_10]
0x1934ca279  eb29                            jmp     short loc_1934CA2A4
0x1934ca27b  4a634c0a08                      movsxd  rcx, dword ptr [rdx+r9+8]
0x1934ca280  4c8b84c8d0000000                mov     r8, [rax+rcx*8+0D0h]
0x1934ca288  4c89442420                      mov     [rsp+38h+var_18], r8
0x1934ca28d  0fb790c4000000                  movzx   edx, word ptr [rax+0C4h]
0x1934ca294  4801ca                          add     rdx, rcx
0x1934ca297  488b94d0d0000000                mov     rdx, [rax+rdx*8+0D0h]
0x1934ca29f  4889542428                      mov     [rsp+38h+var_10], rdx
0x1934ca2a4  4889f1                          mov     rcx, rsi
0x1934ca2a7  41ffd0                          call    r8
0x1934ca2aa  90                              nop
0x1934ca2ab  4883c430                        add     rsp, 30h
0x1934ca2af  5e                              pop     rsi
0x1934ca2b0  c3                              retn
0x1934ca2b1  b983a30300                      mov     ecx, 3A383h
0x1934ca2b6  e8e524dcec                      call    sub_18028C7A0
0x1934ca2bb  c6050129f0f101                  mov     cs:byte_1853CCBC3, 1
0x1934ca2c2  488b0d1771aff1                  mov     rcx, cs:qword_184FC13E0
0x1934ca2c9  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x1934ca2d0  0f8530ffffff                    jnz     loc_1934CA206
0x1934ca2d6  e815afdbec                      call    sub_1802851F0
0x1934ca2db  488b051e31a0f1                  mov     rax, cs:qword_184ECD400
0x1934ca2e2  488b8068e40200                  mov     rax, [rax+2E468h]
0x1934ca2e9  4885c0                          test    rax, rax
0x1934ca2ec  0f852bffffff                    jnz     loc_1934CA21D
0x1934ca2f2  e869b046ed                      call    sub_180935360
0x1934ca2f7  e864b046ed                      call    sub_180935360
