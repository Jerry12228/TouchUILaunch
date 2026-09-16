__int64 sub_1934C7D90()
{
  __int64 v0; // rax
  __int64 *v1; // rsi
  __int64 v2; // rax
  __int64 v3; // rdx
  __int64 v4; // r9
  _QWORD v6[3]; // [rsp+20h] [rbp-18h] BYREF

  if ( byte_1853CCB43 )
  {
    if ( *(_BYTE *)(qword_184FC13E0 + 203) )
      goto LABEL_3;
  }
  else
  {
    sub_18028C7A0(238339);
    byte_1853CCB43 = 1;
    if ( *(_BYTE *)(qword_184FC13E0 + 203) )
    {
LABEL_3:
      v0 = *(_QWORD *)(qword_184ECD400 + 189544);
      if ( v0 )
        goto LABEL_4;
LABEL_14:
      sub_180935360();
    }
  }
  sub_1802851F0();
  v0 = *(_QWORD *)(qword_184ECD400 + 189544);
  if ( !v0 )
    goto LABEL_14;
LABEL_4:
  v1 = *(__int64 **)(v0 + 152);
  if ( !v1 )
    sub_180935360();
  v2 = *v1;
  if ( *(_WORD *)(*v1 + 198) )
  {
    v3 = *(_QWORD *)(v2 + 80);
    v4 = 0;
    while ( *(_QWORD *)(v3 + v4) != qword_184FFE8D8 )
    {
      v4 += 16;
      if ( 16LL * *(unsigned __int16 *)(*v1 + 198) == v4 )
        goto LABEL_9;
    }
    return (*(__int64 (__fastcall **)(__int64 *, _QWORD))(v2 + 8LL * *(int *)(v3 + v4 + 8) + 208))(
             v1,
             *(_QWORD *)(v2 + 8 * (*(int *)(v3 + v4 + 8) + (unsigned __int64)*(unsigned __int16 *)(v2 + 196)) + 208));
  }
  else
  {
LABEL_9:
    sub_180282510(v6, v1, qword_184FFE8D8, 0);
    return ((__int64 (__fastcall *)(__int64 *, _QWORD))v6[0])(v1, v6[1]);
  }
}
