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
  String strdebug =  F("Debug Level 0 (OFF)");
  if (debugLevel == 1) strdebug = F("Debug Level 1 (CW 0 + 1)");
  if (debugLevel == 2) strdebug = F("Debug Level 2 (ALL)");
  Serial.println(String(F("Parity Check ")) + strpcheck + "\n" + strdebug);
}

void print_message(String s_address, byte function, char message[MSGLENGTH]) {
  Serial.print(s_address);
  Serial.print(";");
  Serial.print(functions[function]);
  Serial.print(";");
  Serial.println(message);
}

void process_serial_input(String serread) {
  if (serread.startsWith("h") || serread.startsWith("?")) Serial.println(F("p0 = Parity Check disabled\np1 = Parity Check enabled\nd0 = Debug Level 0\nd1 = Debug Level 1\nd2 = Debug Level 2\nsh = print config"));
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
}

