__int64 sub_18786FC80()
{
  int v0; // eax
  char v1; // dl

  if ( byte_18531A611 )
  {
    if ( !byte_184F07733 )
    {
LABEL_3:
      if ( !*(_BYTE *)(qword_184F87FA0 + 203) )
        sub_180243EF0();
      v0 = sub_187862010();
      v1 = *(_BYTE *)(qword_184F87FA0 + 203);
      if ( v0 )
      {
        if ( !v1 )
          sub_180243EF0();
        return sub_187862010();
      }
      else
      {
        if ( !v1 )
          sub_180243EF0();
        return sub_18786FB60();
      }
    }
  }
  else
  {
    sub_18024B350(229185);
    byte_18531A611 = 1;
    if ( !byte_184F07733 )
      goto LABEL_3;
  }
  if ( !*(_BYTE *)(qword_184FBA640 + 203) )
    sub_180243EF0();
  if ( !*(_QWORD *)(*(_QWORD *)(qword_184E93B48 + 337056) + 916408LL) )
    sub_180B51440();
  return sub_187DF6580();
}
