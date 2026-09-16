__int64 __fastcall sub_18B108CE0(__int64 a1, __int64 a2, __int64 a3)
{
  __int64 v3; // rcx
  __int64 v4; // rax
  __int64 *v5; // rdi
  __int64 v6; // rax
  __int64 v7; // rdx
  __int64 v8; // rsi
  _QWORD v10[4]; // [rsp+28h] [rbp-20h] BYREF

  if ( byte_184CCBD15 )
  {
    v3 = qword_184950E90;
    if ( *(_BYTE *)(qword_184950E90 + 203) )
      goto LABEL_3;
  }
  else
  {
    sub_18023BC30(121429);
    byte_184CCBD15 = 1;
    v3 = qword_184950E90;
    if ( *(_BYTE *)(qword_184950E90 + 203) )
    {
LABEL_3:
      v4 = *(_QWORD *)(qword_184885C60 + 169384);
      if ( v4 )
        goto LABEL_4;
LABEL_14:
      sub_180979760(v3, a2, a3);
    }
  }
  sub_180234770();
  v4 = *(_QWORD *)(qword_184885C60 + 169384);
  if ( !v4 )
    goto LABEL_14;
LABEL_4:
  v5 = *(__int64 **)(v4 + 96);
  if ( !v5 )
    sub_180979760(v3, a2, a3);
  v6 = *v5;
  if ( *(_WORD *)(*v5 + 198) )
  {
    v7 = *(_QWORD *)(v6 + 48);
    v8 = 0;
    while ( *(_QWORD *)(v7 + v8) != qword_18495F048 )
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
    sub_1802319F0(v10, v5, qword_18495F048, 0);
    return ((__int64 (__fastcall *)(__int64 *, _QWORD))v10[0])(v5, v10[1]);
  }
}
