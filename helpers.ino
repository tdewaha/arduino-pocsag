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

void enable_fsaled()
{
  SET(PORTH, fsaledPin - 3);
}

void disable_fsaled()
{
  CLR(PORTH, fsaledPin - 3);
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
    Serial.print("\r\n" + String(s_address) + ";" + functions[function] + ";" + strMessage);
  }
}

void init_led() {
  for (int i = 0; i < 5; i++) {
    enable_pmbled();
    delay(100);
    disable_pmbled();
    delay(100);
  }
  disable_pmbled();
  disable_syncled();
  disable_fsaled();
}

