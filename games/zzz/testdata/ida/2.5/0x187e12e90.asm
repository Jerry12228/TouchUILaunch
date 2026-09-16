0x187e12e90  56                              push    rsi
0x187e12e91  57                              push    rdi
0x187e12e92  53                              push    rbx
0x187e12e93  4883ec20                        sub     rsp, 20h
0x187e12e97  89d3                            mov     ebx, edx
0x187e12e99  4889ce                          mov     rsi, rcx
0x187e12e9c  803dc87850fd00                  cmp     cs:byte_18531A76B, 0
0x187e12ea3  0f841d010000                    jz      loc_187E12FC6
0x187e12ea9  803dc47610fd00                  cmp     cs:byte_184F1A574, 0
0x187e12eb0  0f852e010000                    jnz     loc_187E12FE4
0x187e12eb6  84db                            test    bl, bl
0x187e12eb8  0f848c000000                    jz      loc_187E12F4A
0x187e12ebe  488b0ddb5017fd                  mov     rcx, cs:qword_184F87FA0
0x187e12ec5  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x187e12ecc  0f844e010000                    jz      loc_187E13020
0x187e12ed2  31c9                            xor     ecx, ecx
0x187e12ed4  31d2                            xor     edx, edx
0x187e12ed6  4531c0                          xor     r8d, r8d
0x187e12ed9  e892d0a5ff                      call    sub_18786FF70
0x187e12ede  bf01000000                      mov     edi, 1
0x187e12ee3  84c0                            test    al, al
0x187e12ee5  0f85d1000000                    jnz     loc_187E12FBC
0x187e12eeb  488b0dae5017fd                  mov     rcx, cs:qword_184F87FA0
0x187e12ef2  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x187e12ef9  0f8435010000                    jz      loc_187E13034
0x187e12eff  31c9                            xor     ecx, ecx
0x187e12f01  e80acfa5ff                      call    sub_18786FE10
0x187e12f06  bf03000000                      mov     edi, 3
0x187e12f0b  84c0                            test    al, al
0x187e12f0d  0f85a9000000                    jnz     loc_187E12FBC
0x187e12f13  488b0d865017fd                  mov     rcx, cs:qword_184F87FA0
0x187e12f1a  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x187e12f21  0f8421010000                    jz      loc_187E13048
0x187e12f27  31c9                            xor     ecx, ecx
0x187e12f29  e892cfa5ff                      call    sub_18786FEC0
0x187e12f2e  8bbe64010000                    mov     edi, [rsi+164h]
0x187e12f34  84c0                            test    al, al
0x187e12f36  0f8480000000                    jz      loc_187E12FBC
0x187e12f3c  31c0                            xor     eax, eax
0x187e12f3e  83ff02                          cmp     edi, 2
0x187e12f41  0f94c0                          setz    al
0x187e12f44  01c0                            add     eax, eax
0x187e12f46  89c7                            mov     edi, eax
0x187e12f48  eb72                            jmp     short loc_187E12FBC
0x187e12f4a  488b0d4f5017fd                  mov     rcx, cs:qword_184F87FA0
0x187e12f51  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x187e12f58  0f84cc000000                    jz      loc_187E1302A
0x187e12f5e  31c9                            xor     ecx, ecx
0x187e12f60  31d2                            xor     edx, edx
0x187e12f62  4531c0                          xor     r8d, r8d
0x187e12f65  e806d0a5ff                      call    sub_18786FF70
0x187e12f6a  bf01000000                      mov     edi, 1
0x187e12f6f  84c0                            test    al, al
0x187e12f71  7549                            jnz     short loc_187E12FBC
0x187e12f73  488b0d265017fd                  mov     rcx, cs:qword_184F87FA0
0x187e12f7a  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x187e12f81  0f84b7000000                    jz      loc_187E1303E
0x187e12f87  31c9                            xor     ecx, ecx
0x187e12f89  e882cea5ff                      call    sub_18786FE10
0x187e12f8e  bf03000000                      mov     edi, 3
0x187e12f93  84c0                            test    al, al
0x187e12f95  7525                            jnz     short loc_187E12FBC
0x187e12f97  488b0d025017fd                  mov     rcx, cs:qword_184F87FA0
0x187e12f9e  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x187e12fa5  0f84a7000000                    jz      loc_187E13052
0x187e12fab  31ff                            xor     edi, edi
0x187e12fad  31c9                            xor     ecx, ecx
0x187e12faf  e80ccfa5ff                      call    sub_18786FEC0
0x187e12fb4  84c0                            test    al, al
0x187e12fb6  0f8402ffffff                    jz      loc_187E12EBE
0x187e12fbc  89f8                            mov     eax, edi
0x187e12fbe  4883c420                        add     rsp, 20h
0x187e12fc2  5b                              pop     rbx
0x187e12fc3  5f                              pop     rdi
0x187e12fc4  5e                              pop     rsi
0x187e12fc5  c3                              retn
0x187e12fc6  b99b800300                      mov     ecx, 3809Bh
0x187e12fcb  e8808343f8                      call    sub_18024B350
0x187e12fd0  c605947750fd01                  mov     cs:byte_18531A76B, 1
0x187e12fd7  803d967510fd00                  cmp     cs:byte_184F1A574, 0
0x187e12fde  0f84d2feffff                    jz      loc_187E12EB6
0x187e12fe4  488b0d55761afd                  mov     rcx, cs:qword_184FBA640
0x187e12feb  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x187e12ff2  7468                            jz      short loc_187E1305C
0x187e12ff4  488b054d0b08fd                  mov     rax, cs:qword_184E93B48
0x187e12ffb  488b80a0240500                  mov     rax, [rax+524A0h]
0x187e13002  488b88c06d1700                  mov     rcx, [rax+176DC0h]
0x187e13009  4885c9                          test    rcx, rcx
0x187e1300c  7455                            jz      short loc_187E13063
0x187e1300e  4889f2                          mov     rdx, rsi
0x187e13011  4189d8                          mov     r8d, ebx
0x187e13014  4883c420                        add     rsp, 20h
0x187e13018  5b                              pop     rbx
0x187e13019  5f                              pop     rdi
0x187e1301a  5e                              pop     rsi
0x187e1301b  e96001feff                      jmp     sub_187DF3180
0x187e13020  e8cb0e43f8                      call    sub_180243EF0
0x187e13025  e9a8feffff                      jmp     loc_187E12ED2
0x187e1302a  e8c10e43f8                      call    sub_180243EF0
0x187e1302f  e92affffff                      jmp     loc_187E12F5E
0x187e13034  e8b70e43f8                      call    sub_180243EF0
0x187e13039  e9c1feffff                      jmp     loc_187E12EFF
0x187e1303e  e8ad0e43f8                      call    sub_180243EF0
0x187e13043  e93fffffff                      jmp     loc_187E12F87
0x187e13048  e8a30e43f8                      call    sub_180243EF0
0x187e1304d  e9d5feffff                      jmp     loc_187E12F27
0x187e13052  e8990e43f8                      call    sub_180243EF0
0x187e13057  e94fffffff                      jmp     loc_187E12FAB
0x187e1305c  e88f0e43f8                      call    sub_180243EF0
0x187e13061  eb91                            jmp     short loc_187E12FF4
0x187e13063  e8d8e3d3f8                      call    sub_180B51440
