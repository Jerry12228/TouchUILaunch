__int64 __fastcall sub_19166B410(__int64 a1, __int64 a2)
{
  __int64 result; // rax
  __int64 v5; // rax
  __int64 v6; // rdx
  __int64 v7; // rcx
  __int64 v8; // r8

  if ( byte_1853FE205 )
  {
    v5 = sub_19017D760(397141);
    if ( !v5 )
      sub_180AC9120(v7, v6, v8);
    return sub_18B18A0E0(v5, a1, a2);
  }
  else
  {
    result = qword_18540AD20(a1, a2);
    *(float *)(a1 + 52) = (float)(int)result;
  }
  return result;
}
