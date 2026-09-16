0x19048a790  4157                            push    r15
0x19048a792  4156                            push    r14
0x19048a794  4155                            push    r13
0x19048a796  4154                            push    r12
0x19048a798  56                              push    rsi
0x19048a799  57                              push    rdi
0x19048a79a  55                              push    rbp
0x19048a79b  53                              push    rbx
0x19048a79c  4881ece8000000                  sub     rsp, 0E8h
0x19048a7a3  0f29b424d0000000                movaps  [rsp+128h+var_58], xmm6
0x19048a7ab  4889ce                          mov     rsi, rcx
0x19048a7ae  803ddc5bf3f400                  cmp     cs:byte_1853C0391, 0
0x19048a7b5  0f84a3040000                    jz      loc_19048AC5E
0x19048a7bb  48c744242800000000              mov     [rsp+128h+var_100], 0
0x19048a7c4  803dbff4abf400                  cmp     cs:byte_184F49C8A, 0
0x19048a7cb  0f85b4040000                    jnz     loc_19048AC85
0x19048a7d1  488b0d086cb3f4                  mov     rcx, cs:qword_184FC13E0
0x19048a7d8  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x19048a7df  0f84d9040000                    jz      loc_19048ACBE
0x19048a7e5  803d5923f4f400                  cmp     cs:byte_1853CCB45, 0
0x19048a7ec  0f84de040000                    jz      loc_19048ACD0
0x19048a7f2  803de530acf400                  cmp     cs:byte_184F4D8DE, 0
0x19048a7f9  0f85ef040000                    jnz     loc_19048ACEE
0x19048a7ff  488b0dda6bb3f4                  mov     rcx, cs:qword_184FC13E0
0x19048a806  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x19048a80d  0f8408050000                    jz      loc_19048AD1B
0x19048a813  e838d90303                      call    sub_1934C8150
0x19048a818  4531e4                          xor     r12d, r12d
0x19048a81b  83f802                          cmp     eax, 2
0x19048a81e  0f85ef010000                    jnz     loc_19048AA13
0x19048a824  803dafefa8f400                  cmp     cs:byte_184F197DA, 0
0x19048a82b  0f850a050000                    jnz     loc_19048AD3B
0x19048a831  31c9                            xor     ecx, ecx
0x19048a833  ff151f47aff4                    call    cs:qword_184F7EF58
0x19048a839  84c0                            test    al, al
0x19048a83b  753c                            jnz     short loc_19048A879
0x19048a83d  803d96efa8f400                  cmp     cs:byte_184F197DA, 0
0x19048a844  0f85b0050000                    jnz     loc_19048ADFA
0x19048a84a  b901000000                      mov     ecx, 1
0x19048a84f  ff150347aff4                    call    cs:qword_184F7EF58
0x19048a855  84c0                            test    al, al
0x19048a857  7520                            jnz     short loc_19048A879
0x19048a859  803d7aefa8f400                  cmp     cs:byte_184F197DA, 0
0x19048a860  0f85c1050000                    jnz     loc_19048AE27
0x19048a866  b902000000                      mov     ecx, 2
0x19048a86b  ff15e746aff4                    call    cs:qword_184F7EF58
0x19048a871  84c0                            test    al, al
0x19048a873  0f84d6050000                    jz      loc_19048AE4F
0x19048a879  807e3900                        cmp     byte ptr [rsi+39h], 0
0x19048a87d  0f84cb000000                    jz      loc_19048A94E
0x19048a883  c744243800000000                mov     dword ptr [rsp+128h+var_F8+8], 0
0x19048a88b  48c744243000000000              mov     qword ptr [rsp+128h+var_F8], 0
0x19048a894  488d4c2430                      lea     rcx, [rsp+128h+var_F8]
0x19048a899  ff153147aff4                    call    cs:qword_184F7EFD0
0x19048a89f  488b7c2430                      mov     rdi, qword ptr [rsp+128h+var_F8]
0x19048a8a4  488b0df517b3f4                  mov     rcx, cs:qword_184FBC0A0
0x19048a8ab  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x19048a8b2  0f84df040000                    jz      loc_19048AD97
0x19048a8b8  488b4628                        mov     rax, [rsi+28h]
0x19048a8bc  4885c0                          test    rax, rax
0x19048a8bf  0f84e4040000                    jz      loc_19048ADA9
0x19048a8c5  83781800                        cmp     dword ptr [rax+18h], 0
0x19048a8c9  0f84df040000                    jz      loc_19048ADAE
0x19048a8cf  660f6ecf                        movd    xmm1, edi
0x19048a8d3  48c1ef20                        shr     rdi, 20h
0x19048a8d7  660f6ec7                        movd    xmm0, edi
0x19048a8db  f30f5c4028                      subss   xmm0, dword ptr [rax+28h]
0x19048a8e0  f30f5c4824                      subss   xmm1, dword ptr [rax+24h]
0x19048a8e5  66410f7ece                      movd    r14d, xmm1
0x19048a8ea  660f7ec3                        movd    ebx, xmm0
0x19048a8ee  803d92f3abf400                  cmp     cs:byte_184F49C87, 0
0x19048a8f5  0f85c2040000                    jnz     loc_19048ADBD
0x19048a8fb  bf000080bf                      mov     edi, 0BF800000h
0x19048a900  f30f10155c2ffcf1                movss   xmm2, cs:X
0x19048a908  0f2ed1                          ucomiss xmm2, xmm1
0x19048a90b  b8000080bf                      mov     eax, 0BF800000h
0x19048a910  7711                            ja      short loc_19048A923
0x19048a912  0f2e0d9392ecf1                  ucomiss xmm1, cs:flt_182353BAC
0x19048a919  4c89f0                          mov     rax, r14
0x19048a91c  7605                            jbe     short loc_19048A923
0x19048a91e  b80000803f                      mov     eax, 3F800000h
0x19048a923  0f2ed0                          ucomiss xmm2, xmm0
0x19048a926  7711                            ja      short loc_19048A939
0x19048a928  0f2e057d92ecf1                  ucomiss xmm0, cs:flt_182353BAC
0x19048a92f  4889df                          mov     rdi, rbx
0x19048a932  7605                            jbe     short loc_19048A939
0x19048a934  bf0000803f                      mov     edi, 3F800000h
0x19048a939  48c1e720                        shl     rdi, 20h
0x19048a93d  4809c7                          or      rdi, rax
0x19048a940  488b5e28                        mov     rbx, [rsi+28h]
0x19048a944  4885db                          test    rbx, rbx
0x19048a947  7519                            jnz     short loc_19048A962
0x19048a949  e9a7040000                      jmp     loc_19048ADF5
0x19048a94e  488b3d834da4f4                  mov     rdi, qword ptr cs:xmmword_184ECF6D0+8
0x19048a955  488b5e28                        mov     rbx, [rsi+28h]
0x19048a959  4885db                          test    rbx, rbx
0x19048a95c  0f8493040000                    jz      loc_19048ADF5
0x19048a962  837b1800                        cmp     dword ptr [rbx+18h], 0
0x19048a966  0f84f9030000                    jz      loc_19048AD65
0x19048a96c  c7432004ffffff                  mov     dword ptr [rbx+20h], 0FFFFFF04h
0x19048a973  c744243800000000                mov     dword ptr [rsp+128h+var_F8+8], 0
0x19048a97b  48c744243000000000              mov     qword ptr [rsp+128h+var_F8], 0
0x19048a984  488d4c2430                      lea     rcx, [rsp+128h+var_F8]
0x19048a989  ff154146aff4                    call    cs:qword_184F7EFD0
0x19048a98f  837b1800                        cmp     dword ptr [rbx+18h], 0
0x19048a993  0f84db030000                    jz      loc_19048AD74
0x19048a999  488b442430                      mov     rax, qword ptr [rsp+128h+var_F8]
0x19048a99e  48894324                        mov     [rbx+24h], rax
0x19048a9a2  488b5e30                        mov     rbx, [rsi+30h]
0x19048a9a6  4885db                          test    rbx, rbx
0x19048a9a9  743b                            jz      short loc_19048A9E6
0x19048a9ab  807e3900                        cmp     byte ptr [rsi+39h], 0
0x19048a9af  7535                            jnz     short loc_19048A9E6
0x19048a9b1  c744243800000000                mov     dword ptr [rsp+128h+var_F8+8], 0
0x19048a9b9  48c744243000000000              mov     qword ptr [rsp+128h+var_F8], 0
0x19048a9c2  488d4c2430                      lea     rcx, [rsp+128h+var_F8]
0x19048a9c7  ff150346aff4                    call    cs:qword_184F7EFD0
0x19048a9cd  4c8b442430                      mov     r8, qword ptr [rsp+128h+var_F8]
0x19048a9d2  4c8b0da72eccf4                  mov     r9, cs:qword_18514D880
0x19048a9d9  4889d9                          mov     rcx, rbx
0x19048a9dc  ba04ffffff                      mov     edx, 0FFFFFF04h
0x19048a9e1  e8ea38d4fc                      call    sub_18D1CE2D0
0x19048a9e6  488b4e18                        mov     rcx, [rsi+18h]
0x19048a9ea  4885c9                          test    rcx, rcx
0x19048a9ed  741a                            jz      short loc_19048AA09
0x19048a9ef  807e3900                        cmp     byte ptr [rsi+39h], 0
0x19048a9f3  7414                            jz      short loc_19048AA09
0x19048a9f5  4c8b0d842eccf4                  mov     r9, cs:qword_18514D880
0x19048a9fc  ba04ffffff                      mov     edx, 0FFFFFF04h
0x19048aa01  4989f8                          mov     r8, rdi
0x19048aa04  e8c738d4fc                      call    sub_18D1CE2D0
0x19048aa09  c6463901                        mov     byte ptr [rsi+39h], 1
0x19048aa0d  41bc01000000                    mov     r12d, 1
0x19048aa13  31ff                            xor     edi, edi
0x19048aa15  0f57f6                          xorps   xmm6, xmm6
0x19048aa18  488d5c2430                      lea     rbx, [rsp+128h+var_F8]
0x19048aa1d  4c8dac2480000000                lea     r13, [rsp+128h+var_A8]
0x19048aa25  4c8d742428                      lea     r14, [rsp+128h+var_100]
0x19048aa2a  4c8dbc248c000000                lea     r15, [rsp+128h+var_9C]
0x19048aa32  eb0e                            jmp     short loc_19048AA42
0x19048aa40  ffc7                            inc     edi
0x19048aa42  803d96eda8f400                  cmp     cs:byte_184F197DF, 0
0x19048aa49  0f85e0000000                    jnz     loc_19048AB2F
0x19048aa4f  ff155345aff4                    call    cs:qword_184F7EFA8
0x19048aa55  39c7                            cmp     edi, eax
0x19048aa57  0f8d6f010000                    jge     loc_19048ABCC
0x19048aa5d  803d78eda8f400                  cmp     cs:byte_184F197DC, 0
0x19048aa64  0f85ea000000                    jnz     loc_19048AB54
0x19048aa6a  0f29742460                      movaps  [rsp+128h+var_C8], xmm6
0x19048aa6f  0f29742450                      movaps  [rsp+128h+var_D8], xmm6
0x19048aa74  0f29742440                      movaps  [rsp+128h+var_E8], xmm6
0x19048aa79  0f29742430                      movaps  [rsp+128h+var_F8], xmm6
0x19048aa7e  c744247000000000                mov     [rsp+128h+var_B8], 0
0x19048aa86  89f9                            mov     ecx, edi
0x19048aa88  4889da                          mov     rdx, rbx
0x19048aa8b  ff153745aff4                    call    cs:qword_184F7EFC8
0x19048aa91  8b6c2430                        mov     ebp, dword ptr [rsp+128h+var_F8]
0x19048aa95  488b442434                      mov     rax, qword ptr [rsp+128h+var_F8+4]
0x19048aa9a  4889842480000000                mov     [rsp+128h+var_A8], rax
0x19048aaa2  8b442454                        mov     eax, dword ptr [rsp+128h+var_D8+4]
0x19048aaa6  4c89e9                          mov     rcx, r13
0x19048aaa9  4c8b01                          mov     r8, [rcx]
0x19048aaac  83f803                          cmp     eax, 3
0x19048aaaf  0f83eb000000                    jnb     loc_19048ABA0
0x19048aab5  488b4e28                        mov     rcx, [rsi+28h]
0x19048aab9  4885c9                          test    rcx, rcx
0x19048aabc  0f8488010000                    jz      loc_19048AC4A
0x19048aac2  443b6118                        cmp     r12d, [rcx+18h]
0x19048aac6  0f8383010000                    jnb     loc_19048AC4F
0x19048aacc  4963d4                          movsxd  rdx, r12d
0x19048aacf  488d1452                        lea     rdx, [rdx+rdx*2]
0x19048aad3  896c9120                        mov     [rcx+rdx*4+20h], ebp
0x19048aad7  4c89449124                      mov     [rcx+rdx*4+24h], r8
0x19048aadc  83f801                          cmp     eax, 1
0x19048aadf  740f                            jz      short loc_19048AAF0
0x19048aae1  85c0                            test    eax, eax
0x19048aae3  7538                            jnz     short loc_19048AB1D
0x19048aae5  488b4e30                        mov     rcx, [rsi+30h]
0x19048aae9  4885c9                          test    rcx, rcx
0x19048aaec  7521                            jnz     short loc_19048AB0F
0x19048aaee  eb2d                            jmp     short loc_19048AB1D
0x19048aaf0  4889f1                          mov     rcx, rsi
0x19048aaf3  89ea                            mov     edx, ebp
0x19048aaf5  4d89f1                          mov     r9, r14
0x19048aaf8  e873040000                      call    sub_19048AF70
0x19048aafd  84c0                            test    al, al
0x19048aaff  741c                            jz      short loc_19048AB1D
0x19048ab01  488b4e18                        mov     rcx, [rsi+18h]
0x19048ab05  4885c9                          test    rcx, rcx
0x19048ab08  7413                            jz      short loc_19048AB1D
0x19048ab0a  4c8b442428                      mov     r8, [rsp+128h+var_100]
0x19048ab0f  4c8b0d6a2dccf4                  mov     r9, cs:qword_18514D880
0x19048ab16  89ea                            mov     edx, ebp
0x19048ab18  e8b337d4fc                      call    sub_18D1CE2D0
0x19048ab1d  4183fc0a                        cmp     r12d, 0Ah
0x19048ab21  0f8dd3000000                    jge     loc_19048ABFA
0x19048ab27  41ffc4                          inc     r12d
0x19048ab2a  e911ffffff                      jmp     loc_19048AA40
0x19048ab2f  b91fe80000                      mov     ecx, 0E81Fh
0x19048ab34  e8e7116a02                      call    sub_192B2BD20
0x19048ab39  4885c0                          test    rax, rax
0x19048ab3c  0f84f6030000                    jz      loc_19048AF38
0x19048ab42  4889c1                          mov     rcx, rax
0x19048ab45  e8465ce1fd                      call    sub_18E2A0790
0x19048ab4a  39c7                            cmp     edi, eax
0x19048ab4c  0f8c0bffffff                    jl      loc_19048AA5D
0x19048ab52  eb78                            jmp     short loc_19048ABCC
0x19048ab54  b91ce80000                      mov     ecx, 0E81Ch
0x19048ab59  e8c2116a02                      call    sub_192B2BD20
0x19048ab5e  4885c0                          test    rax, rax
0x19048ab61  0f84d6030000                    jz      loc_19048AF3D
0x19048ab67  4c89f9                          mov     rcx, r15
0x19048ab6a  4889c2                          mov     rdx, rax
0x19048ab6d  4189f8                          mov     r8d, edi
0x19048ab70  e8ab2ab8f9                      call    sub_18A00D620
0x19048ab75  8bac248c000000                  mov     ebp, [rsp+128h+var_9C]
0x19048ab7c  8b8424b0000000                  mov     eax, [rsp+128h+var_78]
0x19048ab83  488d8c2490000000                lea     rcx, [rsp+128h+var_98]
0x19048ab8b  4c8b01                          mov     r8, [rcx]
0x19048ab8e  83f803                          cmp     eax, 3
0x19048ab91  0f821effffff                    jb      loc_19048AAB5
0x19048ab97  660f1f840000000000              nop     word ptr [rax+rax+00000000h]
0x19048aba0  83c0fd                          add     eax, 0FFFFFFFDh
0x19048aba3  83f802                          cmp     eax, 2
0x19048aba6  0f8394feffff                    jnb     loc_19048AA40
0x19048abac  488b4e20                        mov     rcx, [rsi+20h]
0x19048abb0  4885c9                          test    rcx, rcx
0x19048abb3  0f8487feffff                    jz      loc_19048AA40
0x19048abb9  4c8b0dc02cccf4                  mov     r9, cs:qword_18514D880
0x19048abc0  89ea                            mov     edx, ebp
0x19048abc2  e80937d4fc                      call    sub_18D1CE2D0
0x19048abc7  e974feffff                      jmp     loc_19048AA40
0x19048abcc  488b7e28                        mov     rdi, [rsi+28h]
0x19048abd0  4183fc0a                        cmp     r12d, 0Ah
0x19048abd4  7f28                            jg      short loc_19048ABFE
0x19048abd6  4885ff                          test    rdi, rdi
0x19048abd9  0f84a4010000                    jz      loc_19048AD83
0x19048abdf  443b6718                        cmp     r12d, [rdi+18h]
0x19048abe3  0f839f010000                    jnb     loc_19048AD88
0x19048abe9  4963c4                          movsxd  rax, r12d
0x19048abec  488d0440                        lea     rax, [rax+rax*2]
0x19048abf0  c744872003ffffff                mov     dword ptr [rdi+rax*4+20h], 0FFFFFF03h
0x19048abf8  eb04                            jmp     short loc_19048ABFE
0x19048abfa  488b7e28                        mov     rdi, [rsi+28h]
0x19048abfe  488b5e10                        mov     rbx, [rsi+10h]
0x19048ac02  803d9b80f0f400                  cmp     cs:byte_185392CA4, 0
0x19048ac09  0f8416010000                    jz      loc_19048AD25
0x19048ac0f  4889f9                          mov     rcx, rdi
0x19048ac12  4889da                          mov     rdx, rbx
0x19048ac15  41b80b000000                    mov     r8d, 0Bh
0x19048ac1b  e870ca6a0c                      call    sub_19CB37690
0x19048ac20  807e3800                        cmp     byte ptr [rsi+38h], 0
0x19048ac24  7408                            jz      short loc_19048AC2E
0x19048ac26  4889f1                          mov     rcx, rsi
0x19048ac29  e8c2f7ffff                      call    sub_19048A3F0
0x19048ac2e  0f28b424d0000000                movaps  xmm6, [rsp+128h+var_58]
0x19048ac36  4881c4e8000000                  add     rsp, 0E8h
0x19048ac3d  5b                              pop     rbx
0x19048ac3e  5d                              pop     rbp
0x19048ac3f  5f                              pop     rdi
0x19048ac40  5e                              pop     rsi
0x19048ac41  415c                            pop     r12
0x19048ac43  415d                            pop     r13
0x19048ac45  415e                            pop     r14
0x19048ac47  415f                            pop     r15
0x19048ac49  c3                              retn
0x19048ac4a  e811a74af0                      call    sub_180935360
0x19048ac4f  e85c9edfef                      call    sub_180284AB0
0x19048ac54  4889c1                          mov     rcx, rax
0x19048ac57  31d2                            xor     edx, edx
0x19048ac59  e892a64af0                      call    sub_1809352F0
0x19048ac5e  b951db0200                      mov     ecx, 2DB51h
0x19048ac63  e8381be0ef                      call    sub_18028C7A0
0x19048ac68  c6052257f3f401                  mov     cs:byte_1853C0391, 1
0x19048ac6f  48c744242800000000              mov     [rsp+128h+var_100], 0
0x19048ac78  803d0bf0abf400                  cmp     cs:byte_184F49C8A, 0
0x19048ac7f  0f844cfbffff                    jz      loc_19048A7D1
0x19048ac85  b9caec0300                      mov     ecx, 3ECCAh
0x19048ac8a  e891106a02                      call    sub_192B2BD20
0x19048ac8f  4885c0                          test    rax, rax
0x19048ac92  0f84aa020000                    jz      loc_19048AF42
0x19048ac98  4889c1                          mov     rcx, rax
0x19048ac9b  4889f2                          mov     rdx, rsi
0x19048ac9e  0f28b424d0000000                movaps  xmm6, [rsp+128h+var_58]
0x19048aca6  4881c4e8000000                  add     rsp, 0E8h
0x19048acad  5b                              pop     rbx
0x19048acae  5d                              pop     rbp
0x19048acaf  5f                              pop     rdi
0x19048acb0  5e                              pop     rsi
0x19048acb1  415c                            pop     r12
0x19048acb3  415d                            pop     r13
0x19048acb5  415e                            pop     r14
0x19048acb7  415f                            pop     r15
0x19048acb9  e92222dcfd                      jmp     sub_18E24CEE0
0x19048acbe  e82da5dfef                      call    sub_1802851F0
0x19048acc3  803d7b1ef4f400                  cmp     cs:byte_1853CCB45, 0
0x19048acca  0f8522fbffff                    jnz     loc_19048A7F2
0x19048acd0  b905a30300                      mov     ecx, 3A305h
0x19048acd5  e8c61ae0ef                      call    sub_18028C7A0
0x19048acda  c605641ef4f401                  mov     cs:byte_1853CCB45, 1
0x19048ace1  803df62bacf400                  cmp     cs:byte_184F4D8DE, 0
0x19048ace8  0f8411fbffff                    jz      loc_19048A7FF
0x19048acee  b91e290400                      mov     ecx, 4291Eh
0x19048acf3  e828106a02                      call    sub_192B2BD20
0x19048acf8  4885c0                          test    rax, rax
0x19048acfb  0f8446020000                    jz      loc_19048AF47
0x19048ad01  4531e4                          xor     r12d, r12d
0x19048ad04  4889c1                          mov     rcx, rax
0x19048ad07  31d2                            xor     edx, edx
0x19048ad09  e832b8defd                      call    sub_18E276540
0x19048ad0e  84c0                            test    al, al
0x19048ad10  0f850efbffff                    jnz     loc_19048A824
0x19048ad16  e9f8fcffff                      jmp     loc_19048AA13
0x19048ad1b  e8d0a4dfef                      call    sub_1802851F0
0x19048ad20  e9eefaffff                      jmp     loc_19048A813
0x19048ad25  b964040000                      mov     ecx, 464h
0x19048ad2a  e8711ae0ef                      call    sub_18028C7A0
0x19048ad2f  c6056e7ff0f401                  mov     cs:byte_185392CA4, 1
0x19048ad36  e9d4feffff                      jmp     loc_19048AC0F
0x19048ad3b  b91ae80000                      mov     ecx, 0E81Ah
0x19048ad40  e8db0f6a02                      call    sub_192B2BD20
0x19048ad45  4885c0                          test    rax, rax
0x19048ad48  0f84fe010000                    jz      loc_19048AF4C
0x19048ad4e  4889c1                          mov     rcx, rax
0x19048ad51  31d2                            xor     edx, edx
0x19048ad53  e8e8b7defd                      call    sub_18E276540
0x19048ad58  84c0                            test    al, al
0x19048ad5a  0f84ddfaffff                    jz      loc_19048A83D
0x19048ad60  e914fbffff                      jmp     loc_19048A879
0x19048ad65  e8469ddfef                      call    sub_180284AB0
0x19048ad6a  4889c1                          mov     rcx, rax
0x19048ad6d  31d2                            xor     edx, edx
0x19048ad6f  e87ca54af0                      call    sub_1809352F0
0x19048ad74  e8379ddfef                      call    sub_180284AB0
0x19048ad79  4889c1                          mov     rcx, rax
0x19048ad7c  31d2                            xor     edx, edx
0x19048ad7e  e86da54af0                      call    sub_1809352F0
0x19048ad83  e8d8a54af0                      call    sub_180935360
0x19048ad88  e8239ddfef                      call    sub_180284AB0
0x19048ad8d  4889c1                          mov     rcx, rax
0x19048ad90  31d2                            xor     edx, edx
0x19048ad92  e859a54af0                      call    sub_1809352F0
0x19048ad97  e854a4dfef                      call    sub_1802851F0
0x19048ad9c  488b4628                        mov     rax, [rsi+28h]
0x19048ada0  4885c0                          test    rax, rax
0x19048ada3  0f851cfbffff                    jnz     loc_19048A8C5
0x19048ada9  e8b2a54af0                      call    sub_180935360
0x19048adae  e8fd9cdfef                      call    sub_180284AB0
0x19048adb3  4889c1                          mov     rcx, rax
0x19048adb6  31d2                            xor     edx, edx
0x19048adb8  e833a54af0                      call    sub_1809352F0
0x19048adbd  b9c7ec0300                      mov     ecx, 3ECC7h
0x19048adc2  e8590f6a02                      call    sub_192B2BD20
0x19048adc7  4885c0                          test    rax, rax
0x19048adca  0f8481010000                    jz      loc_19048AF51
0x19048add0  48c1e320                        shl     rbx, 20h
0x19048add4  4c09f3                          or      rbx, r14
0x19048add7  4889c1                          mov     rcx, rax
0x19048adda  4889f2                          mov     rdx, rsi
0x19048addd  4989d8                          mov     r8, rbx
0x19048ade0  e8ab2508fb                      call    sub_18B50D390
0x19048ade5  4889c7                          mov     rdi, rax
0x19048ade8  488b5e28                        mov     rbx, [rsi+28h]
0x19048adec  4885db                          test    rbx, rbx
0x19048adef  0f856dfbffff                    jnz     loc_19048A962
0x19048adf5  e866a54af0                      call    sub_180935360
0x19048adfa  b91ae80000                      mov     ecx, 0E81Ah
0x19048adff  e81c0f6a02                      call    sub_192B2BD20
0x19048ae04  4885c0                          test    rax, rax
0x19048ae07  0f8449010000                    jz      loc_19048AF56
0x19048ae0d  4889c1                          mov     rcx, rax
0x19048ae10  ba01000000                      mov     edx, 1
0x19048ae15  e826b7defd                      call    sub_18E276540
0x19048ae1a  84c0                            test    al, al
0x19048ae1c  0f8437faffff                    jz      loc_19048A859
0x19048ae22  e952faffff                      jmp     loc_19048A879
0x19048ae27  b91ae80000                      mov     ecx, 0E81Ah
0x19048ae2c  e8ef0e6a02                      call    sub_192B2BD20
0x19048ae31  4885c0                          test    rax, rax
0x19048ae34  0f8421010000                    jz      loc_19048AF5B
0x19048ae3a  4889c1                          mov     rcx, rax
0x19048ae3d  ba02000000                      mov     edx, 2
0x19048ae42  e8f9b6defd                      call    sub_18E276540
0x19048ae47  84c0                            test    al, al
0x19048ae49  0f852afaffff                    jnz     loc_19048A879
0x19048ae4f  803d8ae9a8f400                  cmp     cs:byte_184F197E0, 0
0x19048ae56  0f8590000000                    jnz     loc_19048AEEC
0x19048ae5c  31c9                            xor     ecx, ecx
0x19048ae5e  ff150441aff4                    call    cs:qword_184F7EF68
0x19048ae64  84c0                            test    al, al
0x19048ae66  752a                            jnz     short loc_19048AE92
0x19048ae68  803d71e9a8f400                  cmp     cs:byte_184F197E0, 0
0x19048ae6f  0f859a000000                    jnz     loc_19048AF0F
0x19048ae75  b901000000                      mov     ecx, 1
0x19048ae7a  ff15e840aff4                    call    cs:qword_184F7EF68
0x19048ae80  84c0                            test    al, al
0x19048ae82  750e                            jnz     short loc_19048AE92
0x19048ae84  b902000000                      mov     ecx, 2
0x19048ae89  e8324c7f02                      call    sub_192C7FAC0
0x19048ae8e  84c0                            test    al, al
0x19048ae90  744e                            jz      short loc_19048AEE0
0x19048ae92  c6463900                        mov     byte ptr [rsi+39h], 0
0x19048ae96  488b7e20                        mov     rdi, [rsi+20h]
0x19048ae9a  4531e4                          xor     r12d, r12d
0x19048ae9d  4885ff                          test    rdi, rdi
0x19048aea0  0f846dfbffff                    jz      loc_19048AA13
0x19048aea6  c744243800000000                mov     dword ptr [rsp+128h+var_F8+8], 0
0x19048aeae  48c744243000000000              mov     qword ptr [rsp+128h+var_F8], 0
0x19048aeb7  488d4c2430                      lea     rcx, [rsp+128h+var_F8]
0x19048aebc  ff150e41aff4                    call    cs:qword_184F7EFD0
0x19048aec2  4c8b442430                      mov     r8, qword ptr [rsp+128h+var_F8]
0x19048aec7  4c8b0db229ccf4                  mov     r9, cs:qword_18514D880
0x19048aece  4889f9                          mov     rcx, rdi
0x19048aed1  ba04ffffff                      mov     edx, 0FFFFFF04h
0x19048aed6  e8f533d4fc                      call    sub_18D1CE2D0
0x19048aedb  e933fbffff                      jmp     loc_19048AA13
0x19048aee0  c6463900                        mov     byte ptr [rsi+39h], 0
0x19048aee4  4531e4                          xor     r12d, r12d
0x19048aee7  e927fbffff                      jmp     loc_19048AA13
0x19048aeec  b920e80000                      mov     ecx, 0E820h
0x19048aef1  e82a0e6a02                      call    sub_192B2BD20
0x19048aef6  4885c0                          test    rax, rax
0x19048aef9  7465                            jz      short loc_19048AF60
0x19048aefb  4889c1                          mov     rcx, rax
0x19048aefe  31d2                            xor     edx, edx
0x19048af00  e83bb6defd                      call    sub_18E276540
0x19048af05  84c0                            test    al, al
0x19048af07  0f845bffffff                    jz      loc_19048AE68
0x19048af0d  eb83                            jmp     short loc_19048AE92
0x19048af0f  b920e80000                      mov     ecx, 0E820h
0x19048af14  e8070e6a02                      call    sub_192B2BD20
0x19048af19  4885c0                          test    rax, rax
0x19048af1c  7447                            jz      short loc_19048AF65
0x19048af1e  4889c1                          mov     rcx, rax
0x19048af21  ba01000000                      mov     edx, 1
0x19048af26  e815b6defd                      call    sub_18E276540
0x19048af2b  84c0                            test    al, al
0x19048af2d  0f8451ffffff                    jz      loc_19048AE84
0x19048af33  e95affffff                      jmp     loc_19048AE92
0x19048af38  e823a44af0                      call    sub_180935360
0x19048af3d  e81ea44af0                      call    sub_180935360
0x19048af42  e819a44af0                      call    sub_180935360
0x19048af47  e814a44af0                      call    sub_180935360
0x19048af4c  e80fa44af0                      call    sub_180935360
0x19048af51  e80aa44af0                      call    sub_180935360
0x19048af56  e805a44af0                      call    sub_180935360
0x19048af5b  e800a44af0                      call    sub_180935360
0x19048af60  e8fba34af0                      call    sub_180935360
0x19048af65  e8f6a34af0                      call    sub_180935360
