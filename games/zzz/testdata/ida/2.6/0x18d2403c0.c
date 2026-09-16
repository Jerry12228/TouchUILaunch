char __fastcall sub_18D2403C0(unsigned int a1, __int64 a2, __int64 a3)
{
  __int64 v4; // rcx
  __int64 v6; // rcx

  if ( byte_184CCBCDE )
  {
    if ( !byte_1848FD776 )
    {
LABEL_3:
      if ( !a1 )
      {
        v4 = qword_184950E90;
        if ( !*(_BYTE *)(qword_184950E90 + 203) )
          sub_180234770();
        a1 = sub_18B103BE0(v4, a2, a3);
      }
      return a1 == 1;
    }
  }
  else
  {
    sub_18023BC30(121374);
    byte_184CCBCDE = 1;
    if ( !byte_1848FD776 )
      goto LABEL_3;
  }
  if ( !*(_BYTE *)(qword_184985118 + 203) )
    sub_180234770();
  v6 = *(_QWORD *)(*(_QWORD *)(qword_184885C60 + 255088) + 2162640LL);
  if ( !v6 )
    sub_180979760(0, a2, a3);
  return sub_1870573C0(v6, a1);
}
