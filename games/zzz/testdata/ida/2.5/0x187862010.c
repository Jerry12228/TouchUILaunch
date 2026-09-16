__int64 sub_187862010()
{
  __int64 v0; // rax
  __int64 *v1; // rdi
  __int64 v2; // rax
  __int64 v3; // rdx
  __int64 v4; // rsi
  _QWORD v6[4]; // [rsp+28h] [rbp-20h] BYREF

  if ( byte_18531A5AF )
  {
    if ( *(_BYTE *)(qword_184F87FA0 + 203) )
      goto LABEL_3;
  }
  else
  {
    sub_18024B350(229087);
    byte_18531A5AF = 1;
    if ( *(_BYTE *)(qword_184F87FA0 + 203) )
    {
LABEL_3:
      v0 = *(_QWORD *)(qword_184E93B48 + 252320);
      if ( v0 )
        goto LABEL_4;
LABEL_14:
      sub_180B51440();
    }
  }
  sub_180243EF0();
  v0 = *(_QWORD *)(qword_184E93B48 + 252320);
  if ( !v0 )
    goto LABEL_14;
LABEL_4:
  v1 = *(__int64 **)(v0 + 104);
  if ( !v1 )
    sub_180B51440();
  v2 = *v1;
  if ( *(_WORD *)(*v1 + 194) )
  {
    v3 = *(_QWORD *)(v2 + 24);
    v4 = 0;
    while ( *(_QWORD *)(v3 + v4) != qword_184FC9048 )
    {
      v4 += 16;
      if ( 16LL * *(unsigned __int16 *)(*v1 + 194) == v4 )
        goto LABEL_9;
    }
    return (*(__int64 (__fastcall **)(__int64 *, _QWORD))(v2 + 8LL * *(int *)(v3 + v4 + 8) + 208))(
             v1,
             *(_QWORD *)(v2 + 8 * (*(int *)(v3 + v4 + 8) + (unsigned __int64)*(unsigned __int16 *)(v2 + 198)) + 208));
  }
  else
  {
LABEL_9:
    sub_180241160(v6, v1, qword_184FC9048, 0);
    return ((__int64 (__fastcall *)(__int64 *, _QWORD))v6[1])(v1, v6[0]);
  }
}
