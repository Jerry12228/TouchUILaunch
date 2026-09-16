__int64 __fastcall sub_1934C7090(unsigned int a1)
{
  __int64 v2; // rax
  __int64 *v3; // rdi
  __int64 v4; // rax
  __int64 v5; // rdx
  __int64 v6; // r9
  __int64 v8; // rcx
  __int64 v9; // rax
  _QWORD v10[4]; // [rsp+28h] [rbp-20h] BYREF

  if ( byte_1853CCB3B )
  {
    if ( !byte_184F4D8D4 )
      goto LABEL_3;
LABEL_14:
    v9 = sub_192B2BD20(272660);
    if ( !v9 )
      sub_180935360();
    return sub_18E26FE80(v9, a1);
  }
  sub_18028C7A0(238331);
  byte_1853CCB3B = 1;
  if ( byte_184F4D8D4 )
    goto LABEL_14;
LABEL_3:
  if ( *(_BYTE *)(qword_184FC13E0 + 203) )
  {
    v2 = *(_QWORD *)(qword_184ECD400 + 189544);
    if ( v2 )
      goto LABEL_5;
LABEL_17:
    sub_180935360();
  }
  sub_1802851F0();
  v2 = *(_QWORD *)(qword_184ECD400 + 189544);
  if ( !v2 )
    goto LABEL_17;
LABEL_5:
  v3 = *(__int64 **)(v2 + 152);
  if ( !v3 )
    sub_180935360();
  v4 = *v3;
  if ( *(_WORD *)(*v3 + 198) )
  {
    v5 = *(_QWORD *)(v4 + 80);
    v6 = 0;
    while ( *(_QWORD *)(v5 + v6) != qword_184FFE8D8 )
    {
      v6 += 16;
      if ( 16LL * *(unsigned __int16 *)(*v3 + 198) == v6 )
        goto LABEL_10;
    }
    v8 = *(_DWORD *)(v5 + v6 + 8) + 1;
    return (*(__int64 (__fastcall **)(__int64 *, _QWORD, _QWORD))(v4 + 8 * v8 + 208))(
             v3,
             a1,
             *(_QWORD *)(v4 + 8 * (v8 + *(unsigned __int16 *)(v4 + 196)) + 208));
  }
  else
  {
LABEL_10:
    sub_180282510(v10, v3, qword_184FFE8D8, 1);
    return ((__int64 (__fastcall *)(__int64 *, _QWORD, _QWORD))v10[0])(v3, a1, v10[1]);
  }
}
