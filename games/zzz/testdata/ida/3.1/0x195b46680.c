__int64 __fastcall sub_195B46680(unsigned int a1, __int64 a2, __int64 a3)
{
  __int64 v4; // rcx
  __int64 v5; // rax
  __int64 *v6; // rdi
  __int64 v7; // rax
  __int64 v8; // rdx
  __int64 v9; // r9
  __int64 v11; // rcx
  __int64 v12; // rax
  __int64 v13; // rdx
  __int64 v14; // rcx
  __int64 v15; // r8
  _QWORD v16[4]; // [rsp+28h] [rbp-20h] BYREF

  if ( byte_1858954E5 )
  {
    if ( !byte_1853AA6FE )
      goto LABEL_3;
LABEL_14:
    v12 = sub_19017D760(54350);
    if ( !v12 )
      sub_180AC9120(v14, v13, v15);
    return sub_18B1C40D0(v12, a1);
  }
  sub_1802793E0(0x43AF5u);
  byte_1858954E5 = 1;
  if ( byte_1853AA6FE )
    goto LABEL_14;
LABEL_3:
  v4 = qword_185450EB0;
  if ( *(_BYTE *)(qword_185450EB0 + 203) )
  {
    v5 = *(_QWORD *)(qword_185360730 + 186816);
    if ( v5 )
      goto LABEL_5;
LABEL_17:
    sub_180AC9120(v4, a2, a3);
  }
  sub_180271E50(qword_185450EB0);
  v5 = *(_QWORD *)(qword_185360730 + 186816);
  if ( !v5 )
    goto LABEL_17;
LABEL_5:
  v6 = *(__int64 **)(v5 + 128);
  if ( !v6 )
    sub_180AC9120(v4, a2, a3);
  v7 = *v6;
  if ( *(_WORD *)(*v6 + 198) )
  {
    v8 = *(_QWORD *)(v7 + 16);
    v9 = 0;
    while ( *(_QWORD *)(v8 + v9) != qword_18549E4A0 )
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
    sub_18026F170(v16, v6, qword_18549E4A0, 1);
    return ((__int64 (__fastcall *)(__int64 *, _QWORD, _QWORD))v16[0])(v6, a1, v16[1]);
  }
}
