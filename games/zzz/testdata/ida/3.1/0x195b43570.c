__int64 __fastcall sub_195B43570(__int64 a1, __int64 a2, __int64 a3)
{
  __int64 v3; // rcx
  int v4; // eax
  __int64 v5; // r8
  __int64 v6; // rcx
  __int64 v7; // rdx
  __int64 v9; // rax
  __int64 v10; // rdx
  __int64 v11; // rcx
  __int64 v12; // r8

  if ( byte_18589553C )
  {
    if ( !byte_1853AA758 )
    {
LABEL_3:
      v3 = qword_185450EB0;
      if ( !*(_BYTE *)(qword_185450EB0 + 203) )
        sub_180271E50(qword_185450EB0);
      v4 = sub_195B462A0(v3, a2, a3);
      v6 = qword_185450EB0;
      v7 = *(unsigned __int8 *)(qword_185450EB0 + 203);
      if ( v4 )
      {
        if ( !(_BYTE)v7 )
          sub_180271E50(qword_185450EB0);
        return sub_195B462A0(v6, v7, v5);
      }
      else
      {
        if ( !(_BYTE)v7 )
          sub_180271E50(qword_185450EB0);
        return sub_195B46180();
      }
    }
  }
  else
  {
    sub_1802793E0(0x43B4Cu);
    byte_18589553C = 1;
    if ( !byte_1853AA758 )
      goto LABEL_3;
  }
  v9 = sub_19017D760(54440);
  if ( !v9 )
    sub_180AC9120(v11, v10, v12);
  return sub_18B1B9FF0(v9);
}
