char __fastcall sub_18786FEC0(unsigned int a1)
{
  __int64 v3; // rcx

  if ( byte_18531A613 )
  {
    if ( !byte_184F07735 )
    {
LABEL_3:
      if ( !a1 )
      {
        if ( !*(_BYTE *)(qword_184F87FA0 + 203) )
          sub_180243EF0();
        a1 = sub_18786FC80();
      }
      return a1 == 2;
    }
  }
  else
  {
    sub_18024B350(229187);
    byte_18531A613 = 1;
    if ( !byte_184F07735 )
      goto LABEL_3;
  }
  if ( !*(_BYTE *)(qword_184FBA640 + 203) )
    sub_180243EF0();
  v3 = *(_QWORD *)(*(_QWORD *)(qword_184E93B48 + 337056) + 916424LL);
  if ( !v3 )
    sub_180B51440();
  return sub_187DF1700(v3, a1);
}
