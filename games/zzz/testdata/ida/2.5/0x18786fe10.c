char __fastcall sub_18786FE10(unsigned int a1)
{
  __int64 v3; // rcx

  if ( byte_18531A612 )
  {
    if ( !byte_184F07734 )
    {
LABEL_3:
      if ( !a1 )
      {
        if ( !*(_BYTE *)(qword_184F87FA0 + 203) )
          sub_180243EF0();
        a1 = sub_18786FC80();
      }
      return a1 == 1;
    }
  }
  else
  {
    sub_18024B350(229186);
    byte_18531A612 = 1;
    if ( !byte_184F07734 )
      goto LABEL_3;
  }
  if ( !*(_BYTE *)(qword_184FBA640 + 203) )
    sub_180243EF0();
  v3 = *(_QWORD *)(*(_QWORD *)(qword_184E93B48 + 337056) + 916416LL);
  if ( !v3 )
    sub_180B51440();
  return sub_187DF1700(v3, a1);
}
