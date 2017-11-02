/**
     Provides helper functions for the pocsag decoder
     Copyright (C) 2013  Tom de Waha

     This program is free software; you can redistribute it and/or modify
     it under the terms of the GNU General Public License as published by
     the Free Software Foundation; either version 2 of the License, or
     (at your option) any later version.
     This program is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
     GNU General Public License for more details.
     You should have received a copy of the GNU General Public License along
     with this program; if not, write to the Free Software Foundation, Inc.,
     51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
**/


#define CLR(x,y) (x&=(~(1<<y)))
#define SET(x,y) (x|=(1<<y))
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

int parity (unsigned long x)  // Parity check. If ==1, codeword is invalid
{
  x = x ^ x >> 16;
  x = x ^ x >> 8;
  x = x ^ x >> 4;
  x = x ^ x >> 2;
  x = x ^ x >> 1;
  return x & 1;
}

void enable_trigger()
{
  SET(PORTD, triggerPin);
}

void disable_trigger()
{
  CLR(PORTD, triggerPin);
}

void enable_led()
{
  SET(PORTD, ledPin);
}

void disable_led()
{
  CLR(PORTD, ledPin);
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

void flank_isr() {
  delayMicroseconds(halfBitPeriod - 20);
  start_timer();
}

void timer_isr() {
  buffer = buffer << 1;
  bitWrite(buffer, 0, bitRead(PIND, 3));
  if (state > STATE_WAIT_FOR_PRMB) bitcounter++;
}

void start_timer() {
  Timer1.restart();
  Timer1.attachInterrupt(timer_isr);
}

void stop_timer() {
  Timer1.detachInterrupt();
}

void start_flank() {
  attachInterrupt(1, flank_isr, RISING);
}

void stop_flank() {
  detachInterrupt(1);
}

void print_config() {
  String strpcheck =  ((enableParityCheck == true) ? "enabled" : "disabled");
  String strecc =  ((enable_ecc == true) ? F("ECC enabled") : F("ECC disabled"));
  String strdebug =  F("Debug Level 0 (OFF)");
  if (debugLevel == 1) strdebug = F("Debug Level 1 (CW 0 + 1)");
  if (debugLevel == 2) strdebug = F("Debug Level 2 (ALL)");
  Serial.println(String(F("Parity Check ")) + strpcheck + F("\n") + strdebug + F("\n") + strecc);
}

void print_message(String NetID, String s_address, byte function, char message[MSGLENGTH]) {
  Serial.print(NetID);
  Serial.print(";");
  Serial.print(s_address);
  Serial.print(";");
  Serial.print(functions[function]);
  Serial.print(";");
  String strMessage = "";
  for (int i = 0; i < MSGLENGTH; i++)  {
    if (message[i] > 31 && message[i] < 127) {
      switch (message[i]) {
        case '|':
          strMessage += "ö";
          break;
        case '{':
          strMessage += "ä";
          break;
        case '}':
          strMessage += "ü";
          break;
        case '[':
          strMessage += "Ä";
          break;
        case ']':
          strMessage += "Ü";
          break;
        case '\\':
          strMessage += "Ö";
          break;
        case '~':
          strMessage += "ß";
          break;
        case '\n':
          strMessage += "[0A]";
          break;
        case '\r':
          strMessage += "[0D]";
          break;
        default:
          strMessage += message[i];
      }
    }
  }
  Serial.println(strMessage);
}

void process_serial_input(String serread) {
  if (serread.startsWith("h") || serread.startsWith("?")) Serial.println(F("p0 = Parity Check disabled\np1 = Parity Check enabled\nd0 = Debug Level 0\nd1 = Debug Level 1\nd2 = Debug Level 2\ne0 = ECC disabled\ne1 = ECC enabled\nsh = print config"));
  if (serread.startsWith("sh")) {
    print_config();
  }
  if (serread.startsWith("p0")) {
    EEPROM.write(0, false);
    enableParityCheck = false;
    Serial.println(F("Parity Check disabled"));
  }
  if (serread.startsWith("p1")) {
    EEPROM.write(0, true);
    enableParityCheck = true;
    Serial.println(F("Parity Check enabled"));
  }
  if (serread.startsWith("d0")) {
    EEPROM.write(1, 0);
    debugLevel = 0;
    Serial.println(F("Debug Level 0 (OFF)"));
  }
  if (serread.startsWith("d1")) {
    EEPROM.write(1, 1);
    debugLevel = 1;
    Serial.println(F("Debug Level 1 (CW 0 + 1)"));
  }
  if (serread.startsWith("d2")) {
    EEPROM.write(1, 2);
    debugLevel = 2;
    Serial.println(F("Debug Level 2 (ALL)"));
  }
  if (serread.startsWith("e0")) {
    EEPROM.write(2, false);
    enable_ecc = false;
    Serial.println(F("ECC disabled"));
  }
  if (serread.startsWith("e1")) {
    EEPROM.write(2, true);
    enable_ecc = true;
    Serial.println(F("ECC enabled"));
  }
}

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

  // run through error detection and correction routine

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
  Serial.println("SYND = " + String(synd));

  if (synd != 0) {// if nonzero syndrome we have error
    if (bch[synd] != 0) { // check for correctable error
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

  for (i = 0; i <= 20; i++) {
    ecs[i] = srr;
    if ((srr & 0x01) != 0) srr = (srr >> 1) ^ 0x3B4;
    else                   srr = srr >> 1;
  }

  // bch holds a syndrome look-up table telling which bits to correct
  // first 5 bits hold location of first error; next 5 bits hold location
  // of second error; bits 12 & 13 tell how many bits are bad
  for (i = 0; i < 255; i++) bch[i] = 0;

  for (n = 0; n <= 20; n++) { // two errors in data
    for (i = 0; i <= 20; i++) {
      j = (i << 5) + n;
      k = ecs[n] ^ ecs[i];
      bch[k] = j + 0x2000;
    }
  }

  // one error in data
  for (n = 0; n <= 20; n++) {
    k = ecs[n];
    j = n + (0x1f << 5);
    bch[k] = j + 0x1000;
  }

  // one error in data and one error in ecc portion
  for (n = 0; n <= 20; n++) {
    for (i = 0; i < 10; i++) {// ecc screwed up bit
      k = ecs[n] ^ (1 << i);
      j = n + (0x1f << 5);
      bch[k] = j + 0x2000;
    }
  }

  // one error in ecc
  for (n = 0; n < 10; n++) {
    k = 1 << n;
    bch[k] = 0x3ff + 0x1000;
  }

  // two errors in ecc
  for (n = 0; n < 10; n++) {
    for (i = 0; i < 10; i++) {
      if (i != n) {
        k = (1 << n) ^ (1 << i);
        bch[k] = 0x3ff + 0x2000;
      }
    }
  }
}


