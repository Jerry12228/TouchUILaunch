0x18b506910  4157                            push    r15
0x18b506912  4156                            push    r14
0x18b506914  4155                            push    r13
0x18b506916  4154                            push    r12
0x18b506918  56                              push    rsi
0x18b506919  57                              push    rdi
0x18b50691a  55                              push    rbp
0x18b50691b  53                              push    rbx
0x18b50691c  4881ece8000000                  sub     rsp, 0E8h
0x18b506923  0f29bc24d0000000                movaps  [rsp+128h+var_58], xmm7
0x18b50692b  0f29b424c0000000                movaps  [rsp+128h+var_68], xmm6
0x18b506933  4989cc                          mov     r12, rcx
0x18b506936  803d3c41e1f900                  cmp     cs:byte_18531AA79, 0
0x18b50693d  0f84c6040000                    jz      loc_18B506E09
0x18b506943  48c744246800000000              mov     [rsp+128h+var_C0], 0
0x18b50694c  803d8b67a0f900                  cmp     cs:byte_184F0D0DE, 0
0x18b506953  0f85d7040000                    jnz     loc_18B506E30
0x18b506959  488b0d4016a8f9                  mov     rcx, cs:qword_184F87FA0
0x18b506960  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x18b506967  0f8420050000                    jz      loc_18B506E8D
0x18b50696d  31ff                            xor     edi, edi
0x18b50696f  31c9                            xor     ecx, ecx
0x18b506971  e84a9536fc                      call    sub_18786FEC0
0x18b506976  be00000000                      mov     esi, 0
0x18b50697b  84c0                            test    al, al
0x18b50697d  0f840b020000                    jz      loc_18B506B8E
0x18b506983  803de531a0f900                  cmp     cs:byte_184F09B6F, 0
0x18b50698a  0f8507050000                    jnz     loc_18B506E97
0x18b506990  31c9                            xor     ecx, ecx
0x18b506992  ff1598d5a3f9                    call    cs:qword_184F43F30
0x18b506998  84c0                            test    al, al
0x18b50699a  753c                            jnz     short loc_18B5069D8
0x18b50699c  803dcc31a0f900                  cmp     cs:byte_184F09B6F, 0
0x18b5069a3  0f85e0050000                    jnz     loc_18B506F89
0x18b5069a9  b901000000                      mov     ecx, 1
0x18b5069ae  ff157cd5a3f9                    call    cs:qword_184F43F30
0x18b5069b4  84c0                            test    al, al
0x18b5069b6  7520                            jnz     short loc_18B5069D8
0x18b5069b8  803db031a0f900                  cmp     cs:byte_184F09B6F, 0
0x18b5069bf  0f850d060000                    jnz     loc_18B506FD2
0x18b5069c5  b902000000                      mov     ecx, 2
0x18b5069ca  ff1560d5a3f9                    call    cs:qword_184F43F30
0x18b5069d0  84c0                            test    al, al
0x18b5069d2  0f843e060000                    jz      loc_18B507016
0x18b5069d8  41807c243800                    cmp     byte ptr [r12+38h], 0
0x18b5069de  0f84d3000000                    jz      loc_18B506AB7
0x18b5069e4  c744242800000000                mov     dword ptr [rsp+128h+var_108+8], 0
0x18b5069ec  48c744242000000000              mov     qword ptr [rsp+128h+var_108], 0
0x18b5069f5  488d4c2420                      lea     rcx, [rsp+128h+var_108]
0x18b5069fa  ff15a8d5a3f9                    call    cs:qword_184F43FA8
0x18b506a00  f30f107c2420                    movss   xmm7, dword ptr [rsp+128h+var_108]
0x18b506a06  f30f10742424                    movss   xmm6, dword ptr [rsp+128h+var_108+4]
0x18b506a0c  488b0d5587a7f9                  mov     rcx, cs:qword_184F7F168
0x18b506a13  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x18b506a1a  0f84ef040000                    jz      loc_18B506F0F
0x18b506a20  498b442410                      mov     rax, [r12+10h]
0x18b506a25  4885c0                          test    rax, rax
0x18b506a28  0f84f4040000                    jz      loc_18B506F22
0x18b506a2e  83781800                        cmp     dword ptr [rax+18h], 0
0x18b506a32  0f84ef040000                    jz      loc_18B506F27
0x18b506a38  f30f5c7028                      subss   xmm6, dword ptr [rax+28h]
0x18b506a3d  f30f5c7824                      subss   xmm7, dword ptr [rax+24h]
0x18b506a42  660f7efb                        movd    ebx, xmm7
0x18b506a46  660f7ef0                        movd    eax, xmm6
0x18b506a4a  4889c1                          mov     rcx, rax
0x18b506a4d  48c1e120                        shl     rcx, 20h
0x18b506a51  4809cb                          or      rbx, rcx
0x18b506a54  803d8066a0f900                  cmp     cs:byte_184F0D0DB, 0
0x18b506a5b  0f85d5040000                    jnz     loc_18B506F36
0x18b506a61  b9000080bf                      mov     ecx, 0BF800000h
0x18b506a66  f30f1005063218f7                movss   xmm0, cs:dword_182689C74
0x18b506a6e  0f2ec7                          ucomiss xmm0, xmm7
0x18b506a71  ba000080bf                      mov     edx, 0BF800000h
0x18b506a76  7711                            ja      short loc_18B506A89
0x18b506a78  0f2e3d2daa09f7                  ucomiss xmm7, cs:dword_1825A14AC
0x18b506a7f  4889da                          mov     rdx, rbx
0x18b506a82  7605                            jbe     short loc_18B506A89
0x18b506a84  ba0000803f                      mov     edx, 3F800000h
0x18b506a89  0f2ec6                          ucomiss xmm0, xmm6
0x18b506a8c  7711                            ja      short loc_18B506A9F
0x18b506a8e  0f2e3517aa09f7                  ucomiss xmm6, cs:dword_1825A14AC
0x18b506a95  4889c1                          mov     rcx, rax
0x18b506a98  7605                            jbe     short loc_18B506A9F
0x18b506a9a  b90000803f                      mov     ecx, 3F800000h
0x18b506a9f  48c1e120                        shl     rcx, 20h
0x18b506aa3  89d3                            mov     ebx, edx
0x18b506aa5  4809cb                          or      rbx, rcx
0x18b506aa8  498b742410                      mov     rsi, [r12+10h]
0x18b506aad  4885f6                          test    rsi, rsi
0x18b506ab0  751a                            jnz     short loc_18B506ACC
0x18b506ab2  e9cd040000                      jmp     loc_18B506F84
0x18b506ab7  488b1dd2f198f9                  mov     rbx, cs:qword_184E95C90
0x18b506abe  498b742410                      mov     rsi, [r12+10h]
0x18b506ac3  4885f6                          test    rsi, rsi
0x18b506ac6  0f84b8040000                    jz      loc_18B506F84
0x18b506acc  837e1800                        cmp     dword ptr [rsi+18h], 0
0x18b506ad0  0f8407040000                    jz      loc_18B506EDD
0x18b506ad6  c7462004ffffff                  mov     dword ptr [rsi+20h], 0FFFFFF04h
0x18b506add  c744242800000000                mov     dword ptr [rsp+128h+var_108+8], 0
0x18b506ae5  48c744242000000000              mov     qword ptr [rsp+128h+var_108], 0
0x18b506aee  488d4c2420                      lea     rcx, [rsp+128h+var_108]
0x18b506af3  ff15afd4a3f9                    call    cs:qword_184F43FA8
0x18b506af9  837e1800                        cmp     dword ptr [rsi+18h], 0
0x18b506afd  0f84e9030000                    jz      loc_18B506EEC
0x18b506b03  8b442420                        mov     eax, dword ptr [rsp+128h+var_108]
0x18b506b07  8b4c2424                        mov     ecx, dword ptr [rsp+128h+var_108+4]
0x18b506b0b  48c1e120                        shl     rcx, 20h
0x18b506b0f  4809c1                          or      rcx, rax
0x18b506b12  48894e24                        mov     [rsi+24h], rcx
0x18b506b16  498b6c2418                      mov     rbp, [r12+18h]
0x18b506b1b  4885ed                          test    rbp, rbp
0x18b506b1e  743d                            jz      short loc_18B506B5D
0x18b506b20  41807c243800                    cmp     byte ptr [r12+38h], 0
0x18b506b26  7535                            jnz     short loc_18B506B5D
0x18b506b28  c744242800000000                mov     dword ptr [rsp+128h+var_108+8], 0
0x18b506b30  48c744242000000000              mov     qword ptr [rsp+128h+var_108], 0
0x18b506b39  488d4c2420                      lea     rcx, [rsp+128h+var_108]
0x18b506b3e  ff1564d4a3f9                    call    cs:qword_184F43FA8
0x18b506b44  4c8b442420                      mov     r8, qword ptr [rsp+128h+var_108]
0x18b506b49  4c8b0d18c1c3f9                  mov     r9, cs:qword_185142C68
0x18b506b50  4889e9                          mov     rcx, rbp
0x18b506b53  ba04ffffff                      mov     edx, 0FFFFFF04h
0x18b506b58  e8832b1f04                      call    sub_18F6F96E0
0x18b506b5d  498b4c2430                      mov     rcx, [r12+30h]
0x18b506b62  4885c9                          test    rcx, rcx
0x18b506b65  741c                            jz      short loc_18B506B83
0x18b506b67  41807c243800                    cmp     byte ptr [r12+38h], 0
0x18b506b6d  7414                            jz      short loc_18B506B83
0x18b506b6f  4c8b0df2c0c3f9                  mov     r9, cs:qword_185142C68
0x18b506b76  ba04ffffff                      mov     edx, 0FFFFFF04h
0x18b506b7b  4989d8                          mov     r8, rbx
0x18b506b7e  e85d2b1f04                      call    sub_18F6F96E0
0x18b506b83  41c644243801                    mov     byte ptr [r12+38h], 1
0x18b506b89  be01000000                      mov     esi, 1
0x18b506b8e  0f57f6                          xorps   xmm6, xmm6
0x18b506b91  488d5c2420                      lea     rbx, [rsp+128h+var_108]
0x18b506b96  4c8d6c2470                      lea     r13, [rsp+128h+var_B8]
0x18b506b9b  4c8d742468                      lea     r14, [rsp+128h+var_C0]
0x18b506ba0  4c8d7c2478                      lea     r15, [rsp+128h+var_B0]
0x18b506ba5  eb0b                            jmp     short loc_18B506BB2
0x18b506bb0  ffc7                            inc     edi
0x18b506bb2  803db82fa0f900                  cmp     cs:byte_184F09B71, 0
0x18b506bb9  0f85dd000000                    jnz     loc_18B506C9C
0x18b506bbf  ff15bbd3a3f9                    call    cs:qword_184F43F80
0x18b506bc5  39c7                            cmp     edi, eax
0x18b506bc7  0f8db4010000                    jge     loc_18B506D81
0x18b506bcd  803d9e2fa0f900                  cmp     cs:byte_184F09B72, 0
0x18b506bd4  0f8506010000                    jnz     loc_18B506CE0
0x18b506bda  0f29742450                      movaps  [rsp+128h+var_D8], xmm6
0x18b506bdf  0f29742440                      movaps  [rsp+128h+var_E8], xmm6
0x18b506be4  0f29742430                      movaps  [rsp+128h+var_F8], xmm6
0x18b506be9  0f29742420                      movaps  [rsp+128h+var_108], xmm6
0x18b506bee  c744246000000000                mov     [rsp+128h+var_C8], 0
0x18b506bf6  89f9                            mov     ecx, edi
0x18b506bf8  4889da                          mov     rdx, rbx
0x18b506bfb  ff159fd3a3f9                    call    cs:qword_184F43FA0
0x18b506c01  8b6c2420                        mov     ebp, dword ptr [rsp+128h+var_108]
0x18b506c05  488b442424                      mov     rax, qword ptr [rsp+128h+var_108+4]
0x18b506c0a  4889442470                      mov     [rsp+128h+var_B8], rax
0x18b506c0f  8b442444                        mov     eax, dword ptr [rsp+128h+var_E8+4]
0x18b506c13  4c89e9                          mov     rcx, r13
0x18b506c16  4c8b01                          mov     r8, [rcx]
0x18b506c19  83f803                          cmp     eax, 3
0x18b506c1c  0f831e010000                    jnb     loc_18B506D40
0x18b506c22  498b4c2410                      mov     rcx, [r12+10h]
0x18b506c27  4885c9                          test    rcx, rcx
0x18b506c2a  0f84c5010000                    jz      loc_18B506DF5
0x18b506c30  3b7118                          cmp     esi, [rcx+18h]
0x18b506c33  0f83c1010000                    jnb     loc_18B506DFA
0x18b506c39  4863d6                          movsxd  rdx, esi
0x18b506c3c  488d1452                        lea     rdx, [rdx+rdx*2]
0x18b506c40  896c9120                        mov     [rcx+rdx*4+20h], ebp
0x18b506c44  4c89449124                      mov     [rcx+rdx*4+24h], r8
0x18b506c49  83f801                          cmp     eax, 1
0x18b506c4c  7410                            jz      short loc_18B506C5E
0x18b506c4e  85c0                            test    eax, eax
0x18b506c50  753a                            jnz     short loc_18B506C8C
0x18b506c52  498b4c2418                      mov     rcx, [r12+18h]
0x18b506c57  4885c9                          test    rcx, rcx
0x18b506c5a  7522                            jnz     short loc_18B506C7E
0x18b506c5c  eb2e                            jmp     short loc_18B506C8C
0x18b506c5e  4c89e1                          mov     rcx, r12
0x18b506c61  89ea                            mov     edx, ebp
0x18b506c63  4d89f1                          mov     r9, r14
0x18b506c66  e8f5f7ffff                      call    sub_18B506460
0x18b506c6b  84c0                            test    al, al
0x18b506c6d  741d                            jz      short loc_18B506C8C
0x18b506c6f  498b4c2430                      mov     rcx, [r12+30h]
0x18b506c74  4885c9                          test    rcx, rcx
0x18b506c77  7413                            jz      short loc_18B506C8C
0x18b506c79  4c8b442468                      mov     r8, [rsp+128h+var_C0]
0x18b506c7e  4c8b0de3bfc3f9                  mov     r9, cs:qword_185142C68
0x18b506c85  89ea                            mov     edx, ebp
0x18b506c87  e8542a1f04                      call    sub_18F6F96E0
0x18b506c8c  83fe0a                          cmp     esi, 0Ah
0x18b506c8f  0f8d17010000                    jge     loc_18B506DAC
0x18b506c95  ffc6                            inc     esi
0x18b506c97  e914ffffff                      jmp     loc_18B506BB0
0x18b506c9c  488b0d9d39abf9                  mov     rcx, cs:qword_184FBA640
0x18b506ca3  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x18b506caa  0f84bd000000                    jz      loc_18B506D6D
0x18b506cb0  488b0591ce98f9                  mov     rax, cs:qword_184E93B48
0x18b506cb7  488b80a0240500                  mov     rax, [rax+524A0h]
0x18b506cbe  488b88a81d0f00                  mov     rcx, [rax+0F1DA8h]
0x18b506cc5  4885c9                          test    rcx, rcx
0x18b506cc8  0f8477040000                    jz      loc_18B507145
0x18b506cce  e8adf88efc                      call    sub_187DF6580
0x18b506cd3  39c7                            cmp     edi, eax
0x18b506cd5  0f8cf2feffff                    jl      loc_18B506BCD
0x18b506cdb  e9a1000000                      jmp     loc_18B506D81
0x18b506ce0  488b0d5939abf9                  mov     rcx, cs:qword_184FBA640
0x18b506ce7  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x18b506cee  0f8483000000                    jz      loc_18B506D77
0x18b506cf4  488b054dce98f9                  mov     rax, cs:qword_184E93B48
0x18b506cfb  488b80a0240500                  mov     rax, [rax+524A0h]
0x18b506d02  488b90b01d0f00                  mov     rdx, [rax+0F1DB0h]
0x18b506d09  4885d2                          test    rdx, rdx
0x18b506d0c  0f8438040000                    jz      loc_18B50714A
0x18b506d12  4c89f9                          mov     rcx, r15
0x18b506d15  4189f8                          mov     r8d, edi
0x18b506d18  e8033db809                      call    sub_19508AA20
0x18b506d1d  8b6c2478                        mov     ebp, [rsp+128h+var_B0]
0x18b506d21  8b84249c000000                  mov     eax, [rsp+128h+var_8C]
0x18b506d28  488d4c247c                      lea     rcx, [rsp+128h+var_AC]
0x18b506d2d  4c8b01                          mov     r8, [rcx]
0x18b506d30  83f803                          cmp     eax, 3
0x18b506d33  0f82e9feffff                    jb      loc_18B506C22
0x18b506d39  0f1f8000000000                  nop     dword ptr [rax+00000000h]
0x18b506d40  83c0fd                          add     eax, 0FFFFFFFDh
0x18b506d43  83f802                          cmp     eax, 2
0x18b506d46  0f8364feffff                    jnb     loc_18B506BB0
0x18b506d4c  498b4c2420                      mov     rcx, [r12+20h]
0x18b506d51  4885c9                          test    rcx, rcx
0x18b506d54  0f8456feffff                    jz      loc_18B506BB0
0x18b506d5a  4c8b0d07bfc3f9                  mov     r9, cs:qword_185142C68
0x18b506d61  89ea                            mov     edx, ebp
0x18b506d63  e878291f04                      call    sub_18F6F96E0
0x18b506d68  e943feffff                      jmp     loc_18B506BB0
0x18b506d6d  e87ed1d3f4                      call    sub_180243EF0
0x18b506d72  e939ffffff                      jmp     loc_18B506CB0
0x18b506d77  e874d1d3f4                      call    sub_180243EF0
0x18b506d7c  e973ffffff                      jmp     loc_18B506CF4
0x18b506d81  83fe0a                          cmp     esi, 0Ah
0x18b506d84  7f26                            jg      short loc_18B506DAC
0x18b506d86  498b442410                      mov     rax, [r12+10h]
0x18b506d8b  4885c0                          test    rax, rax
0x18b506d8e  0f8467010000                    jz      loc_18B506EFB
0x18b506d94  3b7018                          cmp     esi, [rax+18h]
0x18b506d97  0f8363010000                    jnb     loc_18B506F00
0x18b506d9d  4863ce                          movsxd  rcx, esi
0x18b506da0  488d0c49                        lea     rcx, [rcx+rcx*2]
0x18b506da4  c744882003ffffff                mov     dword ptr [rax+rcx*4+20h], 0FFFFFF03h
0x18b506dac  498b4c2410                      mov     rcx, [r12+10h]
0x18b506db1  498b542428                      mov     rdx, [r12+28h]
0x18b506db6  41b80b000000                    mov     r8d, 0Bh
0x18b506dbc  e8dff75a0f                      call    sub_19AAB65A0
0x18b506dc1  41807c243900                    cmp     byte ptr [r12+39h], 0
0x18b506dc7  7408                            jz      short loc_18B506DD1
0x18b506dc9  4c89e1                          mov     rcx, r12
0x18b506dcc  e83ff2ffff                      call    sub_18B506010
0x18b506dd1  0f28b424c0000000                movaps  xmm6, [rsp+128h+var_68]
0x18b506dd9  0f28bc24d0000000                movaps  xmm7, [rsp+128h+var_58]
0x18b506de1  4881c4e8000000                  add     rsp, 0E8h
0x18b506de8  5b                              pop     rbx
0x18b506de9  5d                              pop     rbp
0x18b506dea  5f                              pop     rdi
0x18b506deb  5e                              pop     rsi
0x18b506dec  415c                            pop     r12
0x18b506dee  415d                            pop     r13
0x18b506df0  415e                            pop     r14
0x18b506df2  415f                            pop     r15
0x18b506df4  c3                              retn
0x18b506df5  e846a664f5                      call    sub_180B51440
0x18b506dfa  e8f1c8d3f4                      call    sub_1802436F0
0x18b506dff  4889c1                          mov     rcx, rax
0x18b506e02  31d2                            xor     edx, edx
0x18b506e04  e8d7a564f5                      call    sub_180B513E0
0x18b506e09  b9a9830300                      mov     ecx, 383A9h
0x18b506e0e  e83d45d4f4                      call    sub_18024B350
0x18b506e13  c6055f3ce1f901                  mov     cs:byte_18531AA79, 1
0x18b506e1a  48c744246800000000              mov     [rsp+128h+var_C0], 0
0x18b506e23  803db462a0f900                  cmp     cs:byte_184F0D0DE, 0
0x18b506e2a  0f8429fbffff                    jz      loc_18B506959
0x18b506e30  488b0d0938abf9                  mov     rcx, cs:qword_184FBA640
0x18b506e37  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x18b506e3e  0f840b030000                    jz      loc_18B50714F
0x18b506e44  488b05fdcc98f9                  mov     rax, cs:qword_184E93B48
0x18b506e4b  488b80a0240500                  mov     rax, [rax+524A0h]
0x18b506e52  488b8810c91000                  mov     rcx, [rax+10C910h]
0x18b506e59  4885c9                          test    rcx, rcx
0x18b506e5c  0f84f7020000                    jz      loc_18B507159
0x18b506e62  4c89e2                          mov     rdx, r12
0x18b506e65  0f28b424c0000000                movaps  xmm6, [rsp+128h+var_68]
0x18b506e6d  0f28bc24d0000000                movaps  xmm7, [rsp+128h+var_58]
0x18b506e75  4881c4e8000000                  add     rsp, 0E8h
0x18b506e7c  5b                              pop     rbx
0x18b506e7d  5d                              pop     rbp
0x18b506e7e  5f                              pop     rdi
0x18b506e7f  5e                              pop     rsi
0x18b506e80  415c                            pop     r12
0x18b506e82  415d                            pop     r13
0x18b506e84  415e                            pop     r14
0x18b506e86  415f                            pop     r15
0x18b506e88  e9b3428afc                      jmp     sub_187DAB140
0x18b506e8d  e85ed0d3f4                      call    sub_180243EF0
0x18b506e92  e9d6faffff                      jmp     loc_18B50696D
0x18b506e97  488b0da237abf9                  mov     rcx, cs:qword_184FBA640
0x18b506e9e  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x18b506ea5  0f84b3020000                    jz      loc_18B50715E
0x18b506eab  488b0596cc98f9                  mov     rax, cs:qword_184E93B48
0x18b506eb2  488b80a0240500                  mov     rax, [rax+524A0h]
0x18b506eb9  488b88981d0f00                  mov     rcx, [rax+0F1D98h]
0x18b506ec0  4885c9                          test    rcx, rcx
0x18b506ec3  0f849f020000                    jz      loc_18B507168
0x18b506ec9  31d2                            xor     edx, edx
0x18b506ecb  e830a88efc                      call    sub_187DF1700
0x18b506ed0  84c0                            test    al, al
0x18b506ed2  0f84c4faffff                    jz      loc_18B50699C
0x18b506ed8  e9fbfaffff                      jmp     loc_18B5069D8
0x18b506edd  e80ec8d3f4                      call    sub_1802436F0
0x18b506ee2  4889c1                          mov     rcx, rax
0x18b506ee5  31d2                            xor     edx, edx
0x18b506ee7  e8f4a464f5                      call    sub_180B513E0
0x18b506eec  e8ffc7d3f4                      call    sub_1802436F0
0x18b506ef1  4889c1                          mov     rcx, rax
0x18b506ef4  31d2                            xor     edx, edx
0x18b506ef6  e8e5a464f5                      call    sub_180B513E0
0x18b506efb  e840a564f5                      call    sub_180B51440
0x18b506f00  e8ebc7d3f4                      call    sub_1802436F0
0x18b506f05  4889c1                          mov     rcx, rax
0x18b506f08  31d2                            xor     edx, edx
0x18b506f0a  e8d1a464f5                      call    sub_180B513E0
0x18b506f0f  e8dccfd3f4                      call    sub_180243EF0
0x18b506f14  498b442410                      mov     rax, [r12+10h]
0x18b506f19  4885c0                          test    rax, rax
0x18b506f1c  0f850cfbffff                    jnz     loc_18B506A2E
0x18b506f22  e819a564f5                      call    sub_180B51440
0x18b506f27  e8c4c7d3f4                      call    sub_1802436F0
0x18b506f2c  4889c1                          mov     rcx, rax
0x18b506f2f  31d2                            xor     edx, edx
0x18b506f31  e8aaa464f5                      call    sub_180B513E0
0x18b506f36  488b0d0337abf9                  mov     rcx, cs:qword_184FBA640
0x18b506f3d  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x18b506f44  0f8423020000                    jz      loc_18B50716D
0x18b506f4a  488b05f7cb98f9                  mov     rax, cs:qword_184E93B48
0x18b506f51  488b80a0240500                  mov     rax, [rax+524A0h]
0x18b506f58  488b88f8c81000                  mov     rcx, [rax+10C8F8h]
0x18b506f5f  4885c9                          test    rcx, rcx
0x18b506f62  0f840f020000                    jz      loc_18B507177
0x18b506f68  4c89e2                          mov     rdx, r12
0x18b506f6b  4989d8                          mov     r8, rbx
0x18b506f6e  e85dd2a208                      call    sub_193F341D0
0x18b506f73  4889c3                          mov     rbx, rax
0x18b506f76  498b742410                      mov     rsi, [r12+10h]
0x18b506f7b  4885f6                          test    rsi, rsi
0x18b506f7e  0f8548fbffff                    jnz     loc_18B506ACC
0x18b506f84  e8b7a464f5                      call    sub_180B51440
0x18b506f89  488b0db036abf9                  mov     rcx, cs:qword_184FBA640
0x18b506f90  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x18b506f97  0f84df010000                    jz      loc_18B50717C
0x18b506f9d  488b05a4cb98f9                  mov     rax, cs:qword_184E93B48
0x18b506fa4  488b80a0240500                  mov     rax, [rax+524A0h]
0x18b506fab  488b88981d0f00                  mov     rcx, [rax+0F1D98h]
0x18b506fb2  4885c9                          test    rcx, rcx
0x18b506fb5  0f84cb010000                    jz      loc_18B507186
0x18b506fbb  ba01000000                      mov     edx, 1
0x18b506fc0  e83ba78efc                      call    sub_187DF1700
0x18b506fc5  84c0                            test    al, al
0x18b506fc7  0f84ebf9ffff                    jz      loc_18B5069B8
0x18b506fcd  e906faffff                      jmp     loc_18B5069D8
0x18b506fd2  488b0d6736abf9                  mov     rcx, cs:qword_184FBA640
0x18b506fd9  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x18b506fe0  0f84a5010000                    jz      loc_18B50718B
0x18b506fe6  488b055bcb98f9                  mov     rax, cs:qword_184E93B48
0x18b506fed  488b80a0240500                  mov     rax, [rax+524A0h]
0x18b506ff4  488b88981d0f00                  mov     rcx, [rax+0F1D98h]
0x18b506ffb  4885c9                          test    rcx, rcx
0x18b506ffe  0f8491010000                    jz      loc_18B507195
0x18b507004  ba02000000                      mov     edx, 2
0x18b507009  e8f2a68efc                      call    sub_187DF1700
0x18b50700e  84c0                            test    al, al
0x18b507010  0f85c2f9ffff                    jnz     loc_18B5069D8
0x18b507016  803d502ba0f900                  cmp     cs:byte_184F09B6D, 0
0x18b50701d  0f8593000000                    jnz     loc_18B5070B6
0x18b507023  31c9                            xor     ecx, ecx
0x18b507025  ff1515cfa3f9                    call    cs:qword_184F43F40
0x18b50702b  84c0                            test    al, al
0x18b50702d  752a                            jnz     short loc_18B507059
0x18b50702f  803d372ba0f900                  cmp     cs:byte_184F09B6D, 0
0x18b507036  0f85c0000000                    jnz     loc_18B5070FC
0x18b50703c  b901000000                      mov     ecx, 1
0x18b507041  ff15f9cea3f9                    call    cs:qword_184F43F40
0x18b507047  84c0                            test    al, al
0x18b507049  750e                            jnz     short loc_18B507059
0x18b50704b  b902000000                      mov     ecx, 2
0x18b507050  e84bfe68fc                      call    sub_187B96EA0
0x18b507055  84c0                            test    al, al
0x18b507057  7455                            jz      short loc_18B5070AE
0x18b507059  41c644243800                    mov     byte ptr [r12+38h], 0
0x18b50705f  498b5c2420                      mov     rbx, [r12+20h]
0x18b507064  be00000000                      mov     esi, 0
0x18b507069  4885db                          test    rbx, rbx
0x18b50706c  0f841cfbffff                    jz      loc_18B506B8E
0x18b507072  c744242800000000                mov     dword ptr [rsp+128h+var_108+8], 0
0x18b50707a  48c744242000000000              mov     qword ptr [rsp+128h+var_108], 0
0x18b507083  488d4c2420                      lea     rcx, [rsp+128h+var_108]
0x18b507088  ff151acfa3f9                    call    cs:qword_184F43FA8
0x18b50708e  4c8b442420                      mov     r8, qword ptr [rsp+128h+var_108]
0x18b507093  4c8b0dcebbc3f9                  mov     r9, cs:qword_185142C68
0x18b50709a  4889d9                          mov     rcx, rbx
0x18b50709d  ba04ffffff                      mov     edx, 0FFFFFF04h
0x18b5070a2  e839261f04                      call    sub_18F6F96E0
0x18b5070a7  31f6                            xor     esi, esi
0x18b5070a9  e9e0faffff                      jmp     loc_18B506B8E
0x18b5070ae  41c644243800                    mov     byte ptr [r12+38h], 0
0x18b5070b4  ebf1                            jmp     short loc_18B5070A7
0x18b5070b6  488b0d8335abf9                  mov     rcx, cs:qword_184FBA640
0x18b5070bd  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x18b5070c4  0f84d0000000                    jz      loc_18B50719A
0x18b5070ca  488b0577ca98f9                  mov     rax, cs:qword_184E93B48
0x18b5070d1  488b80a0240500                  mov     rax, [rax+524A0h]
0x18b5070d8  488b88881d0f00                  mov     rcx, [rax+0F1D88h]
0x18b5070df  4885c9                          test    rcx, rcx
0x18b5070e2  0f84bc000000                    jz      loc_18B5071A4
0x18b5070e8  31d2                            xor     edx, edx
0x18b5070ea  e811a68efc                      call    sub_187DF1700
0x18b5070ef  84c0                            test    al, al
0x18b5070f1  0f8438ffffff                    jz      loc_18B50702F
0x18b5070f7  e95dffffff                      jmp     loc_18B507059
0x18b5070fc  488b0d3d35abf9                  mov     rcx, cs:qword_184FBA640
0x18b507103  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x18b50710a  0f8499000000                    jz      loc_18B5071A9
0x18b507110  488b0531ca98f9                  mov     rax, cs:qword_184E93B48
0x18b507117  488b80a0240500                  mov     rax, [rax+524A0h]
0x18b50711e  488b88881d0f00                  mov     rcx, [rax+0F1D88h]
0x18b507125  4885c9                          test    rcx, rcx
0x18b507128  0f8485000000                    jz      loc_18B5071B3
0x18b50712e  ba01000000                      mov     edx, 1
0x18b507133  e8c8a58efc                      call    sub_187DF1700
0x18b507138  84c0                            test    al, al
0x18b50713a  0f840bffffff                    jz      loc_18B50704B
0x18b507140  e914ffffff                      jmp     loc_18B507059
0x18b507145  e8f6a264f5                      call    sub_180B51440
0x18b50714a  e8f1a264f5                      call    sub_180B51440
0x18b50714f  e89ccdd3f4                      call    sub_180243EF0
0x18b507154  e9ebfcffff                      jmp     loc_18B506E44
0x18b507159  e8e2a264f5                      call    sub_180B51440
0x18b50715e  e88dcdd3f4                      call    sub_180243EF0
0x18b507163  e943fdffff                      jmp     loc_18B506EAB
0x18b507168  e8d3a264f5                      call    sub_180B51440
0x18b50716d  e87ecdd3f4                      call    sub_180243EF0
0x18b507172  e9d3fdffff                      jmp     loc_18B506F4A
0x18b507177  e8c4a264f5                      call    sub_180B51440
0x18b50717c  e86fcdd3f4                      call    sub_180243EF0
0x18b507181  e917feffff                      jmp     loc_18B506F9D
0x18b507186  e8b5a264f5                      call    sub_180B51440
0x18b50718b  e860cdd3f4                      call    sub_180243EF0
0x18b507190  e951feffff                      jmp     loc_18B506FE6
0x18b507195  e8a6a264f5                      call    sub_180B51440
0x18b50719a  e851cdd3f4                      call    sub_180243EF0
0x18b50719f  e926ffffff                      jmp     loc_18B5070CA
0x18b5071a4  e897a264f5                      call    sub_180B51440
0x18b5071a9  e842cdd3f4                      call    sub_180243EF0
0x18b5071ae  e95dffffff                      jmp     loc_18B507110
0x18b5071b3  e888a264f5                      call    sub_180B51440
