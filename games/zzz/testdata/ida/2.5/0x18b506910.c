__int64 __fastcall sub_18B506910(__int64 a1)
{
  int v2; // edi
  int v3; // esi
  __int64 v4; // rdx
  __int64 v5; // rcx
  __int64 v6; // r8
  __m128i v7; // xmm7
  __m128i v8; // xmm6
  __int64 v9; // rax
  unsigned int v10; // eax
  unsigned __int64 v11; // rbx
  __int64 v12; // rcx
  __int64 v13; // rbx
  __int64 v14; // rsi
  __int64 v15; // rbp
  __int64 v16; // rcx
  __int64 v17; // rdx
  unsigned int v18; // ebp
  unsigned int v19; // eax
  __int64 v20; // r8
  __int64 v21; // rcx
  __int64 v22; // rdx
  __int64 v23; // rcx
  __int64 v24; // rdx
  __int64 v25; // rcx
  __int64 v26; // rax
  __int64 result; // rax
  __int64 v28; // rax
  __int64 v29; // rcx
  __int64 v30; // rcx
  __int64 v31; // rax
  __int64 v32; // rax
  __int64 v33; // rax
  __int64 v34; // rax
  __int64 v35; // rcx
  __int64 v36; // rcx
  __int64 v37; // rcx
  __int64 v38; // rbx
  __int64 v39; // rcx
  __int64 v40; // rcx
  _OWORD v41[2]; // [rsp+20h] [rbp-108h] BYREF
  __int128 v42; // [rsp+40h] [rbp-E8h]
  __int128 v43; // [rsp+50h] [rbp-D8h]
  int v44; // [rsp+60h] [rbp-C8h]
  _QWORD v45[2]; // [rsp+68h] [rbp-C0h] BYREF
  unsigned int v46; // [rsp+78h] [rbp-B0h] BYREF
  __int64 v47; // [rsp+7Ch] [rbp-ACh]
  unsigned int v48; // [rsp+9Ch] [rbp-8Ch]

  if ( byte_18531AA79 )
  {
    v45[0] = 0;
    if ( !byte_184F0D0DE )
      goto LABEL_3;
LABEL_75:
    if ( !*(_BYTE *)(qword_184FBA640 + 203) )
      sub_180243EF0();
    v29 = *(_QWORD *)(*(_QWORD *)(qword_184E93B48 + 337056) + 1100048LL);
    if ( !v29 )
      ((void (__noreturn *)(void))sub_180B51440)();
    return sub_187DAB140(v29, a1);
  }
  sub_18024B350(230313);
  byte_18531AA79 = 1;
  v45[0] = 0;
  if ( byte_184F0D0DE )
    goto LABEL_75;
LABEL_3:
  if ( !*(_BYTE *)(qword_184F87FA0 + 203) )
    sub_180243EF0();
  v2 = 0;
  v3 = 0;
  if ( sub_18786FEC0(0) )
  {
    if ( byte_184F09B6F )
    {
      if ( !*(_BYTE *)(qword_184FBA640 + 203) )
        sub_180243EF0();
      v30 = *(_QWORD *)(*(_QWORD *)(qword_184E93B48 + 337056) + 990616LL);
      if ( !v30 )
        ((void (__noreturn *)(void))sub_180B51440)();
      if ( (unsigned __int8)sub_187DF1700(v30, 0) )
        goto LABEL_12;
    }
    else if ( (unsigned __int8)qword_184F43F30(0) )
    {
      goto LABEL_12;
    }
    if ( byte_184F09B6F )
    {
      if ( !*(_BYTE *)(qword_184FBA640 + 203) )
        sub_180243EF0();
      v36 = *(_QWORD *)(*(_QWORD *)(qword_184E93B48 + 337056) + 990616LL);
      if ( !v36 )
        ((void (__noreturn *)(void))sub_180B51440)();
      if ( (unsigned __int8)sub_187DF1700(v36, 1) )
        goto LABEL_12;
    }
    else if ( (unsigned __int8)qword_184F43F30(1) )
    {
      goto LABEL_12;
    }
    if ( !byte_184F09B6F )
    {
      if ( (unsigned __int8)qword_184F43F30(2) )
        goto LABEL_12;
LABEL_105:
      if ( byte_184F09B6D )
      {
        if ( !*(_BYTE *)(qword_184FBA640 + 203) )
          sub_180243EF0();
        v39 = *(_QWORD *)(*(_QWORD *)(qword_184E93B48 + 337056) + 990600LL);
        if ( !v39 )
          ((void (__noreturn *)(void))sub_180B51440)();
        if ( (unsigned __int8)sub_187DF1700(v39, 0) )
          goto LABEL_110;
      }
      else if ( (unsigned __int8)qword_184F43F40(0) )
      {
        goto LABEL_110;
      }
      if ( byte_184F09B6D )
      {
        if ( !*(_BYTE *)(qword_184FBA640 + 203) )
          sub_180243EF0();
        v40 = *(_QWORD *)(*(_QWORD *)(qword_184E93B48 + 337056) + 990600LL);
        if ( !v40 )
          ((void (__noreturn *)(void))sub_180B51440)();
        if ( (unsigned __int8)sub_187DF1700(v40, 1) )
          goto LABEL_110;
      }
      else if ( (unsigned __int8)qword_184F43F40(1) )
      {
        goto LABEL_110;
      }
      if ( !(unsigned __int8)sub_187B96EA0(2) )
      {
        *(_BYTE *)(a1 + 56) = 0;
        goto LABEL_112;
      }
LABEL_110:
      *(_BYTE *)(a1 + 56) = 0;
      v38 = *(_QWORD *)(a1 + 32);
      v3 = 0;
      if ( !v38 )
        goto LABEL_37;
      DWORD2(v41[0]) = 0;
      *(_QWORD *)&v41[0] = 0;
      qword_184F43FA8(v41);
      sub_18F6F96E0(v38, 4294967044LL, *(_QWORD *)&v41[0], qword_185142C68);
LABEL_112:
      v3 = 0;
      goto LABEL_37;
    }
    if ( !*(_BYTE *)(qword_184FBA640 + 203) )
      sub_180243EF0();
    v37 = *(_QWORD *)(*(_QWORD *)(qword_184E93B48 + 337056) + 990616LL);
    if ( !v37 )
      ((void (__noreturn *)(void))sub_180B51440)();
    if ( !(unsigned __int8)sub_187DF1700(v37, 2) )
      goto LABEL_105;
LABEL_12:
    if ( !*(_BYTE *)(a1 + 56) )
    {
      v13 = qword_184E95C90;
      v14 = *(_QWORD *)(a1 + 16);
      if ( !v14 )
        goto LABEL_95;
      goto LABEL_26;
    }
    DWORD2(v41[0]) = 0;
    *(_QWORD *)&v41[0] = 0;
    qword_184F43FA8(v41);
    v7 = (__m128i)LODWORD(v41[0]);
    v8 = (__m128i)DWORD1(v41[0]);
    if ( *(_BYTE *)(qword_184F7F168 + 203) )
    {
      v9 = *(_QWORD *)(a1 + 16);
      if ( v9 )
        goto LABEL_15;
    }
    else
    {
      sub_180243EF0();
      v9 = *(_QWORD *)(a1 + 16);
      if ( v9 )
      {
LABEL_15:
        if ( !*(_DWORD *)(v9 + 24) )
        {
          v34 = ((__int64 (*)(void))sub_1802436F0)();
          sub_180B513E0(v34, 0);
        }
        *(float *)v8.m128i_i32 = *(float *)v8.m128i_i32 - *(float *)(v9 + 40);
        *(float *)v7.m128i_i32 = *(float *)v7.m128i_i32 - *(float *)(v9 + 36);
        v10 = _mm_cvtsi128_si32(v8);
        v11 = ((unsigned __int64)v10 << 32) | (unsigned int)_mm_cvtsi128_si32(v7);
        if ( byte_184F0D0DB )
        {
          if ( !*(_BYTE *)(qword_184FBA640 + 203) )
            sub_180243EF0();
          v35 = *(_QWORD *)(*(_QWORD *)(qword_184E93B48 + 337056) + 1100024LL);
          if ( !v35 )
            ((void (__noreturn *)(void))sub_180B51440)();
          v13 = sub_193F341D0(v35, a1, v11);
          v14 = *(_QWORD *)(a1 + 16);
          if ( !v14 )
LABEL_95:
            sub_180B51440(v5, v4, v6);
        }
        else
        {
          v12 = 3212836864LL;
          v4 = 3212836864LL;
          if ( *(float *)v7.m128i_i32 >= -1.0 )
          {
            v4 = v11;
            if ( *(float *)v7.m128i_i32 > 1.0 )
              v4 = 1065353216;
          }
          if ( *(float *)v8.m128i_i32 >= -1.0 )
          {
            v12 = v10;
            if ( *(float *)v8.m128i_i32 > 1.0 )
              v12 = 1065353216;
          }
          v5 = v12 << 32;
          v13 = v5 | (unsigned int)v4;
          v14 = *(_QWORD *)(a1 + 16);
          if ( !v14 )
            goto LABEL_95;
        }
LABEL_26:
        if ( !*(_DWORD *)(v14 + 24) )
        {
          v31 = ((__int64 (*)(void))sub_1802436F0)();
          sub_180B513E0(v31, 0);
        }
        *(_DWORD *)(v14 + 32) = -252;
        DWORD2(v41[0]) = 0;
        *(_QWORD *)&v41[0] = 0;
        qword_184F43FA8(v41);
        if ( !*(_DWORD *)(v14 + 24) )
        {
          v32 = ((__int64 (*)(void))sub_1802436F0)();
          sub_180B513E0(v32, 0);
        }
        *(_QWORD *)(v14 + 36) = *(_QWORD *)&v41[0];
        v15 = *(_QWORD *)(a1 + 24);
        if ( v15 && !*(_BYTE *)(a1 + 56) )
        {
          DWORD2(v41[0]) = 0;
          *(_QWORD *)&v41[0] = 0;
          qword_184F43FA8(v41);
          sub_18F6F96E0(v15, 4294967044LL, *(_QWORD *)&v41[0], qword_185142C68);
        }
        v16 = *(_QWORD *)(a1 + 48);
        if ( v16 && *(_BYTE *)(a1 + 56) )
          sub_18F6F96E0(v16, 4294967044LL, v13, qword_185142C68);
        *(_BYTE *)(a1 + 56) = 1;
        v3 = 1;
        goto LABEL_37;
      }
    }
    ((void (__noreturn *)(void))sub_180B51440)();
  }
LABEL_37:
  while ( !byte_184F09B71 )
  {
    if ( v2 >= (int)qword_184F43F80() )
      goto LABEL_65;
LABEL_39:
    if ( byte_184F09B72 )
    {
      if ( !*(_BYTE *)(qword_184FBA640 + 203) )
        sub_180243EF0();
      v24 = *(_QWORD *)(*(_QWORD *)(qword_184E93B48 + 337056) + 990640LL);
      if ( !v24 )
        ((void (__noreturn *)(void))sub_180B51440)();
      sub_19508AA20(&v46, v24, (unsigned int)v2);
      v18 = v46;
      v19 = v48;
      v20 = v47;
      if ( v48 >= 3 )
      {
LABEL_62:
        if ( v19 - 3 < 2 )
        {
          v25 = *(_QWORD *)(a1 + 32);
          if ( v25 )
            sub_18F6F96E0(v25, v18, v20, qword_185142C68);
        }
        goto LABEL_36;
      }
    }
    else
    {
      v43 = 0;
      v42 = 0;
      memset(v41, 0, sizeof(v41));
      v44 = 0;
      qword_184F43FA0((unsigned int)v2, v41);
      v18 = v41[0];
      v45[1] = *(_QWORD *)((char *)v41 + 4);
      v19 = DWORD1(v42);
      v20 = *(_QWORD *)((char *)v41 + 4);
      if ( DWORD1(v42) >= 3 )
        goto LABEL_62;
    }
    v21 = *(_QWORD *)(a1 + 16);
    if ( !v21 )
      sub_180B51440(0, v17, v20);
    if ( (unsigned int)v3 >= *(_DWORD *)(v21 + 24) )
    {
      v28 = sub_1802436F0(v21, v17, v20);
      sub_180B513E0(v28, 0);
    }
    v22 = 3LL * v3;
    *(_DWORD *)(v21 + 4 * v22 + 32) = v18;
    *(_QWORD *)(v21 + 4 * v22 + 36) = v20;
    if ( v19 == 1 )
    {
      if ( !(unsigned __int8)sub_18B506460(a1, v18, v20, v45) )
        goto LABEL_51;
      v23 = *(_QWORD *)(a1 + 48);
      if ( !v23 )
        goto LABEL_51;
      v20 = v45[0];
    }
    else
    {
      if ( v19 )
        goto LABEL_51;
      v23 = *(_QWORD *)(a1 + 24);
      if ( !v23 )
        goto LABEL_51;
    }
    sub_18F6F96E0(v23, v18, v20, qword_185142C68);
LABEL_51:
    if ( v3 >= 10 )
      goto LABEL_69;
    ++v3;
LABEL_36:
    ++v2;
  }
  if ( !*(_BYTE *)(qword_184FBA640 + 203) )
    sub_180243EF0();
  if ( !*(_QWORD *)(*(_QWORD *)(qword_184E93B48 + 337056) + 990632LL) )
    ((void (__noreturn *)(void))sub_180B51440)();
  if ( v2 < (int)sub_187DF6580() )
    goto LABEL_39;
LABEL_65:
  if ( v3 <= 10 )
  {
    v26 = *(_QWORD *)(a1 + 16);
    if ( !v26 )
      ((void (__noreturn *)(void))sub_180B51440)();
    if ( (unsigned int)v3 >= *(_DWORD *)(v26 + 24) )
    {
      v33 = ((__int64 (*)(void))sub_1802436F0)();
      sub_180B513E0(v33, 0);
    }
    *(_DWORD *)(v26 + 12LL * v3 + 32) = -253;
  }
LABEL_69:
  result = sub_19AAB65A0(*(_QWORD *)(a1 + 16), *(_QWORD *)(a1 + 40), 11);
  if ( *(_BYTE *)(a1 + 57) )
    return sub_18B506010(a1);
  return result;
}
