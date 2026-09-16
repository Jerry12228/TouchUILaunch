0x195b46680  56                              push    rsi
0x195b46681  57                              push    rdi
0x195b46682  4883ec38                        sub     rsp, 38h
0x195b46686  89ce                            mov     esi, ecx
0x195b46688  803d56eed4ef00                  cmp     cs:byte_1858954E5, 0
0x195b4668f  0f84d7000000                    jz      loc_195B4676C
0x195b46695  803d624086ef00                  cmp     cs:byte_1853AA6FE, 0
0x195b4669c  0f85e8000000                    jnz     loc_195B4678A
0x195b466a2  488b0d07a890ef                  mov     rcx, cs:qword_185450EB0
0x195b466a9  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x195b466b0  0f84f3000000                    jz      loc_195B467A9
0x195b466b6  488b0573a081ef                  mov     rax, cs:qword_185360730
0x195b466bd  488b80c0d90200                  mov     rax, [rax+2D9C0h]
0x195b466c4  4885c0                          test    rax, rax
0x195b466c7  0f84f8000000                    jz      loc_195B467C5
0x195b466cd  488bb880000000                  mov     rdi, [rax+80h]
0x195b466d4  4885ff                          test    rdi, rdi
0x195b466d7  0f84ed000000                    jz      loc_195B467CA
0x195b466dd  4c8b05bc7d95ef                  mov     r8, cs:qword_18549E4A0
0x195b466e4  488b07                          mov     rax, [rdi]
0x195b466e7  0fb788c6000000                  movzx   ecx, word ptr [rax+0C6h]
0x195b466ee  4885c9                          test    rcx, rcx
0x195b466f1  741c                            jz      short loc_195B4670F
0x195b466f3  488b5010                        mov     rdx, [rax+10h]
0x195b466f7  48c1e104                        shl     rcx, 4
0x195b466fb  4531c9                          xor     r9d, r9d
0x195b466fe  6690                            xchg    ax, ax
0x195b46700  4e39040a                        cmp     [rdx+r9], r8
0x195b46704  7428                            jz      short loc_195B4672E
0x195b46706  4983c110                        add     r9, 10h
0x195b4670a  4c39c9                          cmp     rcx, r9
0x195b4670d  75f1                            jnz     short loc_195B46700
0x195b4670f  488d4c2428                      lea     rcx, [rsp+48h+var_20]
0x195b46714  4889fa                          mov     rdx, rdi
0x195b46717  41b901000000                    mov     r9d, 1
0x195b4671d  e84e8a72ea                      call    sub_18026F170
0x195b46722  4c8b4c2428                      mov     r9, [rsp+48h+var_20]
0x195b46727  4c8b442430                      mov     r8, [rsp+48h+var_18]
0x195b4672c  eb2e                            jmp     short loc_195B4675C
0x195b4672e  428b4c0a08                      mov     ecx, [rdx+r9+8]
0x195b46733  ffc1                            inc     ecx
0x195b46735  4863c9                          movsxd  rcx, ecx
0x195b46738  4c8b8cc8d0000000                mov     r9, [rax+rcx*8+0D0h]
0x195b46740  4c894c2428                      mov     [rsp+48h+var_20], r9
0x195b46745  0fb790c2000000                  movzx   edx, word ptr [rax+0C2h]
0x195b4674c  4801ca                          add     rdx, rcx
0x195b4674f  4c8b84d0d0000000                mov     r8, [rax+rdx*8+0D0h]
0x195b46757  4c89442430                      mov     [rsp+48h+var_18], r8
0x195b4675c  4889f9                          mov     rcx, rdi
0x195b4675f  89f2                            mov     edx, esi
0x195b46761  41ffd1                          call    r9
0x195b46764  90                              nop
0x195b46765  4883c438                        add     rsp, 38h
0x195b46769  5f                              pop     rdi
0x195b4676a  5e                              pop     rsi
0x195b4676b  c3                              retn
0x195b4676c  b9f53a0400                      mov     ecx, 43AF5h
0x195b46771  e86a2c73ea                      call    sub_1802793E0
0x195b46776  c60568edd4ef01                  mov     cs:byte_1858954E5, 1
0x195b4677d  803d7a3f86ef00                  cmp     cs:byte_1853AA6FE, 0
0x195b46784  0f8418ffffff                    jz      loc_195B466A2
0x195b4678a  b94ed40000                      mov     ecx, 0D44Eh
0x195b4678f  e8cc6f63fa                      call    sub_19017D760
0x195b46794  4885c0                          test    rax, rax
0x195b46797  7436                            jz      short loc_195B467CF
0x195b46799  4889c1                          mov     rcx, rax
0x195b4679c  89f2                            mov     edx, esi
0x195b4679e  4883c438                        add     rsp, 38h
0x195b467a2  5f                              pop     rdi
0x195b467a3  5e                              pop     rsi
0x195b467a4  e927d967f5                      jmp     sub_18B1C40D0
0x195b467a9  e8a2b672ea                      call    sub_180271E50
0x195b467ae  488b057b9f81ef                  mov     rax, cs:qword_185360730
0x195b467b5  488b80c0d90200                  mov     rax, [rax+2D9C0h]
0x195b467bc  4885c0                          test    rax, rax
0x195b467bf  0f8508ffffff                    jnz     loc_195B466CD
0x195b467c5  e85629f8ea                      call    sub_180AC9120
0x195b467ca  e85129f8ea                      call    sub_180AC9120
0x195b467cf  e84c29f8ea                      call    sub_180AC9120
