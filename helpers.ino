/**
 *   Provides helper functions for the pocsag decoder
 *   Copyright (C) 2013  Tom de Waha

 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *   You should have received a copy of the GNU General Public License along
 *   with this program; if not, write to the Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
**/

#define CLR(x,y) (x&=(~(1<<y)))
#define SET(x,y) (x|=(1<<y))
String long2str(unsigned long binstr)
{
   String cwrep;
   int zeros = 32 - String(binstr,BIN).length();
   for (int i=0; i<zeros; i++) {
      cwrep = cwrep + "0";
   }
   cwrep = cwrep + String(binstr,BIN);
   return cwrep;  
}

String byte2str(byte binstr)
{
   String cwrep;
   int zeros = 8 - String(binstr,BIN).length();
   for (int i = 0; i < zeros; i++) 
   {
     cwrep = cwrep + "0";
   } 
   cwrep = cwrep + String(binstr,BIN);
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

void trigger()
{
  SET(PORTD, triggerPin);
  CLR(PORTD, triggerPin);     
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

char checkUmlaut(byte ascii) {
  switch (ascii) {
    case '{': return 'ä'; break; // ä 0x84
    case '|': return 'ö'; break; // ö 0x94
    case '}': return 'ü'; break; // ü 0x81
    case '[': return 'Ä'; break; // Ä 0x8E
    case ']': return 'Ü'; break; // Ü 0x99
    case '\\': return 'Ö'; break; // Ö 0x9A
    case '~' : return 'ß'; break; // ß 0xE1
    default:  return  ascii; break;
  }
}

void print_message(String s_address, byte function, char message[MSGLENGTH]) {
  Serial.print("[");
  Serial.print(s_address);
  Serial.print("];");
  Serial.print(functions[function]);
  Serial.print(";");
  Serial.println(message);
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
