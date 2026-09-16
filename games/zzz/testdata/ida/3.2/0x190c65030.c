__int64 sub_190C65030()
{
  int v0; // esi

  if ( !byte_1853D3204 )
  {
    sub_18028C7A0(264644);
    byte_1853D3204 = 1;
    if ( *(_BYTE *)(qword_184F8C4E8 + 203) )
      goto LABEL_3;
LABEL_5:
    sub_1802851F0();
    goto LABEL_3;
  }
  if ( !*(_BYTE *)(qword_184F8C4E8 + 203) )
    goto LABEL_5;
LABEL_3:
  v0 = qword_184F785F8();
  return v0 * (unsigned int)qword_184F78600();
}
