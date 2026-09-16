__int64 sub_1934D9860()
{
  __int64 v0; // rcx
  int v1; // esi

  if ( !byte_185891F7B )
  {
    sub_1802793E0(0x4058Bu);
    byte_185891F7B = 1;
    v0 = qword_18541B188;
    if ( *(_BYTE *)(qword_18541B188 + 203) )
      goto LABEL_3;
LABEL_5:
    sub_180271E50(v0);
    goto LABEL_3;
  }
  v0 = qword_18541B188;
  if ( !*(_BYTE *)(qword_18541B188 + 203) )
    goto LABEL_5;
LABEL_3:
  v1 = qword_185407458();
  return v1 * (unsigned int)qword_185407460();
}
