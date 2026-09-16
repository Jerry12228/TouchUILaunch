0x188125a70  56                              push    rsi
0x188125a71  57                              push    rdi
0x188125a72  53                              push    rbx
0x188125a73  4883ec20                        sub     rsp, 20h
0x188125a77  89d3                            mov     ebx, edx
0x188125a79  4889ce                          mov     rsi, rcx
0x188125a7c  803d64e6bafc00                  cmp     cs:byte_184CD40E7, 0
0x188125a83  0f841d010000                    jz      loc_188125BA6
0x188125a89  803d72477dfc00                  cmp     cs:byte_1848FA202, 0
0x188125a90  0f852e010000                    jnz     loc_188125BC4
0x188125a96  84db                            test    bl, bl
0x188125a98  0f848c000000                    jz      loc_188125B2A
0x188125a9e  488b0debb382fc                  mov     rcx, cs:qword_184950E90
0x188125aa5  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x188125aac  0f844e010000                    jz      loc_188125C00
0x188125ab2  31c9                            xor     ecx, ecx
0x188125ab4  31d2                            xor     edx, edx
0x188125ab6  4531c0                          xor     r8d, r8d
0x188125ab9  e8f2f3fd02                      call    sub_18B104EB0
0x188125abe  bf01000000                      mov     edi, 1
0x188125ac3  84c0                            test    al, al
0x188125ac5  0f85d1000000                    jnz     loc_188125B9C
0x188125acb  488b0dbeb382fc                  mov     rcx, cs:qword_184950E90
0x188125ad2  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x188125ad9  0f8435010000                    jz      loc_188125C14
0x188125adf  31c9                            xor     ecx, ecx
0x188125ae1  e8daa81105                      call    sub_18D2403C0
0x188125ae6  bf03000000                      mov     edi, 3
0x188125aeb  84c0                            test    al, al
0x188125aed  0f85a9000000                    jnz     loc_188125B9C
0x188125af3  488b0d96b382fc                  mov     rcx, cs:qword_184950E90
0x188125afa  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x188125b01  0f8421010000                    jz      loc_188125C28
0x188125b07  31c9                            xor     ecx, ecx
0x188125b09  e822e0fd02                      call    sub_18B103B30
0x188125b0e  8bbe68010000                    mov     edi, [rsi+168h]
0x188125b14  84c0                            test    al, al
0x188125b16  0f8480000000                    jz      loc_188125B9C
0x188125b1c  31c0                            xor     eax, eax
0x188125b1e  83ff02                          cmp     edi, 2
0x188125b21  0f94c0                          setz    al
0x188125b24  01c0                            add     eax, eax
0x188125b26  89c7                            mov     edi, eax
0x188125b28  eb72                            jmp     short loc_188125B9C
0x188125b2a  488b0d5fb382fc                  mov     rcx, cs:qword_184950E90
0x188125b31  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x188125b38  0f84cc000000                    jz      loc_188125C0A
0x188125b3e  31c9                            xor     ecx, ecx
0x188125b40  31d2                            xor     edx, edx
0x188125b42  4531c0                          xor     r8d, r8d
0x188125b45  e866f3fd02                      call    sub_18B104EB0
0x188125b4a  bf01000000                      mov     edi, 1
0x188125b4f  84c0                            test    al, al
0x188125b51  7549                            jnz     short loc_188125B9C
0x188125b53  488b0d36b382fc                  mov     rcx, cs:qword_184950E90
0x188125b5a  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x188125b61  0f84b7000000                    jz      loc_188125C1E
0x188125b67  31c9                            xor     ecx, ecx
0x188125b69  e852a81105                      call    sub_18D2403C0
0x188125b6e  bf03000000                      mov     edi, 3
0x188125b73  84c0                            test    al, al
0x188125b75  7525                            jnz     short loc_188125B9C
0x188125b77  488b0d12b382fc                  mov     rcx, cs:qword_184950E90
0x188125b7e  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x188125b85  0f84a7000000                    jz      loc_188125C32
0x188125b8b  31ff                            xor     edi, edi
0x188125b8d  31c9                            xor     ecx, ecx
0x188125b8f  e89cdffd02                      call    sub_18B103B30
0x188125b94  84c0                            test    al, al
0x188125b96  0f8402ffffff                    jz      loc_188125A9E
0x188125b9c  89f8                            mov     eax, edi
0x188125b9e  4883c420                        add     rsp, 20h
0x188125ba2  5b                              pop     rbx
0x188125ba3  5f                              pop     rdi
0x188125ba4  5e                              pop     rsi
0x188125ba5  c3                              retn
0x188125ba6  b9275e0200                      mov     ecx, 25E27h
0x188125bab  e8806011f8                      call    sub_18023BC30
0x188125bb0  c60530e5bafc01                  mov     cs:byte_184CD40E7, 1
0x188125bb7  803d44467dfc00                  cmp     cs:byte_1848FA202, 0
0x188125bbe  0f84d2feffff                    jz      loc_188125A96
0x188125bc4  488b0d4df585fc                  mov     rcx, cs:qword_184985118
0x188125bcb  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x188125bd2  7468                            jz      short loc_188125C3C
0x188125bd4  488b05850076fc                  mov     rax, cs:qword_184885C60
0x188125bdb  488b8070e40300                  mov     rax, [rax+3E470h]
0x188125be2  488b8830541f00                  mov     rcx, [rax+1F5430h]
0x188125be9  4885c9                          test    rcx, rcx
0x188125bec  7455                            jz      short loc_188125C43
0x188125bee  4889f2                          mov     rdx, rsi
0x188125bf1  4189d8                          mov     r8d, ebx
0x188125bf4  4883c420                        add     rsp, 20h
0x188125bf8  5b                              pop     rbx
0x188125bf9  5f                              pop     rdi
0x188125bfa  5e                              pop     rsi
0x188125bfb  e96035f3fe                      jmp     sub_187059160
0x188125c00  e86beb10f8                      call    sub_180234770
0x188125c05  e9a8feffff                      jmp     loc_188125AB2
0x188125c0a  e861eb10f8                      call    sub_180234770
0x188125c0f  e92affffff                      jmp     loc_188125B3E
0x188125c14  e857eb10f8                      call    sub_180234770
0x188125c19  e9c1feffff                      jmp     loc_188125ADF
0x188125c1e  e84deb10f8                      call    sub_180234770
0x188125c23  e93fffffff                      jmp     loc_188125B67
0x188125c28  e843eb10f8                      call    sub_180234770
0x188125c2d  e9d5feffff                      jmp     loc_188125B07
0x188125c32  e839eb10f8                      call    sub_180234770
0x188125c37  e94fffffff                      jmp     loc_188125B8B
0x188125c3c  e82feb10f8                      call    sub_180234770
0x188125c41  eb91                            jmp     short loc_188125BD4
0x188125c43  e8183b85f8                      call    sub_180979760
