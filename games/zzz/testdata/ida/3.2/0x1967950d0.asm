0x1967950d0  56                              push    rsi
0x1967950d1  57                              push    rdi
0x1967950d2  53                              push    rbx
0x1967950d3  4883ec20                        sub     rsp, 20h
0x1967950d7  89d3                            mov     ebx, edx
0x1967950d9  4889ce                          mov     rsi, rcx
0x1967950dc  803d315bc2ee00                  cmp     cs:byte_1853BAC14, 0
0x1967950e3  0f842e020000                    jz      loc_196795317
0x1967950e9  803db50579ee00                  cmp     cs:byte_184F256A5, 0
0x1967950f0  0f853f020000                    jnz     loc_196795335
0x1967950f6  84db                            test    bl, bl
0x1967950f8  0f8414010000                    jz      loc_196795212
0x1967950fe  488b0ddbc282ee                  mov     rcx, cs:qword_184FC13E0
0x196795105  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x19679510c  0f844b020000                    jz      loc_19679535D
0x196795112  803d3f7ac3ee00                  cmp     cs:byte_1853CCB58, 0
0x196795119  0f8450020000                    jz      loc_19679536F
0x19679511f  803dcb877bee00                  cmp     cs:byte_184F4D8F1, 0
0x196795126  0f8561020000                    jnz     loc_19679538D
0x19679512c  488b0dadc282ee                  mov     rcx, cs:qword_184FC13E0
0x196795133  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x19679513a  0f8482020000                    jz      loc_1967953C2
0x196795140  e80b30d3fc                      call    sub_1934C8150
0x196795145  83c0fd                          add     eax, 0FFFFFFFDh
0x196795148  bf01000000                      mov     edi, 1
0x19679514d  83f802                          cmp     eax, 2
0x196795150  0f82b7010000                    jb      loc_19679530D
0x196795156  488b0d83c282ee                  mov     rcx, cs:qword_184FC13E0
0x19679515d  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x196795164  0f84d1020000                    jz      loc_19679543B
0x19679516a  803d547ac3ee00                  cmp     cs:byte_1853CCBC5, 0
0x196795171  0f84d6020000                    jz      loc_19679544D
0x196795177  803de8877bee00                  cmp     cs:byte_184F4D966, 0
0x19679517e  0f85e7020000                    jnz     loc_19679546B
0x196795184  488b0d55c282ee                  mov     rcx, cs:qword_184FC13E0
0x19679518b  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x196795192  0f8402030000                    jz      loc_19679549A
0x196795198  e8b32fd3fc                      call    sub_1934C8150
0x19679519d  bf03000000                      mov     edi, 3
0x1967951a2  83f801                          cmp     eax, 1
0x1967951a5  0f8462010000                    jz      loc_19679530D
0x1967951ab  488b0d2ec282ee                  mov     rcx, cs:qword_184FC13E0
0x1967951b2  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x1967951b9  0f844e030000                    jz      loc_19679550D
0x1967951bf  803d7f79c3ee00                  cmp     cs:byte_1853CCB45, 0
0x1967951c6  0f8453030000                    jz      loc_19679551F
0x1967951cc  803d0b877bee00                  cmp     cs:byte_184F4D8DE, 0
0x1967951d3  0f8564030000                    jnz     loc_19679553D
0x1967951d9  488b0d00c282ee                  mov     rcx, cs:qword_184FC13E0
0x1967951e0  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x1967951e7  0f8480030000                    jz      loc_19679556D
0x1967951ed  e85e2fd3fc                      call    sub_1934C8150
0x1967951f2  8bbe88010000                    mov     edi, [rsi+188h]
0x1967951f8  83f802                          cmp     eax, 2
0x1967951fb  0f850c010000                    jnz     loc_19679530D
0x196795201  31c0                            xor     eax, eax
0x196795203  83ff02                          cmp     edi, 2
0x196795206  0f94c0                          setz    al
0x196795209  01c0                            add     eax, eax
0x19679520b  89c7                            mov     edi, eax
0x19679520d  e9fb000000                      jmp     loc_19679530D
0x196795212  488b0dc7c182ee                  mov     rcx, cs:qword_184FC13E0
0x196795219  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x196795220  0f84a6010000                    jz      loc_1967953CC
0x196795226  803d2b79c3ee00                  cmp     cs:byte_1853CCB58, 0
0x19679522d  0f84ab010000                    jz      loc_1967953DE
0x196795233  803db7867bee00                  cmp     cs:byte_184F4D8F1, 0
0x19679523a  0f85bc010000                    jnz     loc_1967953FC
0x196795240  488b0d99c182ee                  mov     rcx, cs:qword_184FC13E0
0x196795247  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x19679524e  0f84dd010000                    jz      loc_196795431
0x196795254  e8f72ed3fc                      call    sub_1934C8150
0x196795259  83c0fd                          add     eax, 0FFFFFFFDh
0x19679525c  bf01000000                      mov     edi, 1
0x196795261  83f802                          cmp     eax, 2
0x196795264  0f82a3000000                    jb      loc_19679530D
0x19679526a  488b0d6fc182ee                  mov     rcx, cs:qword_184FC13E0
0x196795271  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x196795278  0f8426020000                    jz      loc_1967954A4
0x19679527e  803d4079c3ee00                  cmp     cs:byte_1853CCBC5, 0
0x196795285  0f842b020000                    jz      loc_1967954B6
0x19679528b  803dd4867bee00                  cmp     cs:byte_184F4D966, 0
0x196795292  0f853c020000                    jnz     loc_1967954D4
0x196795298  488b0d41c182ee                  mov     rcx, cs:qword_184FC13E0
0x19679529f  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x1967952a6  0f8457020000                    jz      loc_196795503
0x1967952ac  e89f2ed3fc                      call    sub_1934C8150
0x1967952b1  bf03000000                      mov     edi, 3
0x1967952b6  83f801                          cmp     eax, 1
0x1967952b9  7452                            jz      short loc_19679530D
0x1967952bb  488b0d1ec182ee                  mov     rcx, cs:qword_184FC13E0
0x1967952c2  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x1967952c9  0f84a8020000                    jz      loc_196795577
0x1967952cf  803d6f78c3ee00                  cmp     cs:byte_1853CCB45, 0
0x1967952d6  0f84ad020000                    jz      loc_196795589
0x1967952dc  803dfb857bee00                  cmp     cs:byte_184F4D8DE, 0
0x1967952e3  0f85be020000                    jnz     loc_1967955A7
0x1967952e9  488b0df0c082ee                  mov     rcx, cs:qword_184FC13E0
0x1967952f0  80b9cb00000000                  cmp     byte ptr [rcx+0CBh], 0
0x1967952f7  0f84d2020000                    jz      loc_1967955CF
0x1967952fd  e84e2ed3fc                      call    sub_1934C8150
0x196795302  31ff                            xor     edi, edi
0x196795304  83f802                          cmp     eax, 2
0x196795307  0f85f1fdffff                    jnz     loc_1967950FE
0x19679530d  89f8                            mov     eax, edi
0x19679530f  4883c420                        add     rsp, 20h
0x196795313  5b                              pop     rbx
0x196795314  5f                              pop     rdi
0x196795315  5e                              pop     rsi
0x196795316  c3                              retn
0x196795317  b9d4830200                      mov     ecx, 283D4h
0x19679531c  e87f74afe9                      call    sub_18028C7A0
0x196795321  c605ec58c2ee01                  mov     cs:byte_1853BAC14, 1
0x196795328  803d760379ee00                  cmp     cs:byte_184F256A5, 0
0x19679532f  0f84c1fdffff                    jz      loc_1967950F6
0x196795335  b9e5a60100                      mov     ecx, 1A6E5h
0x19679533a  e8e16939fc                      call    sub_192B2BD20
0x19679533f  4885c0                          test    rax, rax
0x196795342  0f8491020000                    jz      loc_1967955D9
0x196795348  4889c1                          mov     rcx, rax
0x19679534b  4889f2                          mov     rdx, rsi
0x19679534e  4189d8                          mov     r8d, ebx
0x196795351  4883c420                        add     rsp, 20h
0x196795355  5b                              pop     rbx
0x196795356  5f                              pop     rdi
0x196795357  5e                              pop     rsi
0x196795358  e93372b0f7                      jmp     sub_18E29C590
0x19679535d  e88efeaee9                      call    sub_1802851F0
0x196795362  803def77c3ee00                  cmp     cs:byte_1853CCB58, 0
0x196795369  0f85b0fdffff                    jnz     loc_19679511F
0x19679536f  b918a30300                      mov     ecx, 3A318h
0x196795374  e82774afe9                      call    sub_18028C7A0
0x196795379  c605d877c3ee01                  mov     cs:byte_1853CCB58, 1
0x196795380  803d6a857bee00                  cmp     cs:byte_184F4D8F1, 0
0x196795387  0f849ffdffff                    jz      loc_19679512C
0x19679538d  b931290400                      mov     ecx, 42931h
0x196795392  e8896939fc                      call    sub_192B2BD20
0x196795397  4885c0                          test    rax, rax
0x19679539a  0f843e020000                    jz      loc_1967955DE
0x1967953a0  4889c1                          mov     rcx, rax
0x1967953a3  31d2                            xor     edx, edx
0x1967953a5  4531c0                          xor     r8d, r8d
0x1967953a8  4531c9                          xor     r9d, r9d
0x1967953ab  e800bab0f7                      call    sub_18E2A0DB0
0x1967953b0  bf01000000                      mov     edi, 1
0x1967953b5  84c0                            test    al, al
0x1967953b7  0f8499fdffff                    jz      loc_196795156
0x1967953bd  e94bffffff                      jmp     loc_19679530D
0x1967953c2  e829feaee9                      call    sub_1802851F0
0x1967953c7  e974fdffff                      jmp     loc_196795140
0x1967953cc  e81ffeaee9                      call    sub_1802851F0
0x1967953d1  803d8077c3ee00                  cmp     cs:byte_1853CCB58, 0
0x1967953d8  0f8555feffff                    jnz     loc_196795233
0x1967953de  b918a30300                      mov     ecx, 3A318h
0x1967953e3  e8b873afe9                      call    sub_18028C7A0
0x1967953e8  c6056977c3ee01                  mov     cs:byte_1853CCB58, 1
0x1967953ef  803dfb847bee00                  cmp     cs:byte_184F4D8F1, 0
0x1967953f6  0f8444feffff                    jz      loc_196795240
0x1967953fc  b931290400                      mov     ecx, 42931h
0x196795401  e81a6939fc                      call    sub_192B2BD20
0x196795406  4885c0                          test    rax, rax
0x196795409  0f84d4010000                    jz      loc_1967955E3
0x19679540f  4889c1                          mov     rcx, rax
0x196795412  31d2                            xor     edx, edx
0x196795414  4531c0                          xor     r8d, r8d
0x196795417  4531c9                          xor     r9d, r9d
0x19679541a  e891b9b0f7                      call    sub_18E2A0DB0
0x19679541f  bf01000000                      mov     edi, 1
0x196795424  84c0                            test    al, al
0x196795426  0f843efeffff                    jz      loc_19679526A
0x19679542c  e9dcfeffff                      jmp     loc_19679530D
0x196795431  e8bafdaee9                      call    sub_1802851F0
0x196795436  e919feffff                      jmp     loc_196795254
0x19679543b  e8b0fdaee9                      call    sub_1802851F0
0x196795440  803d7e77c3ee00                  cmp     cs:byte_1853CCBC5, 0
0x196795447  0f852afdffff                    jnz     loc_196795177
0x19679544d  b985a30300                      mov     ecx, 3A385h
0x196795452  e84973afe9                      call    sub_18028C7A0
0x196795457  c6056777c3ee01                  mov     cs:byte_1853CCBC5, 1
0x19679545e  803d01857bee00                  cmp     cs:byte_184F4D966, 0
0x196795465  0f8419fdffff                    jz      loc_196795184
0x19679546b  b9a6290400                      mov     ecx, 429A6h
0x196795470  e8ab6839fc                      call    sub_192B2BD20
0x196795475  4885c0                          test    rax, rax
0x196795478  0f846a010000                    jz      loc_1967955E8
0x19679547e  4889c1                          mov     rcx, rax
0x196795481  31d2                            xor     edx, edx
0x196795483  e8b810aef7                      call    sub_18E276540
0x196795488  bf03000000                      mov     edi, 3
0x19679548d  84c0                            test    al, al
0x19679548f  0f8416fdffff                    jz      loc_1967951AB
0x196795495  e973feffff                      jmp     loc_19679530D
0x19679549a  e851fdaee9                      call    sub_1802851F0
0x19679549f  e9f4fcffff                      jmp     loc_196795198
0x1967954a4  e847fdaee9                      call    sub_1802851F0
0x1967954a9  803d1577c3ee00                  cmp     cs:byte_1853CCBC5, 0
0x1967954b0  0f85d5fdffff                    jnz     loc_19679528B
0x1967954b6  b985a30300                      mov     ecx, 3A385h
0x1967954bb  e8e072afe9                      call    sub_18028C7A0
0x1967954c0  c605fe76c3ee01                  mov     cs:byte_1853CCBC5, 1
0x1967954c7  803d98847bee00                  cmp     cs:byte_184F4D966, 0
0x1967954ce  0f84c4fdffff                    jz      loc_196795298
0x1967954d4  b9a6290400                      mov     ecx, 429A6h
0x1967954d9  e8426839fc                      call    sub_192B2BD20
0x1967954de  4885c0                          test    rax, rax
0x1967954e1  0f8406010000                    jz      loc_1967955ED
0x1967954e7  4889c1                          mov     rcx, rax
0x1967954ea  31d2                            xor     edx, edx
0x1967954ec  e84f10aef7                      call    sub_18E276540
0x1967954f1  bf03000000                      mov     edi, 3
0x1967954f6  84c0                            test    al, al
0x1967954f8  0f84bdfdffff                    jz      loc_1967952BB
0x1967954fe  e90afeffff                      jmp     loc_19679530D
0x196795503  e8e8fcaee9                      call    sub_1802851F0
0x196795508  e99ffdffff                      jmp     loc_1967952AC
0x19679550d  e8defcaee9                      call    sub_1802851F0
0x196795512  803d2c76c3ee00                  cmp     cs:byte_1853CCB45, 0
0x196795519  0f85adfcffff                    jnz     loc_1967951CC
0x19679551f  b905a30300                      mov     ecx, 3A305h
0x196795524  e87772afe9                      call    sub_18028C7A0
0x196795529  c6051576c3ee01                  mov     cs:byte_1853CCB45, 1
0x196795530  803da7837bee00                  cmp     cs:byte_184F4D8DE, 0
0x196795537  0f849cfcffff                    jz      loc_1967951D9
0x19679553d  b91e290400                      mov     ecx, 4291Eh
0x196795542  e8d96739fc                      call    sub_192B2BD20
0x196795547  4885c0                          test    rax, rax
0x19679554a  0f84a2000000                    jz      loc_1967955F2
0x196795550  4889c1                          mov     rcx, rax
0x196795553  31d2                            xor     edx, edx
0x196795555  e8e60faef7                      call    sub_18E276540
0x19679555a  8bbe88010000                    mov     edi, [rsi+188h]
0x196795560  84c0                            test    al, al
0x196795562  0f8599fcffff                    jnz     loc_196795201
0x196795568  e9a0fdffff                      jmp     loc_19679530D
0x19679556d  e87efcaee9                      call    sub_1802851F0
0x196795572  e976fcffff                      jmp     loc_1967951ED
0x196795577  e874fcaee9                      call    sub_1802851F0
0x19679557c  803dc275c3ee00                  cmp     cs:byte_1853CCB45, 0
0x196795583  0f8553fdffff                    jnz     loc_1967952DC
0x196795589  b905a30300                      mov     ecx, 3A305h
0x19679558e  e80d72afe9                      call    sub_18028C7A0
0x196795593  c605ab75c3ee01                  mov     cs:byte_1853CCB45, 1
0x19679559a  803d3d837bee00                  cmp     cs:byte_184F4D8DE, 0
0x1967955a1  0f8442fdffff                    jz      loc_1967952E9
0x1967955a7  b91e290400                      mov     ecx, 4291Eh
0x1967955ac  e86f6739fc                      call    sub_192B2BD20
0x1967955b1  4885c0                          test    rax, rax
0x1967955b4  7441                            jz      short loc_1967955F7
0x1967955b6  31ff                            xor     edi, edi
0x1967955b8  4889c1                          mov     rcx, rax
0x1967955bb  31d2                            xor     edx, edx
0x1967955bd  e87e0faef7                      call    sub_18E276540
0x1967955c2  84c0                            test    al, al
0x1967955c4  0f8434fbffff                    jz      loc_1967950FE
0x1967955ca  e93efdffff                      jmp     loc_19679530D
0x1967955cf  e81cfcaee9                      call    sub_1802851F0
0x1967955d4  e924fdffff                      jmp     loc_1967952FD
0x1967955d9  e882fd19ea                      call    sub_180935360
0x1967955de  e87dfd19ea                      call    sub_180935360
0x1967955e3  e878fd19ea                      call    sub_180935360
0x1967955e8  e873fd19ea                      call    sub_180935360
0x1967955ed  e86efd19ea                      call    sub_180935360
0x1967955f2  e869fd19ea                      call    sub_180935360
0x1967955f7  e864fd19ea                      call    sub_180935360
