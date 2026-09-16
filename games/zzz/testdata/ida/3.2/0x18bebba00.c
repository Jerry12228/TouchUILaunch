__int64 __fastcall sub_18BEBBA00(__int64 a1, __int64 a2, float a3)
{
  __int64 v5; // rdx
  __int64 v6; // r8
  __int64 v7; // rcx
  __int128 v9; // [rsp+20h] [rbp-48h] BYREF
  __int64 v10; // [rsp+30h] [rbp-38h]

  v9 = 0;
  LOBYTE(v9) = 1;
  *((float *)&v9 + 1) = a3;
  DWORD2(v9) = qword_184F7BEF0(a1, a2);
  v10 = a2;
  v7 = *(_QWORD *)(a1 + 16);
  if ( !v7 )
    sub_180935360(0, v5, v6);
  return sub_18FED1A40(v7, &v9);
}
