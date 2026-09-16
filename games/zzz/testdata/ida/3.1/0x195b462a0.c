__int64 __fastcall sub_195B462A0(__int64 a1, __int64 a2, __int64 a3)
{
  __int64 v3; // rcx
  __int64 v4; // rax
  __int64 *v5; // rsi
  __int64 v6; // rax
  __int64 v7; // rdx
  __int64 v8; // r9
  _QWORD v10[3]; // [rsp+20h] [rbp-18h] BYREF

  if ( byte_1858954CA )
  {
    v3 = qword_185450EB0;
    if ( *(_BYTE *)(qword_185450EB0 + 203) )
      goto LABEL_3;
  }
  else
  {
    sub_1802793E0(0x43ADAu);
    byte_1858954CA = 1;
    v3 = qword_185450EB0;
    if ( *(_BYTE *)(qword_185450EB0 + 203) )
    {
LABEL_3:
      v4 = *(_QWORD *)(qword_185360730 + 186816);
      if ( v4 )
        goto LABEL_4;
LABEL_14:
      sub_180AC9120(v3, a2, a3);
    }
  }
  sub_180271E50(v3);
  v4 = *(_QWORD *)(qword_185360730 + 186816);
  if ( !v4 )
    goto LABEL_14;
LABEL_4:
  v5 = *(__int64 **)(v4 + 128);
  if ( !v5 )
    sub_180AC9120(v3, a2, a3);
  v6 = *v5;
  if ( *(_WORD *)(*v5 + 198) )
  {
    v7 = *(_QWORD *)(v6 + 16);
    v8 = 0;
    while ( *(_QWORD *)(v7 + v8) != qword_18549E4A0 )
    {
      v8 += 16;
      if ( 16LL * *(unsigned __int16 *)(*v5 + 198) == v8 )
        goto LABEL_9;
    }
    return (*(__int64 (__fastcall **)(__int64 *, _QWORD))(v6 + 8LL * *(int *)(v7 + v8 + 8) + 208))(
             v5,
             *(_QWORD *)(v6 + 8 * (*(int *)(v7 + v8 + 8) + (unsigned __int64)*(unsigned __int16 *)(v6 + 194)) + 208));
  }
  else
  {
LABEL_9:
    sub_18026F170(v10, v5, qword_18549E4A0, 0);
    return ((__int64 (__fastcall *)(__int64 *, _QWORD))v10[0])(v5, v10[1]);
  }
}
