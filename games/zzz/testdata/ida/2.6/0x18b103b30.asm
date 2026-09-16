0x18b103b30  56                              push    rsi
0x18b103b31  4883ec20                        sub     rsp, 20h
0x18b103b35  89ce                            mov     esi, ecx
0x18b103b37  803db781bcf900                  cmp     cs:byte_184CCBCF5, 0
0x18b103b3e  7430                            jz      short loc_18B103B70
0x18b103b40  803d499c7ff900                  cmp     cs:byte_1848FD790, 0
0x18b103b47  7541                            jnz     short loc_18B103B8A
0x18b103b49  85f6                            test    esi, esi
0x18b103b4b  7517                            jnz     short loc_18B103B64
0x18b103b4d  488b0d3cd384f9                  mov     rcx, cs:qword_184950E90
0x18b103b54  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x18b103b5b  7463                            jz      short loc_18B103BC0
0x18b103b5d  e87e000000                      call    sub_18B103BE0
0x18b103b62  89c6                            mov     esi, eax
0x18b103b64  83fe02                          cmp     esi, 2
0x18b103b67  0f94c0                          setz    al
0x18b103b6a  4883c420                        add     rsp, 20h
0x18b103b6e  5e                              pop     rsi
0x18b103b6f  c3                              retn
0x18b103b70  b935da0100                      mov     ecx, 1DA35h
0x18b103b75  e8b68013f5                      call    sub_18023BC30
0x18b103b7a  c6057481bcf901                  mov     cs:byte_184CCBCF5, 1
0x18b103b81  803d089c7ff900                  cmp     cs:byte_1848FD790, 0
0x18b103b88  74bf                            jz      short loc_18B103B49
0x18b103b8a  488b0d871588f9                  mov     rcx, cs:qword_184985118
0x18b103b91  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x18b103b98  742d                            jz      short loc_18B103BC7
0x18b103b9a  488b05bf2078f9                  mov     rax, cs:qword_184885C60
0x18b103ba1  488b8070e40300                  mov     rax, [rax+3E470h]
0x18b103ba8  488b88a0002100                  mov     rcx, [rax+2100A0h]
0x18b103baf  4885c9                          test    rcx, rcx
0x18b103bb2  741a                            jz      short loc_18B103BCE
0x18b103bb4  89f2                            mov     edx, esi
0x18b103bb6  4883c420                        add     rsp, 20h
0x18b103bba  5e                              pop     rsi
0x18b103bbb  e90038f5fb                      jmp     sub_1870573C0
0x18b103bc0  e8ab0b13f5                      call    sub_180234770
0x18b103bc5  eb96                            jmp     short loc_18B103B5D
0x18b103bc7  e8a40b13f5                      call    sub_180234770
0x18b103bcc  ebcc                            jmp     short loc_18B103B9A
0x18b103bce  e88d5b87f5                      call    sub_180979760
