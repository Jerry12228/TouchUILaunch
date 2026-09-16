__int64 sub_1934C8150()
{
  int v0; // eax
  char v1; // dl
  __int64 v3; // rax
  __int64 v4; // rdx
  __int64 v5; // rcx
  __int64 v6; // r8

  if ( byte_1853CCB97 )
  {
    if ( !byte_184F4D938 )
    {
LABEL_3:
      if ( !*(_BYTE *)(qword_184FC13E0 + 203) )
        sub_1802851F0();
      v0 = sub_1934C7D90();
      v1 = *(_BYTE *)(qword_184FC13E0 + 203);
      if ( v0 )
      {
        if ( !v1 )
          sub_1802851F0();
        return sub_1934C7D90();
      }
      else
      {
        if ( !v1 )
          sub_1802851F0();
        return sub_1934CA1E0();
      }
    }
  }
  else
  {
    sub_18028C7A0(238423);
    byte_1853CCB97 = 1;
    if ( !byte_184F4D938 )
      goto LABEL_3;
  }
  v3 = sub_192B2BD20(272760);
  if ( !v3 )
    sub_180935360(v5, v4, v6);
  return sub_18E2A0790(v3);
}
