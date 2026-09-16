0x18b109140  56                              push    rsi
0x18b109141  57                              push    rdi
0x18b109142  53                              push    rbx
0x18b109143  4883ec30                        sub     rsp, 30h
0x18b109147  89cb                            mov     ebx, ecx
0x18b109149  803dc72bbcf900                  cmp     cs:byte_184CCBD17, 0
0x18b109150  0f84d5000000                    jz      loc_18B10922B
0x18b109156  803d5b467ff900                  cmp     cs:byte_1848FD7B8, 0
0x18b10915d  0f85e6000000                    jnz     loc_18B109249
0x18b109163  488b0d267d84f9                  mov     rcx, cs:qword_184950E90
0x18b10916a  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x18b109171  0f840a010000                    jz      loc_18B109281
0x18b109177  488b05e2ca77f9                  mov     rax, cs:qword_184885C60
0x18b10917e  488b80a8950200                  mov     rax, [rax+295A8h]
0x18b109185  4885c0                          test    rax, rax
0x18b109188  0f840f010000                    jz      loc_18B10929D
0x18b10918e  488b7870                        mov     rdi, [rax+70h]
0x18b109192  4885ff                          test    rdi, rdi
0x18b109195  0f8407010000                    jz      loc_18B1092A2
0x18b10919b  4c8b05a65e85f9                  mov     r8, cs:qword_18495F048
0x18b1091a2  488b07                          mov     rax, [rdi]
0x18b1091a5  0fb788c6000000                  movzx   ecx, word ptr [rax+0C6h]
0x18b1091ac  4885c9                          test    rcx, rcx
0x18b1091af  741e                            jz      short loc_18B1091CF
0x18b1091b1  488b5030                        mov     rdx, [rax+30h]
0x18b1091b5  48c1e104                        shl     rcx, 4
0x18b1091b9  31f6                            xor     esi, esi
0x18b1091bb  0f1f440000                      nop     dword ptr [rax+rax+00h]
0x18b1091c0  4c390432                        cmp     [rdx+rsi], r8
0x18b1091c4  7428                            jz      short loc_18B1091EE
0x18b1091c6  4883c610                        add     rsi, 10h
0x18b1091ca  4839f1                          cmp     rcx, rsi
0x18b1091cd  75f1                            jnz     short loc_18B1091C0
0x18b1091cf  488d4c2420                      lea     rcx, [rsp+48h+var_28]
0x18b1091d4  4889fa                          mov     rdx, rdi
0x18b1091d7  41b901000000                    mov     r9d, 1
0x18b1091dd  e80e8812f5                      call    sub_1802319F0
0x18b1091e2  488b742420                      mov     rsi, [rsp+48h+var_28]
0x18b1091e7  4c8b442428                      mov     r8, [rsp+48h+var_20]
0x18b1091ec  eb2d                            jmp     short loc_18B10921B
0x18b1091ee  8b4c3208                        mov     ecx, [rdx+rsi+8]
0x18b1091f2  ffc1                            inc     ecx
0x18b1091f4  4863c9                          movsxd  rcx, ecx
0x18b1091f7  488bb4c8d0000000                mov     rsi, [rax+rcx*8+0D0h]
0x18b1091ff  4889742420                      mov     [rsp+48h+var_28], rsi
0x18b109204  0fb790c2000000                  movzx   edx, word ptr [rax+0C2h]
0x18b10920b  4801ca                          add     rdx, rcx
0x18b10920e  4c8b84d0d0000000                mov     r8, [rax+rdx*8+0D0h]
0x18b109216  4c89442428                      mov     [rsp+48h+var_20], r8
0x18b10921b  4889f9                          mov     rcx, rdi
0x18b10921e  89da                            mov     edx, ebx
0x18b109220  ffd6                            call    rsi
0x18b109222  90                              nop
0x18b109223  4883c430                        add     rsp, 30h
0x18b109227  5b                              pop     rbx
0x18b109228  5f                              pop     rdi
0x18b109229  5e                              pop     rsi
0x18b10922a  c3                              retn
0x18b10922b  b957da0100                      mov     ecx, 1DA57h
0x18b109230  e8fb2913f5                      call    sub_18023BC30
0x18b109235  c605db2abcf901                  mov     cs:byte_184CCBD17, 1
0x18b10923c  803d75457ff900                  cmp     cs:byte_1848FD7B8, 0
0x18b109243  0f841affffff                    jz      loc_18B109163
0x18b109249  488b0dc8be87f9                  mov     rcx, cs:qword_184985118
0x18b109250  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x18b109257  744e                            jz      short loc_18B1092A7
0x18b109259  488b0500ca77f9                  mov     rax, cs:qword_184885C60
0x18b109260  488b8070e40300                  mov     rax, [rax+3E470h]
0x18b109267  488b88e0012100                  mov     rcx, [rax+2101E0h]
0x18b10926e  4885c9                          test    rcx, rcx
0x18b109271  743b                            jz      short loc_18B1092AE
0x18b109273  89da                            mov     edx, ebx
0x18b109275  4883c430                        add     rsp, 30h
0x18b109279  5b                              pop     rbx
0x18b10927a  5f                              pop     rdi
0x18b10927b  5e                              pop     rsi
0x18b10927c  e97fd9f2fb                      jmp     sub_187036C00
0x18b109281  e8eab412f5                      call    sub_180234770
0x18b109286  488b05d3c977f9                  mov     rax, cs:qword_184885C60
0x18b10928d  488b80a8950200                  mov     rax, [rax+295A8h]
0x18b109294  4885c0                          test    rax, rax
0x18b109297  0f85f1feffff                    jnz     loc_18B10918E
0x18b10929d  e8be0487f5                      call    sub_180979760
0x18b1092a2  e8b90487f5                      call    sub_180979760
0x18b1092a7  e8c4b412f5                      call    sub_180234770
0x18b1092ac  ebab                            jmp     short loc_18B109259
0x18b1092ae  e8ad0487f5                      call    sub_180979760
