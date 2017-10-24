/**
    Decodes POCSAG data from the FSK demodulator output on the RF board of Motorola pagers.
    Tested successfully with up to 1200 baud POCSAG

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
#include <TimerOne.h>
#define receiverPin 	3
#define triggerPin 	6
#define ledPin 		7
#define bitPeriod 833
#define halfBitPeriod 416
#define prmbWord 1431655765
#define syncWord 2094142936
#define idleWord 2055848343

#define STATE_WAIT_FOR_PRMB 	0
#define STATE_WAIT_FOR_SYNC 	1
#define STATE_PROCESS_BATCH 	2
#define STATE_PROCESS_MESSAGE 	3

#define MSGLENGTH 		    240
#define BITCOUNTERLENGTH	544
static const char *functions[4] = {"A", "B", "C", "D"};

volatile unsigned long buffer = 0;
volatile int bitcounter = 0;
int state = 0;
int wordcounter = 0;
int framecounter = 0;
int batchcounter = 0;

unsigned long wordbuffer[81];

void setup()
{
  pinMode(receiverPin, INPUT_PULLUP);
  pinMode(triggerPin, OUTPUT);
  pinMode(ledPin, OUTPUT);
  Serial.begin(115200);
  Serial.println("START (MSGLENGTH = " + String(MSGLENGTH) + ", BITCOUNTERLENGTH = " + String(BITCOUNTERLENGTH) + ")");
  disable_trigger();
  disable_led();
  start_flank();
  Timer1.initialize(bitPeriod);
}

void loop() {
  switch (state) {
    case STATE_WAIT_FOR_PRMB:
      if (buffer == prmbWord) {
        state = STATE_WAIT_FOR_SYNC;
        enable_trigger();
      }
      break;

    case STATE_WAIT_FOR_SYNC:
      if (buffer == syncWord) {
        enable_led();
        bitcounter = 0;
        state = STATE_PROCESS_BATCH;
      } else {
        if (bitcounter > BITCOUNTERLENGTH) {
          bitcounter = 0;
          if (batchcounter > 0) {
            if (state != STATE_PROCESS_MESSAGE) {
              disable_led();
              disable_trigger();
            }
            state = STATE_PROCESS_MESSAGE;
          } else {
            state = STATE_WAIT_FOR_PRMB;
            disable_trigger();
            disable_led();
          }
          batchcounter = 0;
        }
      }
      break;

    case STATE_PROCESS_BATCH:
      if (bitcounter >= 32) {
        bitcounter = 0;
        wordbuffer[(batchcounter * 16) + (framecounter * 2) + wordcounter] = buffer;
        wordcounter++;
	String t = String(buffer);
 	if (buffer == idleWord) t = "idleWord";
	if (buffer == prmbWord) t = "prmbWord";
	if (buffer == syncWord) t = "syncWord";
	Serial.println("STATE_PROCESS_BATCH: wordbuffer: "+String((batchcounter * 16) + (framecounter * 2) + wordcounter) +" = "+t);
      }

      if (wordcounter >= 2) {
        wordcounter = 0;
        framecounter++;
      }

      if (framecounter >= 8) {
        framecounter = 0;
        batchcounter++;
        state = STATE_WAIT_FOR_SYNC;
      }

      if (batchcounter >= 5) {
        batchcounter = 0;
        state = STATE_PROCESS_MESSAGE;
      }
      break;

    case STATE_PROCESS_MESSAGE:
      stop_flank();
      stop_timer();

      decode_wordbuffer();

      memset(wordbuffer, 0, sizeof(wordbuffer));
      state = STATE_WAIT_FOR_PRMB;
      disable_trigger();
      disable_led();
      start_flank();
      break;
  }
}

void decode_wordbuffer() {
  unsigned long address = 0;
  byte function = 0;
  char message[MSGLENGTH];
  memset(message, 0, sizeof(message));
  byte character = 0;
  int bcounter = 0;
  int ccounter = 0;
  boolean eot = false;

  for (int i = 0; i < 81; i++) {
    if (parity(wordbuffer[i]) == 1) continue;                      // Invalid Codeword
    if (wordbuffer[i] == idleWord) continue;                       // IDLE
    if (wordbuffer[i] == 0) continue;                              // Empty Codeword

    if (bitRead(wordbuffer[i], 31) == 0) {                         // Found an Address 
      if (address == 0) {
        address = extract_address(i);
        function = extract_function(i);
        eot = false;
      } else {
    	 print_message(address, function, message);                                                                                                                                                                                                                             
	 for (int j = 0; j < MSGLENGTH; j++) { message[j] = 32;}
	 address = 0;
      }
    } else {
      if (address != 0 && ccounter < MSGLENGTH) {
        for (int c = 30; c > 10; c--) {
          bitWrite(character, bcounter, bitRead(wordbuffer[i], c));
          bcounter++;
          if (bcounter >= 7) {
            if (character == 4) {
              eot = true;
            }
            bcounter = 0;
            if (eot == false) {
              message[ccounter] = checkUmlaut(character);
              ccounter++;
            }
          }
        }
      }
    }
  }
  if (address != 0) {
    print_message(address, function, message);
  }
}

