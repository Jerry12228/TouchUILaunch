__int64 __fastcall sub_187E12E90(__int64 a1, __int64 a2, __int64 a3)
{
  unsigned int v3; // ebx
  unsigned int v5; // edi
  char v6; // al
  __int64 v8; // rcx

  v3 = a2;
  if ( byte_18531A76B )
  {
    if ( !byte_184F1A574 )
      goto LABEL_3;
  }
  else
  {
    sub_18024B350(229531);
    byte_18531A76B = 1;
    if ( !byte_184F1A574 )
    {
LABEL_3:
      if ( (_BYTE)v3 )
        goto LABEL_32;
      if ( !*(_BYTE *)(qword_184F87FA0 + 203) )
        sub_180243EF0();
      v5 = 1;
      if ( !(unsigned __int8)sub_18786FF70(0, 0, 0) )
      {
        if ( !*(_BYTE *)(qword_184F87FA0 + 203) )
          sub_180243EF0();
        v5 = 3;
        if ( !sub_18786FE10(0) )
        {
          if ( !*(_BYTE *)(qword_184F87FA0 + 203) )
            sub_180243EF0();
          v5 = 0;
          if ( !sub_18786FEC0(0) )
          {
LABEL_32:
            if ( !*(_BYTE *)(qword_184F87FA0 + 203) )
              sub_180243EF0();
            v5 = 1;
            if ( !(unsigned __int8)sub_18786FF70(0, 0, 0) )
            {
              if ( !*(_BYTE *)(qword_184F87FA0 + 203) )
                sub_180243EF0();
              v5 = 3;
              if ( !sub_18786FE10(0) )
              {
                if ( !*(_BYTE *)(qword_184F87FA0 + 203) )
                  sub_180243EF0();
                v6 = sub_18786FEC0(0);
                v5 = *(_DWORD *)(a1 + 356);
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
  if ( !*(_BYTE *)(qword_184FBA640 + 203) )
    sub_180243EF0();
  v8 = *(_QWORD *)(*(_QWORD *)(qword_184E93B48 + 337056) + 1535424LL);
  if ( !v8 )
    sub_180B51440(0, a2, a3);
  return sub_187DF3180(v8, a1, v3);
}
