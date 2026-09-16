0x195b462a0  56                              push    rsi
0x195b462a1  4883ec30                        sub     rsp, 30h
0x195b462a5  803d1ef2d4ef00                  cmp     cs:byte_1858954CA, 0
0x195b462ac  0f84bf000000                    jz      loc_195B46371
0x195b462b2  488b0df7ab90ef                  mov     rcx, cs:qword_185450EB0
0x195b462b9  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x195b462c0  0f84d0000000                    jz      loc_195B46396
0x195b462c6  488b0563a481ef                  mov     rax, cs:qword_185360730
0x195b462cd  488b80c0d90200                  mov     rax, [rax+2D9C0h]
0x195b462d4  4885c0                          test    rax, rax
0x195b462d7  0f84d5000000                    jz      loc_195B463B2
0x195b462dd  488bb080000000                  mov     rsi, [rax+80h]
0x195b462e4  4885f6                          test    rsi, rsi
0x195b462e7  0f84ca000000                    jz      loc_195B463B7
0x195b462ed  4c8b05ac8195ef                  mov     r8, cs:qword_18549E4A0
0x195b462f4  488b06                          mov     rax, [rsi]
0x195b462f7  0fb788c6000000                  movzx   ecx, word ptr [rax+0C6h]
0x195b462fe  4885c9                          test    rcx, rcx
0x195b46301  741c                            jz      short loc_195B4631F
0x195b46303  488b5010                        mov     rdx, [rax+10h]
0x195b46307  48c1e104                        shl     rcx, 4
0x195b4630b  4531c9                          xor     r9d, r9d
0x195b4630e  6690                            xchg    ax, ax
0x195b46310  4e39040a                        cmp     [rdx+r9], r8
0x195b46314  7425                            jz      short loc_195B4633B
0x195b46316  4983c110                        add     r9, 10h
0x195b4631a  4c39c9                          cmp     rcx, r9
0x195b4631d  75f1                            jnz     short loc_195B46310
0x195b4631f  488d4c2420                      lea     rcx, [rsp+38h+var_18]
0x195b46324  4889f2                          mov     rdx, rsi
0x195b46327  4531c9                          xor     r9d, r9d
0x195b4632a  e8418e72ea                      call    sub_18026F170
0x195b4632f  4c8b442420                      mov     r8, [rsp+38h+var_18]
0x195b46334  488b542428                      mov     rdx, [rsp+38h+var_10]
0x195b46339  eb29                            jmp     short loc_195B46364
0x195b4633b  4a634c0a08                      movsxd  rcx, dword ptr [rdx+r9+8]
0x195b46340  4c8b84c8d0000000                mov     r8, [rax+rcx*8+0D0h]
0x195b46348  4c89442420                      mov     [rsp+38h+var_18], r8
0x195b4634d  0fb790c2000000                  movzx   edx, word ptr [rax+0C2h]
0x195b46354  4801ca                          add     rdx, rcx
0x195b46357  488b94d0d0000000                mov     rdx, [rax+rdx*8+0D0h]
0x195b4635f  4889542428                      mov     [rsp+38h+var_10], rdx
0x195b46364  4889f1                          mov     rcx, rsi
0x195b46367  41ffd0                          call    r8
0x195b4636a  90                              nop
0x195b4636b  4883c430                        add     rsp, 30h
0x195b4636f  5e                              pop     rsi
0x195b46370  c3                              retn
0x195b46371  b9da3a0400                      mov     ecx, 43ADAh
0x195b46376  e8653073ea                      call    sub_1802793E0
0x195b4637b  c60548f1d4ef01                  mov     cs:byte_1858954CA, 1
0x195b46382  488b0d27ab90ef                  mov     rcx, cs:qword_185450EB0
0x195b46389  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x195b46390  0f8530ffffff                    jnz     loc_195B462C6
0x195b46396  e8b5ba72ea                      call    sub_180271E50
0x195b4639b  488b058ea381ef                  mov     rax, cs:qword_185360730
0x195b463a2  488b80c0d90200                  mov     rax, [rax+2D9C0h]
0x195b463a9  4885c0                          test    rax, rax
0x195b463ac  0f852bffffff                    jnz     loc_195B462DD
0x195b463b2  e8692df8ea                      call    sub_180AC9120
0x195b463b7  e8642df8ea                      call    sub_180AC9120
