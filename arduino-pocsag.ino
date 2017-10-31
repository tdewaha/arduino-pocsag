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
#include <EEPROM.h>

#define receiverPin 	  3 //PORTD
#define triggerPin 	    6 //PORTD
#define ledPin          7 //PORTD
#define bitPeriod 833
#define halfBitPeriod 416
#define prmbWord 1431655765
#define syncWord 2094142936
#define idleWord 2055848343

#define STATE_WAIT_FOR_PRMB 	  0
#define STATE_WAIT_FOR_SYNC 	  1
#define STATE_PROCESS_BATCH 	  2
#define STATE_PROCESS_MESSAGE 	3

#define MSGLENGTH 	        240
#define BITCOUNTERLENGTH	  544
#define MAXNUMBATCHES		     15    //5
static const char *functions[4] = {"A", "B", "C", "D"};

volatile unsigned long buffer = 0;
volatile int bitcounter = 0;
int state = 0;
int wordcounter = 0;
int framecounter = 0;
int batchcounter = 0;
bool enableParityCheck = false;
byte debugLevel = 0;
unsigned long wordbuffer[(MAXNUMBATCHES * 16) + 1];

void setup()
{
  pinMode(receiverPin, INPUT_PULLUP);
  pinMode(triggerPin, OUTPUT);
  pinMode(ledPin, OUTPUT);
  Serial.begin(57600);
  enableParityCheck = EEPROM.read(0);
  debugLevel = EEPROM.read(1);
  Serial.println(F("START"));
  print_config();
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

      if (batchcounter >= MAXNUMBATCHES) {
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

  if (Serial.available()) {
    String serread = Serial.readStringUntil('\n');
    process_serial_input(serread);
  }
}

void decode_wordbuffer() {
  int address_counter = 0;
  unsigned long address[16];
  memset(address, 0, sizeof(address));
  byte function[16];
  memset(function, 0, sizeof(function));
  char message[MSGLENGTH];
  memset(message, 0, sizeof(message));

  byte character = 0;
  int bcounter = 0;
  int ccounter = 0;
  boolean eot = false;

  for (int i = 0; i < ((MAXNUMBATCHES * 16) + 1); i++) {
    if (debugLevel > 0) {
      String t = String(wordbuffer[i]);
      if (wordbuffer[i] == prmbWord) t = F("prmbWord");
      if (wordbuffer[i] == idleWord) t = F("idleWord");
      if (wordbuffer[i] == syncWord) t = F("syncWord");
      if (debugLevel == 2 || (debugLevel == 1 && i < 2))
        Serial.println(String(F("CW[")) + String(i) + F("] ") + t);
    }

    if (wordbuffer[i] == 0) continue;
    if (wordbuffer[i] == idleWord) continue;

    if (enableParityCheck) {
      if (parity(wordbuffer[i]) == 1) {
        if (debugLevel == 2) Serial.println (String(F("PE[")) + String(i) + F("]"));
        continue;
      }
    }

    if (bitRead(wordbuffer[i], 31) == 0) {
      if  ((i > 0 && wordbuffer[i - 1] == idleWord || address_counter == 0) && (parity(wordbuffer[i]) != 1)) {
        address[address_counter] = extract_address(i);
        if (debugLevel == 2) Serial.println(String(F("Adresse ")) + String(address[address_counter]) + F(" gefunden an Stelle ") + String(i) + F(", address_counter = ") + String(address_counter));
        function[address_counter] = extract_function(i);
        if (address_counter > 0) print_message(String(address[address_counter - 1]), function[address_counter - 1], message);
        eot = false;
        ccounter = 0;
        bcounter = 0;
        address_counter++;
      }
    } else {
      if (address[address_counter - 1] != 0 && ccounter < MSGLENGTH) {
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
    if (debugLevel == 2) Serial.println(String(F("address_counter = ")) + String(address_counter));
  }
}
