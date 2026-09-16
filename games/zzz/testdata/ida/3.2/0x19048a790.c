__int64 __fastcall sub_19048A790(__int64 a1)
{
  int v2; // r12d
  __int64 v3; // rdx
  __int64 v4; // rcx
  __int64 v5; // r8
  __int64 v6; // rdi
  __int64 v7; // rax
  __m128i v8; // xmm0
  __m128i v9; // xmm1
  unsigned int v10; // r14d
  unsigned int v11; // ebx
  __int64 v12; // rdi
  __int64 v13; // rax
  __int64 v14; // rdi
  __int64 v15; // rbx
  __int64 v16; // rbx
  __int64 v17; // rcx
  int v18; // edi
  __int64 v19; // rdx
  unsigned int v20; // ebp
  unsigned int v21; // eax
  __int64 v22; // r8
  __int64 v23; // rcx
  __int64 v24; // rdx
  __int64 v25; // rcx
  __int64 v26; // rax
  __int64 v27; // rax
  __int64 v28; // rcx
  __int64 v29; // rdi
  __int64 v30; // rbx
  __int64 result; // rax
  __int64 v32; // rax
  __int64 v33; // rax
  __int64 v34; // rax
  __int64 v35; // rax
  __int64 v36; // rax
  __int64 v37; // rax
  __int64 v38; // rax
  __int64 v39; // rax
  __int64 v40; // rax
  __int64 v41; // rax
  __int64 v42; // rax
  __int64 v43; // rdi
  __int64 v44; // rax
  __int64 v45; // rax
  __int64 v46; // [rsp+28h] [rbp-100h] BYREF
  _OWORD v47[2]; // [rsp+30h] [rbp-F8h] BYREF
  __int128 v48; // [rsp+50h] [rbp-D8h]
  __int128 v49; // [rsp+60h] [rbp-C8h]
  int v50; // [rsp+70h] [rbp-B8h]
  __int64 v51; // [rsp+80h] [rbp-A8h]
  unsigned int v52; // [rsp+8Ch] [rbp-9Ch] BYREF
  __int64 v53; // [rsp+90h] [rbp-98h]
  unsigned int v54; // [rsp+B0h] [rbp-78h]

  if ( byte_1853C0391 )
  {
    v46 = 0;
    if ( !byte_184F49C8A )
      goto LABEL_3;
LABEL_77:
    v33 = sub_192B2BD20(257226);
    if ( !v33 )
      ((void (__noreturn *)(void))sub_180935360)();
    return sub_18E24CEE0(v33, a1);
  }
  sub_18028C7A0(187217);
  byte_1853C0391 = 1;
  v46 = 0;
  if ( byte_184F49C8A )
    goto LABEL_77;
LABEL_3:
  if ( *(_BYTE *)(qword_184FC13E0 + 203) )
  {
    if ( byte_1853CCB45 )
      goto LABEL_5;
  }
  else
  {
    sub_1802851F0();
    if ( byte_1853CCB45 )
    {
LABEL_5:
      if ( !byte_184F4D8DE )
        goto LABEL_6;
LABEL_81:
      v34 = sub_192B2BD20(272670);
      if ( !v34 )
        ((void (__noreturn *)(void))sub_180935360)();
      v2 = 0;
      if ( !(unsigned __int8)sub_18E276540(v34, 0) )
        goto LABEL_38;
      goto LABEL_9;
    }
  }
  sub_18028C7A0(238341);
  byte_1853CCB45 = 1;
  if ( byte_184F4D8DE )
    goto LABEL_81;
LABEL_6:
  if ( !*(_BYTE *)(qword_184FC13E0 + 203) )
    sub_1802851F0();
  v2 = 0;
  if ( (unsigned int)sub_1934C8150() != 2 )
    goto LABEL_38;
LABEL_9:
  if ( byte_184F197DA )
  {
    v35 = sub_192B2BD20(59418);
    if ( !v35 )
      ((void (__noreturn *)(void))sub_180935360)();
    if ( (unsigned __int8)sub_18E276540(v35, 0) )
      goto LABEL_15;
  }
  else if ( (unsigned __int8)qword_184F7EF58(0) )
  {
    goto LABEL_15;
  }
  if ( byte_184F197DA )
  {
    v41 = sub_192B2BD20(59418);
    if ( !v41 )
      ((void (__noreturn *)(void))sub_180935360)();
    if ( (unsigned __int8)sub_18E276540(v41, 1) )
      goto LABEL_15;
  }
  else if ( (unsigned __int8)qword_184F7EF58(1) )
  {
    goto LABEL_15;
  }
  if ( byte_184F197DA )
  {
    v42 = sub_192B2BD20(59418);
    if ( !v42 )
      ((void (__noreturn *)(void))sub_180935360)();
    if ( !(unsigned __int8)sub_18E276540(v42, 2) )
      goto LABEL_102;
LABEL_15:
    if ( !*(_BYTE *)(a1 + 57) )
    {
      v14 = *((_QWORD *)&xmmword_184ECF6D0 + 1);
      v15 = *(_QWORD *)(a1 + 40);
      if ( !v15 )
        goto LABEL_96;
LABEL_29:
      if ( !*(_DWORD *)(v15 + 24) )
      {
        v36 = ((__int64 (*)(void))sub_180284AB0)();
        sub_1809352F0(v36, 0);
      }
      *(_DWORD *)(v15 + 32) = -252;
      DWORD2(v47[0]) = 0;
      *(_QWORD *)&v47[0] = 0;
      qword_184F7EFD0(v47);
      if ( !*(_DWORD *)(v15 + 24) )
      {
        v37 = ((__int64 (*)(void))sub_180284AB0)();
        sub_1809352F0(v37, 0);
      }
      *(_QWORD *)(v15 + 36) = *(_QWORD *)&v47[0];
      v16 = *(_QWORD *)(a1 + 48);
      if ( v16 && !*(_BYTE *)(a1 + 57) )
      {
        DWORD2(v47[0]) = 0;
        *(_QWORD *)&v47[0] = 0;
        qword_184F7EFD0(v47);
        sub_18D1CE2D0(v16, 4294967044LL, *(_QWORD *)&v47[0], qword_18514D880);
      }
      v17 = *(_QWORD *)(a1 + 24);
      if ( v17 && *(_BYTE *)(a1 + 57) )
        sub_18D1CE2D0(v17, 4294967044LL, v14, qword_18514D880);
      *(_BYTE *)(a1 + 57) = 1;
      v2 = 1;
      goto LABEL_38;
    }
    DWORD2(v47[0]) = 0;
    *(_QWORD *)&v47[0] = 0;
    qword_184F7EFD0(v47);
    v6 = *(_QWORD *)&v47[0];
    v4 = qword_184FBC0A0;
    if ( *(_BYTE *)(qword_184FBC0A0 + 203) )
    {
      v7 = *(_QWORD *)(a1 + 40);
      if ( v7 )
        goto LABEL_18;
    }
    else
    {
      sub_1802851F0();
      v7 = *(_QWORD *)(a1 + 40);
      if ( v7 )
      {
LABEL_18:
        if ( !*(_DWORD *)(v7 + 24) )
        {
          v39 = ((__int64 (*)(void))sub_180284AB0)();
          sub_1809352F0(v39, 0);
        }
        v9 = _mm_cvtsi32_si128(v6);
        v8 = _mm_cvtsi32_si128(HIDWORD(v6));
        *(float *)v8.m128i_i32 = *(float *)v8.m128i_i32 - *(float *)(v7 + 40);
        *(float *)v9.m128i_i32 = *(float *)v9.m128i_i32 - *(float *)(v7 + 36);
        v10 = _mm_cvtsi128_si32(v9);
        v11 = _mm_cvtsi128_si32(v8);
        if ( byte_184F49C87 )
        {
          v40 = sub_192B2BD20(257223);
          if ( !v40 )
            ((void (__noreturn *)(void))sub_180935360)();
          v14 = sub_18B50D390(v40, a1, v10 | ((unsigned __int64)v11 << 32));
          v15 = *(_QWORD *)(a1 + 40);
          if ( !v15 )
LABEL_96:
            sub_180935360(v4, v3, v5);
        }
        else
        {
          v12 = 3212836864LL;
          v13 = 3212836864LL;
          if ( *(float *)v9.m128i_i32 >= -1.0 )
          {
            v13 = v10;
            if ( *(float *)v9.m128i_i32 > 1.0 )
              v13 = 1065353216;
          }
          if ( *(float *)v8.m128i_i32 >= -1.0 )
          {
            v12 = v11;
            if ( *(float *)v8.m128i_i32 > 1.0 )
              v12 = 1065353216;
          }
          v14 = v13 | (v12 << 32);
          v15 = *(_QWORD *)(a1 + 40);
          if ( !v15 )
            goto LABEL_96;
        }
        goto LABEL_29;
      }
    }
    ((void (__noreturn *)(void))sub_180935360)();
  }
  if ( (unsigned __int8)qword_184F7EF58(2) )
    goto LABEL_15;
LABEL_102:
  if ( byte_184F197E0 )
  {
    v44 = sub_192B2BD20(59424);
    if ( !v44 )
      ((void (__noreturn *)(void))sub_180935360)();
    if ( (unsigned __int8)sub_18E276540(v44, 0) )
      goto LABEL_107;
  }
  else if ( (unsigned __int8)qword_184F7EF68(0) )
  {
    goto LABEL_107;
  }
  if ( byte_184F197E0 )
  {
    v45 = sub_192B2BD20(59424);
    if ( !v45 )
      ((void (__noreturn *)(void))sub_180935360)();
    if ( (unsigned __int8)sub_18E276540(v45, 1) )
      goto LABEL_107;
  }
  else if ( (unsigned __int8)qword_184F7EF68(1) )
  {
    goto LABEL_107;
  }
  if ( (unsigned __int8)sub_192C7FAC0(2) )
  {
LABEL_107:
    *(_BYTE *)(a1 + 57) = 0;
    v43 = *(_QWORD *)(a1 + 32);
    v2 = 0;
    if ( v43 )
    {
      DWORD2(v47[0]) = 0;
      *(_QWORD *)&v47[0] = 0;
      qword_184F7EFD0(v47);
      sub_18D1CE2D0(v43, 4294967044LL, *(_QWORD *)&v47[0], qword_18514D880);
    }
    goto LABEL_38;
  }
  *(_BYTE *)(a1 + 57) = 0;
  v2 = 0;
LABEL_38:
  v18 = 0;
  while ( !byte_184F197DF )
  {
    if ( v18 >= (int)qword_184F7EFA8() )
      goto LABEL_64;
LABEL_42:
    if ( byte_184F197DC )
    {
      v27 = sub_192B2BD20(59420);
      if ( !v27 )
        ((void (__noreturn *)(void))sub_180935360)();
      sub_18A00D620(&v52, v27, (unsigned int)v18);
      v20 = v52;
      v21 = v54;
      v22 = v53;
      if ( v54 >= 3 )
      {
LABEL_61:
        if ( v21 - 3 < 2 )
        {
          v28 = *(_QWORD *)(a1 + 32);
          if ( v28 )
            sub_18D1CE2D0(v28, v20, v22, qword_18514D880);
        }
        goto LABEL_39;
      }
    }
    else
    {
      v49 = 0;
      v48 = 0;
      memset(v47, 0, sizeof(v47));
      v50 = 0;
      qword_184F7EFC8((unsigned int)v18, v47);
      v20 = v47[0];
      v51 = *(_QWORD *)((char *)v47 + 4);
      v21 = DWORD1(v48);
      v22 = *(_QWORD *)((char *)v47 + 4);
      if ( DWORD1(v48) >= 3 )
        goto LABEL_61;
    }
    v23 = *(_QWORD *)(a1 + 40);
    if ( !v23 )
      sub_180935360(0, v19, v22);
    if ( (unsigned int)v2 >= *(_DWORD *)(v23 + 24) )
    {
      v32 = sub_180284AB0(v23, v19, v22);
      sub_1809352F0(v32, 0);
    }
    v24 = 3LL * v2;
    *(_DWORD *)(v23 + 4 * v24 + 32) = v20;
    *(_QWORD *)(v23 + 4 * v24 + 36) = v22;
    if ( v21 == 1 )
    {
      if ( !(unsigned __int8)sub_19048AF70(a1, v20, v22, &v46) )
        goto LABEL_54;
      v25 = *(_QWORD *)(a1 + 24);
      if ( !v25 )
        goto LABEL_54;
      v22 = v46;
    }
    else
    {
      if ( v21 )
        goto LABEL_54;
      v25 = *(_QWORD *)(a1 + 48);
      if ( !v25 )
        goto LABEL_54;
    }
    sub_18D1CE2D0(v25, v20, v22, qword_18514D880);
LABEL_54:
    if ( v2 >= 10 )
    {
      v29 = *(_QWORD *)(a1 + 40);
      goto LABEL_69;
    }
    ++v2;
LABEL_39:
    ++v18;
  }
  v26 = sub_192B2BD20(59423);
  if ( !v26 )
    ((void (__noreturn *)(void))sub_180935360)();
  if ( v18 < (int)sub_18E2A0790(v26) )
    goto LABEL_42;
LABEL_64:
  v29 = *(_QWORD *)(a1 + 40);
  if ( v2 <= 10 )
  {
    if ( !v29 )
      ((void (__noreturn *)(void))sub_180935360)();
    if ( (unsigned int)v2 >= *(_DWORD *)(v29 + 24) )
    {
      v38 = ((__int64 (*)(void))sub_180284AB0)();
      sub_1809352F0(v38, 0);
    }
    *(_DWORD *)(v29 + 12LL * v2 + 32) = -253;
  }
LABEL_69:
  v30 = *(_QWORD *)(a1 + 16);
  if ( !byte_185392CA4 )
  {
    sub_18028C7A0(1124);
    byte_185392CA4 = 1;
  }
  result = sub_19CB37690(v29, v30, 11);
  if ( *(_BYTE *)(a1 + 56) )
    return sub_19048A3F0(a1);
  return result;
}
