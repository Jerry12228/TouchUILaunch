__int64 __fastcall sub_18B103BE0(__int64 a1, __int64 a2, __int64 a3)
{
  int v3; // eax
  char v4; // dl

  if ( byte_184CCBD51 )
  {
    if ( !byte_1848FD7F6 )
    {
LABEL_3:
      if ( !*(_BYTE *)(qword_184950E90 + 203) )
        sub_180234770();
      v3 = sub_18B10D400();
      v4 = *(_BYTE *)(qword_184950E90 + 203);
      if ( v3 )
      {
        if ( !v4 )
          sub_180234770();
        return sub_18B10D400();
      }
      else
      {
        if ( !v4 )
          sub_180234770();
        return sub_18B108CE0();
      }
    }
  }
  else
  {
    sub_18023BC30(121489);
    byte_184CCBD51 = 1;
    if ( !byte_1848FD7F6 )
      goto LABEL_3;
  }
  if ( !*(_BYTE *)(qword_184985118 + 203) )
    sub_180234770();
  if ( !*(_QWORD *)(*(_QWORD *)(qword_184885C60 + 255088) + 2163664LL) )
    sub_180979760(0, a2, a3);
  return sub_18705CCB0();
}
