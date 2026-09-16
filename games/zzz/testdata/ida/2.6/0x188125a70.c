__int64 __fastcall sub_188125A70(__int64 a1, __int64 a2, __int64 a3)
{
  unsigned int v3; // ebx
  unsigned int v5; // edi
  char v6; // al
  __int64 v8; // rcx

  v3 = a2;
  if ( byte_184CD40E7 )
  {
    if ( !byte_1848FA202 )
      goto LABEL_3;
  }
  else
  {
    sub_18023BC30(155175);
    byte_184CD40E7 = 1;
    if ( !byte_1848FA202 )
    {
LABEL_3:
      if ( (_BYTE)v3 )
        goto LABEL_32;
      if ( !*(_BYTE *)(qword_184950E90 + 203) )
        sub_180234770();
      v5 = 1;
      if ( !(unsigned __int8)sub_18B104EB0(0, 0, 0) )
      {
        if ( !*(_BYTE *)(qword_184950E90 + 203) )
          sub_180234770();
        v5 = 3;
        if ( !(unsigned __int8)sub_18D2403C0(0) )
        {
          if ( !*(_BYTE *)(qword_184950E90 + 203) )
            sub_180234770();
          v5 = 0;
          if ( !(unsigned __int8)sub_18B103B30(0) )
          {
LABEL_32:
            if ( !*(_BYTE *)(qword_184950E90 + 203) )
              sub_180234770();
            v5 = 1;
            if ( !(unsigned __int8)sub_18B104EB0(0, 0, 0) )
            {
              if ( !*(_BYTE *)(qword_184950E90 + 203) )
                sub_180234770();
              v5 = 3;
              if ( !(unsigned __int8)sub_18D2403C0(0) )
              {
                if ( !*(_BYTE *)(qword_184950E90 + 203) )
                  sub_180234770();
                v6 = sub_18B103B30(0);
                v5 = *(_DWORD *)(a1 + 360);
                if ( v6 )
                  return 2 * (unsigned int)(v5 == 2);
              }
            }
          }
        }
      }
      return v5;
    }
  }
  if ( !*(_BYTE *)(qword_184985118 + 203) )
    sub_180234770();
  v8 = *(_QWORD *)(*(_QWORD *)(qword_184885C60 + 255088) + 2053168LL);
  if ( !v8 )
    sub_180979760(0, a2, a3);
  return sub_187059160(v8, a1, v3);
}
