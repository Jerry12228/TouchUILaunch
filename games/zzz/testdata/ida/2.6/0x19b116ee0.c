__int64 __fastcall sub_19B116EE0(__int64 a1, unsigned int a2)
{
  __int128 v3; // xmm0
  __int128 v4; // xmm1
  __int128 v5; // xmm2
  __int128 v7; // [rsp+20h] [rbp-58h] BYREF
  __int128 v8; // [rsp+30h] [rbp-48h]
  __int128 v9; // [rsp+40h] [rbp-38h]
  __int128 v10; // [rsp+50h] [rbp-28h]
  int v11; // [rsp+60h] [rbp-18h]

  v10 = 0;
  v9 = 0;
  v8 = 0;
  v7 = 0;
  v11 = 0;
  qword_184916A88(a2, &v7);
  *(_DWORD *)(a1 + 64) = v11;
  v3 = v7;
  v4 = v8;
  v5 = v9;
  *(_OWORD *)(a1 + 48) = v10;
  *(_OWORD *)(a1 + 32) = v5;
  *(_OWORD *)(a1 + 16) = v4;
  *(_OWORD *)a1 = v3;
  return a1;
}
