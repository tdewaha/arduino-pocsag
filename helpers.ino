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

void enable_pmbled()
{
  SET(PORTH, pmbledPin - 3);
}

void disable_pmbled()
{
  CLR(PORTH, pmbledPin - 3);
}

void enable_syncled()
{
  SET(PORTH, syncledPin - 3);
}

void disable_syncled()
{
  CLR(PORTH, syncledPin - 3);
}

unsigned long extract_address(int idx) {
  unsigned long address = 0;
  int pos = idx / 2;
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
  delayMicroseconds(halfBitPeriod - halfBitPeriodTolerance);
  start_timer();
}

void timer_isr() {
  buffer = buffer << 1;
  bitWrite(buffer, 0, bitRead(PIND, 2));
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
  attachInterrupt(4, flank_isr, invert_signal + 2);
}

void stop_flank() {
  detachInterrupt(4);
}

void print_message(unsigned long s_address, byte function, char message[MSGLENGTH]) {
  if (s_address > 1949000 && s_address < 1955000) {
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
    Serial.println("\r\n" + String(s_address) + ";" + functions[function] + ";" + strMessage);
  }
}

void print_config() {
  String strpcheck =  ((enable_paritycheck == true) ? F("Parity Check ON")     : F("Parity Check OFF"));
  String strecc =                                     F("ECC-Mode     OFF");
  if (ecc_mode == 1) strecc =                         F("ECC-Mode     1 bit");
  if (ecc_mode == 2) strecc =                         F("ECC-Mode     2 bit");
  if (ecc_mode == 3) strecc =                         F("ECC-Mode    >2 bit");
  String strled =  ((enable_led == true) ?            F("LED          ON")     : F("LED          OFF"));
  String strdebug =                                   F("Debug Level  OFF (0)");
  if (debugLevel == 1) strdebug =                     F("Debug Level  CW 0+1 ");
  if (debugLevel == 2) strdebug =                     F("Debug Level  ALL (2)");
  String strinvert =  ((invert_signal == FALLING) ?   F("Input Level  NORMAL") : F("Input Level  INV."));

  Serial.println( strpcheck + F("\r\n") + strdebug + F("\r\n") + strecc + F("\r\n") + strled + F("\r\n") + strinvert);
}

void process_serial_input() {
  Serial.println();
  if (strstr(serbuf, "time")) {
    if (serbufcounter > 23) {
      tag = getIntFromString (serbuf, 1);
      monat = getIntFromString (serbuf, 2);
      jahr = getIntFromString (serbuf, 3);
      stunde = getIntFromString (serbuf, 4);
      minute = getIntFromString (serbuf, 5);
      sekunde = getIntFromString (serbuf, 6);

      // Ausgelesene Werte einer groben Plausibilitätsprüfung unterziehen:
      if (!checkDateTime(jahr, monat, tag, stunde, minute, sekunde)) {
        Serial.println(serbuf);
        Serial.println("Fehlerhafte Zeitangabe im 'set' Befehl");
        Serial.println("Beispiel: set 08.11.2017 10:54:00");
        return;
      }
      rtcWriteTime(jahr, monat, tag, stunde, minute, sekunde);
      Serial.println("Zeit und Datum wurden auf neue Werte gesetzt.");
    }
    Serial.println(strRTCDateTime());
    return;
  }
  if (strstr(serbuf, "h")|| strstr(serbuf, "?")) Serial.println(F("p0 = Parity Check disabled\r\np1 = Parity Check enabled\r\nd0 = Debug Level 0\r\nd1 = Debug Level 1\r\nd2 = Debug Level 2\r\ne0 = ECC disabled\r\ne1 = ECC enabled\r\nr0 = RAW output disabled\r\nr1 = RAW Output enabled\r\nl0 = LED disabled\r\nl1 = LED enabled\r\ni0 = Input normal\r\nl1 = Input inverted\nsh = print config"));
  if (strstr(serbuf, "p0")) {
    EEPROM.write(0, false);
    enable_paritycheck = false;
  }
  if (strstr(serbuf, "p1")) {
    EEPROM.write(0, true);
    enable_paritycheck = true;
  }
  if (strstr(serbuf, "d0")) {
    EEPROM.write(1, 0);
    debugLevel = 0;
  }
  if (strstr(serbuf, "d1")) {
    EEPROM.write(1, 1);
    debugLevel = 1;
  }
  if (strstr(serbuf, "d2")) {
    EEPROM.write(1, 2);
    debugLevel = 2;
  }
  if (strstr(serbuf, "e0")) {
    EEPROM.write(2, 0);
    ecc_mode = 0;
  }
  if (strstr(serbuf, "e1")) {
    EEPROM.write(2, 1);
    ecc_mode = 1;
  }
  if (strstr(serbuf, "e2")) {
    EEPROM.write(2, 2);
    ecc_mode = 2;
  }
  if (strstr(serbuf, "e3")) {
    EEPROM.write(2, 3);
    ecc_mode = 3;
  }
  if (strstr(serbuf, "l0")) {
    EEPROM.write(3, false);
    enable_led = false;
  }
  if (strstr(serbuf, "l1")) {
    EEPROM.write(3, true);
    enable_led = true;
  }
  if (strstr(serbuf, "i0")) {
    EEPROM.write(4, FALLING);
    invert_signal = FALLING;
  }
  if (strstr(serbuf, "i1")) {
    EEPROM.write(4, RISING);
    invert_signal = RISING;
  }
  print_config();
}



