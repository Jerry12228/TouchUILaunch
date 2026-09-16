__int64 __fastcall sub_18B109140(unsigned int a1, __int64 a2, __int64 a3)
{
  __int64 v4; // rcx
  __int64 v5; // rax
  __int64 *v6; // rdi
  __int64 v7; // rax
  __int64 v8; // rdx
  __int64 v9; // rsi
  __int64 v11; // rcx
  __int64 v12; // rcx
  _QWORD v13[5]; // [rsp+20h] [rbp-28h] BYREF

  if ( byte_184CCBD17 )
  {
    if ( !byte_1848FD7B8 )
      goto LABEL_3;
LABEL_13:
    if ( !*(_BYTE *)(qword_184985118 + 203) )
      sub_180234770();
    v12 = *(_QWORD *)(*(_QWORD *)(qword_184885C60 + 255088) + 2163168LL);
    if ( !v12 )
      sub_180979760(0, a2, a3);
    return sub_187036C00(v12, a1);
  }
  sub_18023BC30(121431);
  byte_184CCBD17 = 1;
  if ( byte_1848FD7B8 )
    goto LABEL_13;
LABEL_3:
  v4 = qword_184950E90;
  if ( *(_BYTE *)(qword_184950E90 + 203) )
  {
    v5 = *(_QWORD *)(qword_184885C60 + 169384);
    if ( v5 )
      goto LABEL_5;
LABEL_18:
    sub_180979760(v4, a2, a3);
  }
  sub_180234770();
  v5 = *(_QWORD *)(qword_184885C60 + 169384);
  if ( !v5 )
    goto LABEL_18;
LABEL_5:
  v6 = *(__int64 **)(v5 + 112);
  if ( !v6 )
    sub_180979760(v4, a2, a3);
  v7 = *v6;
  if ( *(_WORD *)(*v6 + 198) )
  {
    v8 = *(_QWORD *)(v7 + 48);
    v9 = 0;
    while ( *(_QWORD *)(v8 + v9) != qword_18495F048 )
    {
      v9 += 16;
      if ( 16LL * *(unsigned __int16 *)(*v6 + 198) == v9 )
        goto LABEL_10;
    }
    v11 = *(_DWORD *)(v8 + v9 + 8) + 1;
    return (*(__int64 (__fastcall **)(__int64 *, _QWORD, _QWORD))(v7 + 8 * v11 + 208))(
             v6,
             a1,
             *(_QWORD *)(v7 + 8 * (v11 + *(unsigned __int16 *)(v7 + 194)) + 208));
  }
  else
  {
LABEL_10:
    sub_1802319F0(v13, v6, qword_18495F048, 1);
    return ((__int64 (__fastcall *)(__int64 *, _QWORD, _QWORD))v13[0])(v6, a1, v13[1]);
  }
}
