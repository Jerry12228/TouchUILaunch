__int64 __fastcall sub_187862130(unsigned int a1)
{
  __int64 v2; // rax
  __int64 *v3; // rdi
  __int64 v4; // rax
  __int64 v5; // rdx
  __int64 v6; // rsi
  __int64 v8; // rcx
  __int64 v9; // rcx
  _QWORD v10[5]; // [rsp+20h] [rbp-28h] BYREF

  if ( byte_18531A5B0 )
  {
    if ( !byte_184F076C6 )
      goto LABEL_3;
LABEL_13:
    if ( !*(_BYTE *)(qword_184FBA640 + 203) )
      sub_180243EF0();
    v9 = *(_QWORD *)(*(_QWORD *)(qword_184E93B48 + 337056) + 915536LL);
    if ( !v9 )
      sub_180B51440();
    return sub_187E00430(v9, a1);
  }
  sub_18024B350(229088);
  byte_18531A5B0 = 1;
  if ( byte_184F076C6 )
    goto LABEL_13;
LABEL_3:
  if ( *(_BYTE *)(qword_184F87FA0 + 203) )
  {
    v2 = *(_QWORD *)(qword_184E93B48 + 252320);
    if ( v2 )
      goto LABEL_5;
LABEL_18:
    sub_180B51440();
  }
  sub_180243EF0();
  v2 = *(_QWORD *)(qword_184E93B48 + 252320);
  if ( !v2 )
    goto LABEL_18;
LABEL_5:
  v3 = *(__int64 **)(v2 + 104);
  if ( !v3 )
    sub_180B51440();
  v4 = *v3;
  if ( *(_WORD *)(*v3 + 194) )
  {
    v5 = *(_QWORD *)(v4 + 24);
    v6 = 0;
    while ( *(_QWORD *)(v5 + v6) != qword_184FC9048 )
    {
      v6 += 16;
      if ( 16LL * *(unsigned __int16 *)(*v3 + 194) == v6 )
        goto LABEL_10;
    }
    v8 = *(_DWORD *)(v5 + v6 + 8) + 1;
    return (*(__int64 (__fastcall **)(__int64 *, _QWORD, _QWORD))(v4 + 8 * v8 + 208))(
             v3,
             a1,
             *(_QWORD *)(v4 + 8 * (v8 + *(unsigned __int16 *)(v4 + 198)) + 208));
  }
  else
  {
LABEL_10:
    sub_180241160(v10, v3, qword_184FC9048, 1);
    return ((__int64 (__fastcall *)(__int64 *, _QWORD, _QWORD))v10[1])(v3, a1, v10[0]);
  }
}
