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
