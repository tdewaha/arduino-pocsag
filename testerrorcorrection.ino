#define prmbWord 1431655765
#define syncWord 2094142936
#define idleWord 2055848343
#define MAXNUMBATCHES        12    //5
char ob[32];
unsigned int bch[1024], ecs[25];
unsigned long wordbuffer[(MAXNUMBATCHES * 16) + 1];
static const char *functions[4] = {"A", "B", "C", "D"};

void setup() {
  Serial.begin(115200);
  setupecc();
  Serial.println();
  initwb7();
  decode_wordbuffer(false);
  decode_wordbuffer(true);
  initwb6();
  decode_wordbuffer(false);
  decode_wordbuffer(true);

  initwb5();
  decode_wordbuffer(false);
  decode_wordbuffer(true);

  initwb4();
  decode_wordbuffer(false);
  decode_wordbuffer(true);

  initwb3();
  decode_wordbuffer(false);
  decode_wordbuffer(true);
  initwb2();
  decode_wordbuffer(false);
  decode_wordbuffer(true);
}

void loop() {
  // put your main code here, to run repeatedly:

}

int parity (unsigned long x)  // Parity check. If ==1, codeword is invalid
{
  x = x ^ x >> 16;
  x = x ^ x >> 8;
  x = x ^ x >> 4;
  x = x ^ x >> 2;
  x = x ^ x >> 1;
  return x & 1;
}

unsigned long extract_address(int idx) {
  unsigned long address = 0;
  int pos = idx / 2;// (idx - (idx / 8) * 8) / 2;

  for (int i = 1; i < 19; i++) {
    bitWrite(address, 21 - i, bitRead(wordbuffer[idx], 31 - i));
  }
  bitWrite(address, 0, bitRead((pos), 0));
  bitWrite(address, 1, bitRead((pos), 1));
  bitWrite(address, 2, bitRead((pos), 2));

  return address;
}

byte extract_function(int idx) {
  byte function = 0;
  bitWrite(function, 0, bitRead(wordbuffer[idx], 11));
  bitWrite(function, 1, bitRead(wordbuffer[idx], 12));
  return function;
}

String long2str(unsigned long binstr)
{
  String cwrep;
  int zeros = 32 - String(binstr, BIN).length();
  for (int i = 0; i < zeros; i++) {
    cwrep = cwrep + "0";
  }
  cwrep = cwrep + String(binstr, BIN);
  return cwrep;
}

String byte2str(byte binstr)
{
  String cwrep;
  int zeros = 8 - String(binstr, BIN).length();
  for (int i = 0; i < zeros; i++)
  {
    cwrep = cwrep + "0";
  }
  cwrep = cwrep + String(binstr, BIN);
  return cwrep;
}
void print_message(String s_address, byte function, char message[240]) {
  Serial.print(s_address);
  Serial.print(";");
  Serial.print(functions[function]);
  Serial.print(";");
  String msg = String(message);
  msg.replace("{", "ä");
  msg.replace("|", "ö");
  msg.replace("}", "ü");
  msg.replace("[", "Ä");
  msg.replace("]", "Ü");
  msg.replace("\\", "Ö");
  msg.replace("~", "ß");
  msg.replace("\n", "[0A]");
  msg.replace("\r", "[0D]");
  Serial.println(msg);
}

void decode_wordbuffer(bool ecc) {
  int address_counter = 0;
  unsigned long address[16];
  memset(address, 0, sizeof(address));
  byte function[16];
  memset(function, 0, sizeof(function));
  char message[240];
  memset(message, 0, sizeof(message));

  byte character = 0;
  int bcounter = 0;
  int ccounter = 0;
  boolean eot = false;

  for (int i = 0; i < ((MAXNUMBATCHES * 16) + 1); i++) {
    if (wordbuffer[i] == 0) continue;

    if (wordbuffer[i] == idleWord) continue;

    if (parity(wordbuffer[i]) == 1) {
      //continue;
    }
    if (ecc) {
      for (int obcnt = 0; obcnt < 32; obcnt++) {
        ob[obcnt] = bitRead(wordbuffer[i], 31 - obcnt);
      }

      if (ecd() < 3) {
        for (int obcnt = 0; obcnt < 32; obcnt++) {
          bitWrite(wordbuffer[i], 31 - obcnt, (int)ob[obcnt]);
        }
      }
    }

    if (bitRead(wordbuffer[i], 31) == 0) {
      if  ((i > 0 && wordbuffer[i - 1] == idleWord || address_counter == 0) && (parity(wordbuffer[i]) != 1)) {
        address[address_counter] = extract_address(i);
        function[address_counter] = extract_function(i);
        if (address_counter > 0) print_message(String(address[address_counter - 1]), function[address_counter - 1], message);
        eot = false;
        ccounter = 0;
        bcounter = 0;
        address_counter++;
      }
    } else {
      if (address[address_counter - 1] != 0 && ccounter < 240) {
        for (int c = 30; c > 10; c--) {
          bitWrite(character, bcounter, bitRead(wordbuffer[i], c));
          bcounter++;
          if (bcounter >= 7) {
            if (character == 4) {
              eot = true;
            }
            bcounter = 0;
            if (eot == false) {
              message[ccounter] = character;
              ccounter++;
            }
          }
        }
      }
    }
  }

  if (address_counter > 0) {
    print_message(String(address[address_counter - 1]), function[address_counter - 1], message);
  }
}



int bit10(int gin)
{
  int k = 0;

  for (int i = 0; i < 10; i++)
  {
    if ((gin & 0x01) != 0) k++;
    gin = gin >> 1;
  }
  return (k);
}

int ecd()
{
  int synd, b1, b2, i;
  int errors = 0, parity = 0;

  int ecc = 0x000;
  int acc = 0;

  // run through error detection and correction routine

  for (i = 0; i <= 20; i++)
  {
    if (ob[i] == 1)
    {
      ecc = ecc ^ ecs[i];
      parity = parity ^ 0x01;
    }
  }

  for (i = 21; i <= 30; i++)
  {
    acc = acc << 1;
    if (ob[i] == 1) acc = acc ^ 0x01;
  }

  synd = ecc ^ acc;
  Serial.println("SYND = "+String(synd));

  if (synd != 0) // if nonzero syndrome we have error
  {
    if (bch[synd] != 0) // check for correctable error
    {
      b1 = bch[synd] & 0x1f;
      b2 = bch[synd] >> 5;
      b2 = b2 & 0x1f;

      if (b2 != 0x1f)
      {
        ob[b2] = ob[b2] ^ 0x01;
        ecc = ecc ^ ecs[b2];
      }

      if (b1 != 0x1f)
      {
        ob[b1] = ob[b1] ^ 0x01;
        ecc = ecc ^ ecs[b1];
      }
      errors = bch[synd] >> 12;
    }
    else errors = 3;

    if (errors == 1) parity = parity ^ 0x01;
  }

  // check parity ....
  parity = (parity + bit10(ecc)) & 0x01;

  if (parity != ob[31]) errors++;

  if (errors > 3) errors = 3;

  return (errors);
}

void setupecc()
{
  unsigned int srr, j, k;
  int i, n;

  // calculate all information needed to implement error correction
  srr = 0x3B4;

  for (i = 0; i <= 20; i++)
  {
    ecs[i] = srr;
    if ((srr & 0x01) != 0) srr = (srr >> 1) ^ 0x3B4;
    else                   srr = srr >> 1;
  }

  // bch holds a syndrome look-up table telling which bits to correct
  // first 5 bits hold location of first error; next 5 bits hold location
  // of second error; bits 12 & 13 tell how many bits are bad
  for (i = 0; i < 255; i++) bch[i] = 0;

  for (n = 0; n <= 20; n++) // two errors in data
  {
    for (i = 0; i <= 20; i++)
    {
      j = (i << 5) + n;
      k = ecs[n] ^ ecs[i];
      bch[k] = j + 0x2000;
    }
  }

  // one error in data
  for (n = 0; n <= 20; n++)
  {
    k = ecs[n];
    j = n + (0x1f << 5);
    bch[k] = j + 0x1000;
  }

  // one error in data and one error in ecc portion
  for (n = 0; n <= 20; n++)
  {
    for (i = 0; i < 10; i++) // ecc screwed up bit
    {
      k = ecs[n] ^ (1 << i);
      j = n + (0x1f << 5);
      bch[k] = j + 0x2000;
    }
  }

  // one error in ecc
  for (n = 0; n < 10; n++)
  {
    k = 1 << n;
    bch[k] = 0x3ff + 0x1000;
  }

  // two errors in ecc
  for (n = 0; n < 10; n++)
  {
    for (i = 0; i < 10; i++)
    {
      if (i != n)
      {
        k = (1 << n) ^ (1 << i);
        bch[k] = 0x3ff + 0x2000;
      }
    }
  }
}




