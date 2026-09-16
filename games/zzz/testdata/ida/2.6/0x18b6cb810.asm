0x18b6cb810  4157                            push    r15
0x18b6cb812  4156                            push    r14
0x18b6cb814  4155                            push    r13
0x18b6cb816  4154                            push    r12
0x18b6cb818  56                              push    rsi
0x18b6cb819  57                              push    rdi
0x18b6cb81a  55                              push    rbp
0x18b6cb81b  53                              push    rbx
0x18b6cb81c  4881ece8000000                  sub     rsp, 0E8h
0x18b6cb823  0f29bc24d0000000                movaps  [rsp+128h+var_58], xmm7
0x18b6cb82b  0f29b424c0000000                movaps  [rsp+128h+var_68], xmm6
0x18b6cb833  4989cc                          mov     r12, rcx
0x18b6cb836  803dc94d61f900                  cmp     cs:byte_184CE0606, 0
0x18b6cb83d  0f84c6040000                    jz      loc_18B6CBD09
0x18b6cb843  48c744246800000000              mov     [rsp+128h+var_C0], 0
0x18b6cb84c  803d4b0d23f900                  cmp     cs:byte_1848FC59E, 0
0x18b6cb853  0f85d7040000                    jnz     loc_18B6CBD30
0x18b6cb859  488b0d305628f9                  mov     rcx, cs:qword_184950E90
0x18b6cb860  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x18b6cb867  0f8420050000                    jz      loc_18B6CBD8D
0x18b6cb86d  31ff                            xor     edi, edi
0x18b6cb86f  31c9                            xor     ecx, ecx
0x18b6cb871  e8ba82a3ff                      call    sub_18B103B30
0x18b6cb876  be00000000                      mov     esi, 0
0x18b6cb87b  84c0                            test    al, al
0x18b6cb87d  0f840b020000                    jz      loc_18B6CBA8E
0x18b6cb883  803de2451ff900                  cmp     cs:byte_1848BFE6C, 0
0x18b6cb88a  0f8507050000                    jnz     loc_18B6CBD97
0x18b6cb890  31c9                            xor     ecx, ecx
0x18b6cb892  ff1580b124f9                    call    cs:qword_184916A18
0x18b6cb898  84c0                            test    al, al
0x18b6cb89a  753c                            jnz     short loc_18B6CB8D8
0x18b6cb89c  803dc9451ff900                  cmp     cs:byte_1848BFE6C, 0
0x18b6cb8a3  0f85e0050000                    jnz     loc_18B6CBE89
0x18b6cb8a9  b901000000                      mov     ecx, 1
0x18b6cb8ae  ff1564b124f9                    call    cs:qword_184916A18
0x18b6cb8b4  84c0                            test    al, al
0x18b6cb8b6  7520                            jnz     short loc_18B6CB8D8
0x18b6cb8b8  803dad451ff900                  cmp     cs:byte_1848BFE6C, 0
0x18b6cb8bf  0f850d060000                    jnz     loc_18B6CBED2
0x18b6cb8c5  b902000000                      mov     ecx, 2
0x18b6cb8ca  ff1548b124f9                    call    cs:qword_184916A18
0x18b6cb8d0  84c0                            test    al, al
0x18b6cb8d2  0f843e060000                    jz      loc_18B6CBF16
0x18b6cb8d8  41807c243800                    cmp     byte ptr [r12+38h], 0
0x18b6cb8de  0f84d3000000                    jz      loc_18B6CB9B7
0x18b6cb8e4  c744242800000000                mov     dword ptr [rsp+128h+var_108+8], 0
0x18b6cb8ec  48c744242000000000              mov     qword ptr [rsp+128h+var_108], 0
0x18b6cb8f5  488d4c2420                      lea     rcx, [rsp+128h+var_108]
0x18b6cb8fa  ff1590b124f9                    call    cs:qword_184916A90
0x18b6cb900  f30f107c2420                    movss   xmm7, dword ptr [rsp+128h+var_108]
0x18b6cb906  f30f10742424                    movss   xmm6, dword ptr [rsp+128h+var_108+4]
0x18b6cb90c  488b0da5dd27f9                  mov     rcx, cs:qword_1849496B8
0x18b6cb913  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x18b6cb91a  0f84ef040000                    jz      loc_18B6CBE0F
0x18b6cb920  498b442418                      mov     rax, [r12+18h]
0x18b6cb925  4885c0                          test    rax, rax
0x18b6cb928  0f84f4040000                    jz      loc_18B6CBE22
0x18b6cb92e  83781800                        cmp     dword ptr [rax+18h], 0
0x18b6cb932  0f84ef040000                    jz      loc_18B6CBE27
0x18b6cb938  f30f5c7028                      subss   xmm6, dword ptr [rax+28h]
0x18b6cb93d  f30f5c7824                      subss   xmm7, dword ptr [rax+24h]
0x18b6cb942  660f7efb                        movd    ebx, xmm7
0x18b6cb946  660f7ef0                        movd    eax, xmm6
0x18b6cb94a  4889c1                          mov     rcx, rax
0x18b6cb94d  48c1e120                        shl     rcx, 20h
0x18b6cb951  4809cb                          or      rbx, rcx
0x18b6cb954  803d440c23f900                  cmp     cs:byte_1848FC59F, 0
0x18b6cb95b  0f85d5040000                    jnz     loc_18B6CBE36
0x18b6cb961  b9000080bf                      mov     ecx, 0BF800000h
0x18b6cb966  f30f1005ae3ebbf6                movss   xmm0, cs:X
0x18b6cb96e  0f2ec7                          ucomiss xmm0, xmm7
0x18b6cb971  ba000080bf                      mov     edx, 0BF800000h
0x18b6cb976  7711                            ja      short loc_18B6CB989
0x18b6cb978  0f2e3da93abbf6                  ucomiss xmm7, cs:flt_18227F428
0x18b6cb97f  4889da                          mov     rdx, rbx
0x18b6cb982  7605                            jbe     short loc_18B6CB989
0x18b6cb984  ba0000803f                      mov     edx, 3F800000h
0x18b6cb989  0f2ec6                          ucomiss xmm0, xmm6
0x18b6cb98c  7711                            ja      short loc_18B6CB99F
0x18b6cb98e  0f2e35933abbf6                  ucomiss xmm6, cs:flt_18227F428
0x18b6cb995  4889c1                          mov     rcx, rax
0x18b6cb998  7605                            jbe     short loc_18B6CB99F
0x18b6cb99a  b90000803f                      mov     ecx, 3F800000h
0x18b6cb99f  48c1e120                        shl     rcx, 20h
0x18b6cb9a3  89d3                            mov     ebx, edx
0x18b6cb9a5  4809cb                          or      rbx, rcx
0x18b6cb9a8  498b742418                      mov     rsi, [r12+18h]
0x18b6cb9ad  4885f6                          test    rsi, rsi
0x18b6cb9b0  751a                            jnz     short loc_18B6CB9CC
0x18b6cb9b2  e9cd040000                      jmp     loc_18B6CBE84
0x18b6cb9b7  488b1de2c51bf9                  mov     rbx, qword ptr cs:xmmword_184887FA0
0x18b6cb9be  498b742418                      mov     rsi, [r12+18h]
0x18b6cb9c3  4885f6                          test    rsi, rsi
0x18b6cb9c6  0f84b8040000                    jz      loc_18B6CBE84
0x18b6cb9cc  837e1800                        cmp     dword ptr [rsi+18h], 0
0x18b6cb9d0  0f8407040000                    jz      loc_18B6CBDDD
0x18b6cb9d6  c7462004ffffff                  mov     dword ptr [rsi+20h], 0FFFFFF04h
0x18b6cb9dd  c744242800000000                mov     dword ptr [rsp+128h+var_108+8], 0
0x18b6cb9e5  48c744242000000000              mov     qword ptr [rsp+128h+var_108], 0
0x18b6cb9ee  488d4c2420                      lea     rcx, [rsp+128h+var_108]
0x18b6cb9f3  ff1597b024f9                    call    cs:qword_184916A90
0x18b6cb9f9  837e1800                        cmp     dword ptr [rsi+18h], 0
0x18b6cb9fd  0f84e9030000                    jz      loc_18B6CBDEC
0x18b6cba03  8b442420                        mov     eax, dword ptr [rsp+128h+var_108]
0x18b6cba07  8b4c2424                        mov     ecx, dword ptr [rsp+128h+var_108+4]
0x18b6cba0b  48c1e120                        shl     rcx, 20h
0x18b6cba0f  4809c1                          or      rcx, rax
0x18b6cba12  48894e24                        mov     [rsi+24h], rcx
0x18b6cba16  498b6c2420                      mov     rbp, [r12+20h]
0x18b6cba1b  4885ed                          test    rbp, rbp
0x18b6cba1e  743d                            jz      short loc_18B6CBA5D
0x18b6cba20  41807c243800                    cmp     byte ptr [r12+38h], 0
0x18b6cba26  7535                            jnz     short loc_18B6CBA5D
0x18b6cba28  c744242800000000                mov     dword ptr [rsp+128h+var_108+8], 0
0x18b6cba30  48c744242000000000              mov     qword ptr [rsp+128h+var_108], 0
0x18b6cba39  488d4c2420                      lea     rcx, [rsp+128h+var_108]
0x18b6cba3e  ff154cb024f9                    call    cs:qword_184916A90
0x18b6cba44  4c8b442420                      mov     r8, qword ptr [rsp+128h+var_108]
0x18b6cba49  4c8b0dc8c042f9                  mov     r9, cs:qword_184AF7B18
0x18b6cba50  4889e9                          mov     rcx, rbp
0x18b6cba53  ba04ffffff                      mov     edx, 0FFFFFF04h
0x18b6cba58  e81347bd02                      call    sub_18E2A0170
0x18b6cba5d  498b4c2430                      mov     rcx, [r12+30h]
0x18b6cba62  4885c9                          test    rcx, rcx
0x18b6cba65  741c                            jz      short loc_18B6CBA83
0x18b6cba67  41807c243800                    cmp     byte ptr [r12+38h], 0
0x18b6cba6d  7414                            jz      short loc_18B6CBA83
0x18b6cba6f  4c8b0da2c042f9                  mov     r9, cs:qword_184AF7B18
0x18b6cba76  ba04ffffff                      mov     edx, 0FFFFFF04h
0x18b6cba7b  4989d8                          mov     r8, rbx
0x18b6cba7e  e8ed46bd02                      call    sub_18E2A0170
0x18b6cba83  41c644243801                    mov     byte ptr [r12+38h], 1
0x18b6cba89  be01000000                      mov     esi, 1
0x18b6cba8e  0f57f6                          xorps   xmm6, xmm6
0x18b6cba91  488d5c2420                      lea     rbx, [rsp+128h+var_108]
0x18b6cba96  4c8d6c2470                      lea     r13, [rsp+128h+var_B8]
0x18b6cba9b  4c8d742468                      lea     r14, [rsp+128h+var_C0]
0x18b6cbaa0  4c8d7c2478                      lea     r15, [rsp+128h+var_B0]
0x18b6cbaa5  eb0b                            jmp     short loc_18B6CBAB2
0x18b6cbab0  ffc7                            inc     edi
0x18b6cbab2  803db6431ff900                  cmp     cs:byte_1848BFE6F, 0
0x18b6cbab9  0f85dd000000                    jnz     loc_18B6CBB9C
0x18b6cbabf  ff15a3af24f9                    call    cs:qword_184916A68
0x18b6cbac5  39c7                            cmp     edi, eax
0x18b6cbac7  0f8db4010000                    jge     loc_18B6CBC81
0x18b6cbacd  803d9e431ff900                  cmp     cs:byte_1848BFE72, 0
0x18b6cbad4  0f8506010000                    jnz     loc_18B6CBBE0
0x18b6cbada  0f29742450                      movaps  [rsp+128h+var_D8], xmm6
0x18b6cbadf  0f29742440                      movaps  [rsp+128h+var_E8], xmm6
0x18b6cbae4  0f29742430                      movaps  [rsp+128h+var_F8], xmm6
0x18b6cbae9  0f29742420                      movaps  [rsp+128h+var_108], xmm6
0x18b6cbaee  c744246000000000                mov     [rsp+128h+var_C8], 0
0x18b6cbaf6  89f9                            mov     ecx, edi
0x18b6cbaf8  4889da                          mov     rdx, rbx
0x18b6cbafb  ff1587af24f9                    call    cs:qword_184916A88
0x18b6cbb01  8b6c2420                        mov     ebp, dword ptr [rsp+128h+var_108]
0x18b6cbb05  488b442424                      mov     rax, qword ptr [rsp+128h+var_108+4]
0x18b6cbb0a  4889442470                      mov     [rsp+128h+var_B8], rax
0x18b6cbb0f  8b442444                        mov     eax, dword ptr [rsp+128h+var_E8+4]
0x18b6cbb13  4c89e9                          mov     rcx, r13
0x18b6cbb16  4c8b01                          mov     r8, [rcx]
0x18b6cbb19  83f803                          cmp     eax, 3
0x18b6cbb1c  0f831e010000                    jnb     loc_18B6CBC40
0x18b6cbb22  498b4c2418                      mov     rcx, [r12+18h]
0x18b6cbb27  4885c9                          test    rcx, rcx
0x18b6cbb2a  0f84c5010000                    jz      loc_18B6CBCF5
0x18b6cbb30  3b7118                          cmp     esi, [rcx+18h]
0x18b6cbb33  0f83c1010000                    jnb     loc_18B6CBCFA
0x18b6cbb39  4863d6                          movsxd  rdx, esi
0x18b6cbb3c  488d1452                        lea     rdx, [rdx+rdx*2]
0x18b6cbb40  896c9120                        mov     [rcx+rdx*4+20h], ebp
0x18b6cbb44  4c89449124                      mov     [rcx+rdx*4+24h], r8
0x18b6cbb49  83f801                          cmp     eax, 1
0x18b6cbb4c  7410                            jz      short loc_18B6CBB5E
0x18b6cbb4e  85c0                            test    eax, eax
0x18b6cbb50  753a                            jnz     short loc_18B6CBB8C
0x18b6cbb52  498b4c2420                      mov     rcx, [r12+20h]
0x18b6cbb57  4885c9                          test    rcx, rcx
0x18b6cbb5a  7522                            jnz     short loc_18B6CBB7E
0x18b6cbb5c  eb2e                            jmp     short loc_18B6CBB8C
0x18b6cbb5e  4c89e1                          mov     rcx, r12
0x18b6cbb61  89ea                            mov     edx, ebp
0x18b6cbb63  4d89f1                          mov     r9, r14
0x18b6cbb66  e815060000                      call    sub_18B6CC180
0x18b6cbb6b  84c0                            test    al, al
0x18b6cbb6d  741d                            jz      short loc_18B6CBB8C
0x18b6cbb6f  498b4c2430                      mov     rcx, [r12+30h]
0x18b6cbb74  4885c9                          test    rcx, rcx
0x18b6cbb77  7413                            jz      short loc_18B6CBB8C
0x18b6cbb79  4c8b442468                      mov     r8, [rsp+128h+var_C0]
0x18b6cbb7e  4c8b0d93bf42f9                  mov     r9, cs:qword_184AF7B18
0x18b6cbb85  89ea                            mov     edx, ebp
0x18b6cbb87  e8e445bd02                      call    sub_18E2A0170
0x18b6cbb8c  83fe0a                          cmp     esi, 0Ah
0x18b6cbb8f  0f8d17010000                    jge     loc_18B6CBCAC
0x18b6cbb95  ffc6                            inc     esi
0x18b6cbb97  e914ffffff                      jmp     loc_18B6CBAB0
0x18b6cbb9c  488b0d75952bf9                  mov     rcx, cs:qword_184985118
0x18b6cbba3  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x18b6cbbaa  0f84bd000000                    jz      loc_18B6CBC6D
0x18b6cbbb0  488b05a9a01bf9                  mov     rax, cs:qword_184885C60
0x18b6cbbb7  488b8070e40300                  mov     rax, [rax+3E470h]
0x18b6cbbbe  488b8898370200                  mov     rcx, [rax+23798h]
0x18b6cbbc5  4885c9                          test    rcx, rcx
0x18b6cbbc8  0f8477040000                    jz      loc_18B6CC045
0x18b6cbbce  e8dd1099fb                      call    sub_18705CCB0
0x18b6cbbd3  39c7                            cmp     edi, eax
0x18b6cbbd5  0f8cf2feffff                    jl      loc_18B6CBACD
0x18b6cbbdb  e9a1000000                      jmp     loc_18B6CBC81
0x18b6cbbe0  488b0d31952bf9                  mov     rcx, cs:qword_184985118
0x18b6cbbe7  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x18b6cbbee  0f8483000000                    jz      loc_18B6CBC77
0x18b6cbbf4  488b0565a01bf9                  mov     rax, cs:qword_184885C60
0x18b6cbbfb  488b8070e40300                  mov     rax, [rax+3E470h]
0x18b6cbc02  488b90b0370200                  mov     rdx, [rax+237B0h]
0x18b6cbc09  4885d2                          test    rdx, rdx
0x18b6cbc0c  0f8438040000                    jz      loc_18B6CC04A
0x18b6cbc12  4c89f9                          mov     rcx, r15
0x18b6cbc15  4189f8                          mov     r8d, edi
0x18b6cbc18  e84301f005                      call    sub_1915CBD60
0x18b6cbc1d  8b6c2478                        mov     ebp, [rsp+128h+var_B0]
0x18b6cbc21  8b84249c000000                  mov     eax, [rsp+128h+var_8C]
0x18b6cbc28  488d4c247c                      lea     rcx, [rsp+128h+var_AC]
0x18b6cbc2d  4c8b01                          mov     r8, [rcx]
0x18b6cbc30  83f803                          cmp     eax, 3
0x18b6cbc33  0f82e9feffff                    jb      loc_18B6CBB22
0x18b6cbc39  0f1f8000000000                  nop     dword ptr [rax+00000000h]
0x18b6cbc40  83c0fd                          add     eax, 0FFFFFFFDh
0x18b6cbc43  83f802                          cmp     eax, 2
0x18b6cbc46  0f8364feffff                    jnb     loc_18B6CBAB0
0x18b6cbc4c  498b4c2428                      mov     rcx, [r12+28h]
0x18b6cbc51  4885c9                          test    rcx, rcx
0x18b6cbc54  0f8456feffff                    jz      loc_18B6CBAB0
0x18b6cbc5a  4c8b0db7be42f9                  mov     r9, cs:qword_184AF7B18
0x18b6cbc61  89ea                            mov     edx, ebp
0x18b6cbc63  e80845bd02                      call    sub_18E2A0170
0x18b6cbc68  e943feffff                      jmp     loc_18B6CBAB0
0x18b6cbc6d  e8fe8ab6f4                      call    sub_180234770
0x18b6cbc72  e939ffffff                      jmp     loc_18B6CBBB0
0x18b6cbc77  e8f48ab6f4                      call    sub_180234770
0x18b6cbc7c  e973ffffff                      jmp     loc_18B6CBBF4
0x18b6cbc81  83fe0a                          cmp     esi, 0Ah
0x18b6cbc84  7f26                            jg      short loc_18B6CBCAC
0x18b6cbc86  498b442418                      mov     rax, [r12+18h]
0x18b6cbc8b  4885c0                          test    rax, rax
0x18b6cbc8e  0f8467010000                    jz      loc_18B6CBDFB
0x18b6cbc94  3b7018                          cmp     esi, [rax+18h]
0x18b6cbc97  0f8363010000                    jnb     loc_18B6CBE00
0x18b6cbc9d  4863ce                          movsxd  rcx, esi
0x18b6cbca0  488d0c49                        lea     rcx, [rcx+rcx*2]
0x18b6cbca4  c744882003ffffff                mov     dword ptr [rax+rcx*4+20h], 0FFFFFF03h
0x18b6cbcac  498b542410                      mov     rdx, [r12+10h]
0x18b6cbcb1  498b4c2418                      mov     rcx, [r12+18h]
0x18b6cbcb6  41b80b000000                    mov     r8d, 0Bh
0x18b6cbcbc  e8af0eab0d                      call    sub_19917CB70
0x18b6cbcc1  41807c243900                    cmp     byte ptr [r12+39h], 0
0x18b6cbcc7  7408                            jz      short loc_18B6CBCD1
0x18b6cbcc9  4c89e1                          mov     rcx, r12
0x18b6cbccc  e84f060000                      call    sub_18B6CC320
0x18b6cbcd1  0f28b424c0000000                movaps  xmm6, [rsp+128h+var_68]
0x18b6cbcd9  0f28bc24d0000000                movaps  xmm7, [rsp+128h+var_58]
0x18b6cbce1  4881c4e8000000                  add     rsp, 0E8h
0x18b6cbce8  5b                              pop     rbx
0x18b6cbce9  5d                              pop     rbp
0x18b6cbcea  5f                              pop     rdi
0x18b6cbceb  5e                              pop     rsi
0x18b6cbcec  415c                            pop     r12
0x18b6cbcee  415d                            pop     r13
0x18b6cbcf0  415e                            pop     r14
0x18b6cbcf2  415f                            pop     r15
0x18b6cbcf4  c3                              retn
0x18b6cbcf5  e866da2af5                      call    sub_180979760
0x18b6cbcfa  e88182b6f4                      call    sub_180233F80
0x18b6cbcff  4889c1                          mov     rcx, rax
0x18b6cbd02  31d2                            xor     edx, edx
0x18b6cbd04  e8e7d92af5                      call    sub_1809796F0
0x18b6cbd09  b946230300                      mov     ecx, 32346h
0x18b6cbd0e  e81dffb6f4                      call    sub_18023BC30
0x18b6cbd13  c605ec4861f901                  mov     cs:byte_184CE0606, 1
0x18b6cbd1a  48c744246800000000              mov     [rsp+128h+var_C0], 0
0x18b6cbd23  803d740823f900                  cmp     cs:byte_1848FC59E, 0
0x18b6cbd2a  0f8429fbffff                    jz      loc_18B6CB859
0x18b6cbd30  488b0de1932bf9                  mov     rcx, cs:qword_184985118
0x18b6cbd37  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x18b6cbd3e  0f840b030000                    jz      loc_18B6CC04F
0x18b6cbd44  488b05159f1bf9                  mov     rax, cs:qword_184885C60
0x18b6cbd4b  488b8070e40300                  mov     rax, [rax+3E470h]
0x18b6cbd52  488b8810712000                  mov     rcx, [rax+207110h]
0x18b6cbd59  4885c9                          test    rcx, rcx
0x18b6cbd5c  0f84f7020000                    jz      loc_18B6CC059
0x18b6cbd62  4c89e2                          mov     rdx, r12
0x18b6cbd65  0f28b424c0000000                movaps  xmm6, [rsp+128h+var_68]
0x18b6cbd6d  0f28bc24d0000000                movaps  xmm7, [rsp+128h+var_58]
0x18b6cbd75  4881c4e8000000                  add     rsp, 0E8h
0x18b6cbd7c  5b                              pop     rbx
0x18b6cbd7d  5d                              pop     rbp
0x18b6cbd7e  5f                              pop     rdi
0x18b6cbd7f  5e                              pop     rsi
0x18b6cbd80  415c                            pop     r12
0x18b6cbd82  415d                            pop     r13
0x18b6cbd84  415e                            pop     r14
0x18b6cbd86  415f                            pop     r15
0x18b6cbd88  e9439995fb                      jmp     sub_1870256D0
0x18b6cbd8d  e8de89b6f4                      call    sub_180234770
0x18b6cbd92  e9d6faffff                      jmp     loc_18B6CB86D
0x18b6cbd97  488b0d7a932bf9                  mov     rcx, cs:qword_184985118
0x18b6cbd9e  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x18b6cbda5  0f84b3020000                    jz      loc_18B6CC05E
0x18b6cbdab  488b05ae9e1bf9                  mov     rax, cs:qword_184885C60
0x18b6cbdb2  488b8070e40300                  mov     rax, [rax+3E470h]
0x18b6cbdb9  488b8880370200                  mov     rcx, [rax+23780h]
0x18b6cbdc0  4885c9                          test    rcx, rcx
0x18b6cbdc3  0f849f020000                    jz      loc_18B6CC068
0x18b6cbdc9  31d2                            xor     edx, edx
0x18b6cbdcb  e8f0b598fb                      call    sub_1870573C0
0x18b6cbdd0  84c0                            test    al, al
0x18b6cbdd2  0f84c4faffff                    jz      loc_18B6CB89C
0x18b6cbdd8  e9fbfaffff                      jmp     loc_18B6CB8D8
0x18b6cbddd  e89e81b6f4                      call    sub_180233F80
0x18b6cbde2  4889c1                          mov     rcx, rax
0x18b6cbde5  31d2                            xor     edx, edx
0x18b6cbde7  e804d92af5                      call    sub_1809796F0
0x18b6cbdec  e88f81b6f4                      call    sub_180233F80
0x18b6cbdf1  4889c1                          mov     rcx, rax
0x18b6cbdf4  31d2                            xor     edx, edx
0x18b6cbdf6  e8f5d82af5                      call    sub_1809796F0
0x18b6cbdfb  e860d92af5                      call    sub_180979760
0x18b6cbe00  e87b81b6f4                      call    sub_180233F80
0x18b6cbe05  4889c1                          mov     rcx, rax
0x18b6cbe08  31d2                            xor     edx, edx
0x18b6cbe0a  e8e1d82af5                      call    sub_1809796F0
0x18b6cbe0f  e85c89b6f4                      call    sub_180234770
0x18b6cbe14  498b442418                      mov     rax, [r12+18h]
0x18b6cbe19  4885c0                          test    rax, rax
0x18b6cbe1c  0f850cfbffff                    jnz     loc_18B6CB92E
0x18b6cbe22  e839d92af5                      call    sub_180979760
0x18b6cbe27  e85481b6f4                      call    sub_180233F80
0x18b6cbe2c  4889c1                          mov     rcx, rax
0x18b6cbe2f  31d2                            xor     edx, edx
0x18b6cbe31  e8bad82af5                      call    sub_1809796F0
0x18b6cbe36  488b0ddb922bf9                  mov     rcx, cs:qword_184985118
0x18b6cbe3d  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x18b6cbe44  0f8423020000                    jz      loc_18B6CC06D
0x18b6cbe4a  488b050f9e1bf9                  mov     rax, cs:qword_184885C60
0x18b6cbe51  488b8070e40300                  mov     rax, [rax+3E470h]
0x18b6cbe58  488b8818712000                  mov     rcx, [rax+207118h]
0x18b6cbe5f  4885c9                          test    rcx, rcx
0x18b6cbe62  0f840f020000                    jz      loc_18B6CC077
0x18b6cbe68  4c89e2                          mov     rdx, r12
0x18b6cbe6b  4989d8                          mov     r8, rbx
0x18b6cbe6e  e8edfbae07                      call    sub_1931BBA60
0x18b6cbe73  4889c3                          mov     rbx, rax
0x18b6cbe76  498b742418                      mov     rsi, [r12+18h]
0x18b6cbe7b  4885f6                          test    rsi, rsi
0x18b6cbe7e  0f8548fbffff                    jnz     loc_18B6CB9CC
0x18b6cbe84  e8d7d82af5                      call    sub_180979760
0x18b6cbe89  488b0d88922bf9                  mov     rcx, cs:qword_184985118
0x18b6cbe90  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x18b6cbe97  0f84df010000                    jz      loc_18B6CC07C
0x18b6cbe9d  488b05bc9d1bf9                  mov     rax, cs:qword_184885C60
0x18b6cbea4  488b8070e40300                  mov     rax, [rax+3E470h]
0x18b6cbeab  488b8880370200                  mov     rcx, [rax+23780h]
0x18b6cbeb2  4885c9                          test    rcx, rcx
0x18b6cbeb5  0f84cb010000                    jz      loc_18B6CC086
0x18b6cbebb  ba01000000                      mov     edx, 1
0x18b6cbec0  e8fbb498fb                      call    sub_1870573C0
0x18b6cbec5  84c0                            test    al, al
0x18b6cbec7  0f84ebf9ffff                    jz      loc_18B6CB8B8
0x18b6cbecd  e906faffff                      jmp     loc_18B6CB8D8
0x18b6cbed2  488b0d3f922bf9                  mov     rcx, cs:qword_184985118
0x18b6cbed9  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x18b6cbee0  0f84a5010000                    jz      loc_18B6CC08B
0x18b6cbee6  488b05739d1bf9                  mov     rax, cs:qword_184885C60
0x18b6cbeed  488b8070e40300                  mov     rax, [rax+3E470h]
0x18b6cbef4  488b8880370200                  mov     rcx, [rax+23780h]
0x18b6cbefb  4885c9                          test    rcx, rcx
0x18b6cbefe  0f8491010000                    jz      loc_18B6CC095
0x18b6cbf04  ba02000000                      mov     edx, 2
0x18b6cbf09  e8b2b498fb                      call    sub_1870573C0
0x18b6cbf0e  84c0                            test    al, al
0x18b6cbf10  0f85c2f9ffff                    jnz     loc_18B6CB8D8
0x18b6cbf16  803d513f1ff900                  cmp     cs:byte_1848BFE6E, 0
0x18b6cbf1d  0f8593000000                    jnz     loc_18B6CBFB6
0x18b6cbf23  31c9                            xor     ecx, ecx
0x18b6cbf25  ff15fdaa24f9                    call    cs:qword_184916A28
0x18b6cbf2b  84c0                            test    al, al
0x18b6cbf2d  752a                            jnz     short loc_18B6CBF59
0x18b6cbf2f  803d383f1ff900                  cmp     cs:byte_1848BFE6E, 0
0x18b6cbf36  0f85c0000000                    jnz     loc_18B6CBFFC
0x18b6cbf3c  b901000000                      mov     ecx, 1
0x18b6cbf41  ff15e1aa24f9                    call    cs:qword_184916A28
0x18b6cbf47  84c0                            test    al, al
0x18b6cbf49  750e                            jnz     short loc_18B6CBF59
0x18b6cbf4b  b902000000                      mov     ecx, 2
0x18b6cbf50  e8cbcc8ffc                      call    sub_187FC8C20
0x18b6cbf55  84c0                            test    al, al
0x18b6cbf57  7455                            jz      short loc_18B6CBFAE
0x18b6cbf59  41c644243800                    mov     byte ptr [r12+38h], 0
0x18b6cbf5f  498b5c2428                      mov     rbx, [r12+28h]
0x18b6cbf64  be00000000                      mov     esi, 0
0x18b6cbf69  4885db                          test    rbx, rbx
0x18b6cbf6c  0f841cfbffff                    jz      loc_18B6CBA8E
0x18b6cbf72  c744242800000000                mov     dword ptr [rsp+128h+var_108+8], 0
0x18b6cbf7a  48c744242000000000              mov     qword ptr [rsp+128h+var_108], 0
0x18b6cbf83  488d4c2420                      lea     rcx, [rsp+128h+var_108]
0x18b6cbf88  ff1502ab24f9                    call    cs:qword_184916A90
0x18b6cbf8e  4c8b442420                      mov     r8, qword ptr [rsp+128h+var_108]
0x18b6cbf93  4c8b0d7ebb42f9                  mov     r9, cs:qword_184AF7B18
0x18b6cbf9a  4889d9                          mov     rcx, rbx
0x18b6cbf9d  ba04ffffff                      mov     edx, 0FFFFFF04h
0x18b6cbfa2  e8c941bd02                      call    sub_18E2A0170
0x18b6cbfa7  31f6                            xor     esi, esi
0x18b6cbfa9  e9e0faffff                      jmp     loc_18B6CBA8E
0x18b6cbfae  41c644243800                    mov     byte ptr [r12+38h], 0
0x18b6cbfb4  ebf1                            jmp     short loc_18B6CBFA7
0x18b6cbfb6  488b0d5b912bf9                  mov     rcx, cs:qword_184985118
0x18b6cbfbd  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x18b6cbfc4  0f84d0000000                    jz      loc_18B6CC09A
0x18b6cbfca  488b058f9c1bf9                  mov     rax, cs:qword_184885C60
0x18b6cbfd1  488b8070e40300                  mov     rax, [rax+3E470h]
0x18b6cbfd8  488b8890370200                  mov     rcx, [rax+23790h]
0x18b6cbfdf  4885c9                          test    rcx, rcx
0x18b6cbfe2  0f84bc000000                    jz      loc_18B6CC0A4
0x18b6cbfe8  31d2                            xor     edx, edx
0x18b6cbfea  e8d1b398fb                      call    sub_1870573C0
0x18b6cbfef  84c0                            test    al, al
0x18b6cbff1  0f8438ffffff                    jz      loc_18B6CBF2F
0x18b6cbff7  e95dffffff                      jmp     loc_18B6CBF59
0x18b6cbffc  488b0d15912bf9                  mov     rcx, cs:qword_184985118
0x18b6cc003  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x18b6cc00a  0f8499000000                    jz      loc_18B6CC0A9
0x18b6cc010  488b05499c1bf9                  mov     rax, cs:qword_184885C60
0x18b6cc017  488b8070e40300                  mov     rax, [rax+3E470h]
0x18b6cc01e  488b8890370200                  mov     rcx, [rax+23790h]
0x18b6cc025  4885c9                          test    rcx, rcx
0x18b6cc028  0f8485000000                    jz      loc_18B6CC0B3
0x18b6cc02e  ba01000000                      mov     edx, 1
0x18b6cc033  e888b398fb                      call    sub_1870573C0
0x18b6cc038  84c0                            test    al, al
0x18b6cc03a  0f840bffffff                    jz      loc_18B6CBF4B
0x18b6cc040  e914ffffff                      jmp     loc_18B6CBF59
0x18b6cc045  e816d72af5                      call    sub_180979760
0x18b6cc04a  e811d72af5                      call    sub_180979760
0x18b6cc04f  e81c87b6f4                      call    sub_180234770
0x18b6cc054  e9ebfcffff                      jmp     loc_18B6CBD44
0x18b6cc059  e802d72af5                      call    sub_180979760
0x18b6cc05e  e80d87b6f4                      call    sub_180234770
0x18b6cc063  e943fdffff                      jmp     loc_18B6CBDAB
0x18b6cc068  e8f3d62af5                      call    sub_180979760
0x18b6cc06d  e8fe86b6f4                      call    sub_180234770
0x18b6cc072  e9d3fdffff                      jmp     loc_18B6CBE4A
0x18b6cc077  e8e4d62af5                      call    sub_180979760
0x18b6cc07c  e8ef86b6f4                      call    sub_180234770
0x18b6cc081  e917feffff                      jmp     loc_18B6CBE9D
0x18b6cc086  e8d5d62af5                      call    sub_180979760
0x18b6cc08b  e8e086b6f4                      call    sub_180234770
0x18b6cc090  e951feffff                      jmp     loc_18B6CBEE6
0x18b6cc095  e8c6d62af5                      call    sub_180979760
0x18b6cc09a  e8d186b6f4                      call    sub_180234770
0x18b6cc09f  e926ffffff                      jmp     loc_18B6CBFCA
0x18b6cc0a4  e8b7d62af5                      call    sub_180979760
0x18b6cc0a9  e8c286b6f4                      call    sub_180234770
0x18b6cc0ae  e95dffffff                      jmp     loc_18B6CC010
0x18b6cc0b3  e8a8d62af5                      call    sub_180979760
