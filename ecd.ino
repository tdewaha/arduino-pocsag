byte bit10(byte gin) {
  int k = 0;

  for (int i = 0; i < 10; i++) {
    if ((gin & 0x01) != 0) k++;
    gin = gin >> 1;
  }
  return (k);
}

byte ecd() {
  int synd, b1, b2, i;
  byte errors = 0, parity = 0;

  int ecc = 0x000;
  int acc = 0;

  for (i = 0; i <= 20; i++) {
    if (ob[i] == 1)
    {
      ecc = ecc ^ ecs[i];
      parity = parity ^ 0x01;
    }
  }

  for (i = 21; i <= 30; i++) {
    acc = acc << 1;
    if (ob[i] == 1) acc = acc ^ 0x01;
  }

  synd = ecc ^ acc;

  if (synd != 0) {
    if (bch[synd] != 0) {
      b1 = bch[synd] & 0x1f;
      b2 = bch[synd] >> 5;
      b2 = b2 & 0x1f;

      if (b2 != 0x1f) {
        ob[b2] = ob[b2] ^ 0x01;
        ecc = ecc ^ ecs[b2];
      }

      if (b1 != 0x1f) {
        ob[b1] = ob[b1] ^ 0x01;
        ecc = ecc ^ ecs[b1];
      }
      errors = bch[synd] >> 12;
    }
    else errors = 3;

    if (errors == 1) parity = parity ^ 0x01;
  }

  parity = (parity + bit10(ecc)) & 0x01;

  if (parity != ob[31]) errors++;

  if (errors > 3) errors = 3;

  return (errors);
}

void setupecc()
{
  unsigned int srr, j, k;
  int i, n;

  srr = 0x3B4;

  for (i = 0; i <= 20; i++) {
    ecs[i] = srr;
    if ((srr & 0x01) != 0) srr = (srr >> 1) ^ 0x3B4;
    else                   srr = srr >> 1;
  }

  for (i = 0; i < 255; i++) bch[i] = 0;

  for (n = 0; n <= 20; n++) {
    for (i = 0; i <= 20; i++) {
      j = (i << 5) + n;
      k = ecs[n] ^ ecs[i];
      bch[k] = j + 0x2000;
    }
  }

  for (n = 0; n <= 20; n++) {
    k = ecs[n];
    j = n + (0x1f << 5);
    bch[k] = j + 0x1000;
  }

  for (n = 0; n <= 20; n++) {
    for (i = 0; i < 10; i++) {
      k = ecs[n] ^ (1 << i);
      j = n + (0x1f << 5);
      bch[k] = j + 0x2000;
    }
  }

  for (n = 0; n < 10; n++) {
    k = 1 << n;
    bch[k] = 0x3ff + 0x1000;
  }

  for (n = 0; n < 10; n++) {
    for (i = 0; i < 10; i++) {
      if (i != n) {
        k = (1 << n) ^ (1 << i);
        bch[k] = 0x3ff + 0x2000;
      }
    }
  }
}
