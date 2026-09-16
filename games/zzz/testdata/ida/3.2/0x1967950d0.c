__int64 __fastcall sub_1967950D0(__int64 a1, unsigned int a2)
{
  unsigned int v4; // edi
  int v5; // eax
  __int64 v7; // rax
  __int64 v8; // rdx
  __int64 v9; // rcx
  __int64 v10; // r8
  __int64 v11; // rax
  __int64 v12; // rdx
  __int64 v13; // rcx
  __int64 v14; // r8
  __int64 v15; // rax
  __int64 v16; // rdx
  __int64 v17; // rcx
  __int64 v18; // r8
  __int64 v19; // rax
  __int64 v20; // rdx
  __int64 v21; // rcx
  __int64 v22; // r8
  __int64 v23; // rax
  __int64 v24; // rdx
  __int64 v25; // rcx
  __int64 v26; // r8
  __int64 v27; // rax
  __int64 v28; // rdx
  __int64 v29; // rcx
  __int64 v30; // r8
  char v31; // al
  __int64 v32; // rax
  __int64 v33; // rdx
  __int64 v34; // rcx
  __int64 v35; // r8

  if ( byte_1853BAC14 )
  {
    if ( !byte_184F256A5 )
      goto LABEL_3;
  }
  else
  {
    sub_18028C7A0(164820);
    byte_1853BAC14 = 1;
    if ( !byte_184F256A5 )
    {
LABEL_3:
      if ( (_BYTE)a2 )
        goto LABEL_4;
      if ( *(_BYTE *)(qword_184FC13E0 + 203) )
      {
        if ( byte_1853CCB58 )
          goto LABEL_25;
      }
      else
      {
        sub_1802851F0();
        if ( byte_1853CCB58 )
        {
LABEL_25:
          if ( !byte_184F4D8F1 )
            goto LABEL_26;
          goto LABEL_52;
        }
      }
      sub_18028C7A0(238360);
      byte_1853CCB58 = 1;
      if ( !byte_184F4D8F1 )
      {
LABEL_26:
        if ( !*(_BYTE *)(qword_184FC13E0 + 203) )
          sub_1802851F0();
        v4 = 1;
        if ( (unsigned int)sub_1934C8150() - 3 < 2 )
          return v4;
LABEL_29:
        if ( *(_BYTE *)(qword_184FC13E0 + 203) )
        {
          if ( byte_1853CCBC5 )
            goto LABEL_31;
        }
        else
        {
          sub_1802851F0();
          if ( byte_1853CCBC5 )
          {
LABEL_31:
            if ( !byte_184F4D966 )
              goto LABEL_32;
            goto LABEL_62;
          }
        }
        sub_18028C7A0(238469);
        byte_1853CCBC5 = 1;
        if ( !byte_184F4D966 )
        {
LABEL_32:
          if ( !*(_BYTE *)(qword_184FC13E0 + 203) )
            sub_1802851F0();
          v4 = 3;
          if ( (unsigned int)sub_1934C8150() == 1 )
            return v4;
LABEL_35:
          if ( *(_BYTE *)(qword_184FC13E0 + 203) )
          {
            if ( byte_1853CCB45 )
              goto LABEL_37;
          }
          else
          {
            sub_1802851F0();
            if ( byte_1853CCB45 )
            {
LABEL_37:
              if ( !byte_184F4D8DE )
                goto LABEL_38;
              goto LABEL_72;
            }
          }
          sub_18028C7A0(238341);
          byte_1853CCB45 = 1;
          if ( !byte_184F4D8DE )
          {
LABEL_38:
            if ( !*(_BYTE *)(qword_184FC13E0 + 203) )
              sub_1802851F0();
            v4 = 0;
            if ( (unsigned int)sub_1934C8150() == 2 )
              return v4;
LABEL_4:
            if ( *(_BYTE *)(qword_184FC13E0 + 203) )
            {
              if ( byte_1853CCB58 )
                goto LABEL_6;
            }
            else
            {
              sub_1802851F0();
              if ( byte_1853CCB58 )
              {
LABEL_6:
                if ( !byte_184F4D8F1 )
                  goto LABEL_7;
                goto LABEL_47;
              }
            }
            sub_18028C7A0(238360);
            byte_1853CCB58 = 1;
            if ( !byte_184F4D8F1 )
            {
LABEL_7:
              if ( !*(_BYTE *)(qword_184FC13E0 + 203) )
                sub_1802851F0();
              v4 = 1;
              if ( (unsigned int)sub_1934C8150() - 3 < 2 )
                return v4;
LABEL_10:
              if ( *(_BYTE *)(qword_184FC13E0 + 203) )
              {
                if ( byte_1853CCBC5 )
                  goto LABEL_12;
              }
              else
              {
                sub_1802851F0();
                if ( byte_1853CCBC5 )
                {
LABEL_12:
                  if ( !byte_184F4D966 )
                    goto LABEL_13;
                  goto LABEL_57;
                }
              }
              sub_18028C7A0(238469);
              byte_1853CCBC5 = 1;
              if ( !byte_184F4D966 )
              {
LABEL_13:
                if ( !*(_BYTE *)(qword_184FC13E0 + 203) )
                  sub_1802851F0();
                v4 = 3;
                if ( (unsigned int)sub_1934C8150() == 1 )
                  return v4;
LABEL_16:
                if ( *(_BYTE *)(qword_184FC13E0 + 203) )
                {
                  if ( byte_1853CCB45 )
                    goto LABEL_18;
                }
                else
                {
                  sub_1802851F0();
                  if ( byte_1853CCB45 )
                  {
LABEL_18:
                    if ( !byte_184F4D8DE )
                      goto LABEL_19;
                    goto LABEL_67;
                  }
                }
                sub_18028C7A0(238341);
                byte_1853CCB45 = 1;
                if ( !byte_184F4D8DE )
                {
LABEL_19:
                  if ( !*(_BYTE *)(qword_184FC13E0 + 203) )
                    sub_1802851F0();
                  v5 = sub_1934C8150();
                  v4 = *(_DWORD *)(a1 + 392);
                  if ( v5 != 2 )
                    return v4;
                  return 2 * (unsigned int)(v4 == 2);
                }
LABEL_67:
                v27 = sub_192B2BD20(272670);
                if ( !v27 )
                  sub_180935360(v29, v28, v30);
                v31 = sub_18E276540(v27, 0);
                v4 = *(_DWORD *)(a1 + 392);
                if ( !v31 )
                  return v4;
                return 2 * (unsigned int)(v4 == 2);
              }
LABEL_57:
              v19 = sub_192B2BD20(272806);
              if ( !v19 )
                sub_180935360(v21, v20, v22);
              v4 = 3;
              if ( (unsigned __int8)sub_18E276540(v19, 0) )
                return v4;
              goto LABEL_16;
            }
LABEL_47:
            v11 = sub_192B2BD20(272689);
            if ( !v11 )
              sub_180935360(v13, v12, v14);
            v4 = 1;
            if ( (unsigned __int8)sub_18E2A0DB0(v11, 0, 0, 0) )
              return v4;
            goto LABEL_10;
          }
LABEL_72:
          v32 = sub_192B2BD20(272670);
          if ( !v32 )
            sub_180935360(v34, v33, v35);
          v4 = 0;
          if ( (unsigned __int8)sub_18E276540(v32, 0) )
            return v4;
          goto LABEL_4;
        }
LABEL_62:
        v23 = sub_192B2BD20(272806);
        if ( !v23 )
          sub_180935360(v25, v24, v26);
        v4 = 3;
        if ( (unsigned __int8)sub_18E276540(v23, 0) )
          return v4;
        goto LABEL_35;
      }
LABEL_52:
      v15 = sub_192B2BD20(272689);
      if ( !v15 )
        sub_180935360(v17, v16, v18);
      v4 = 1;
      if ( (unsigned __int8)sub_18E2A0DB0(v15, 0, 0, 0) )
        return v4;
      goto LABEL_29;
    }
  }
  v7 = sub_192B2BD20(108261);
  if ( !v7 )
    sub_180935360(v9, v8, v10);
  return sub_18E29C590(v7, a1, a2);
}
