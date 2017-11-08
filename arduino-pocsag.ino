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
#include <Wire.h>
#define RTC_I2C_ADDRESS 0x68
#define receiverPin                   	19
#define pmbledPin         	             8
#define syncledPin                       9
#define bitPeriod                      833
#define halfBitPeriod                  416
#define halfBitPeriodTolerance          30
#define prmbWord                1431655765
#define syncWord                2094142936
#define idleWord                2055848343

#define STATE_WAIT_FOR_PRMB 	  0
#define STATE_WAIT_FOR_SYNC 	  1
#define STATE_PROCESS_BATCH 	  2
#define STATE_PROCESS_MESSAGE 	3

#define MSGLENGTH 	        240
#define BITCOUNTERLENGTH	  540
#define MAXNUMBATCHES		     14
static const char *functions[4] = {"A", "B", "C", "D"};

volatile unsigned long buffer = 0;
volatile int bitcounter = 0;
int state = 0;
int wordcounter = 0;
int framecounter = 0;
int batchcounter = 0;
byte debugLevel = 0;
bool enable_paritycheck = false;
byte ecc_mode = 0;
bool enable_led = false;
byte invert_signal = RISING;
unsigned long wordbuffer[(MAXNUMBATCHES * 16) + 1];
byte ob[32];
unsigned int bch[1025], ecs[25];
byte decode_errorcount = 0;
byte decode_errorcount_max = 3;

//RTC Variablen
int jahr, monat, tag, stunde, minute, sekunde, wochentag;
int daysInMonth[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

char serbuf[30];
byte serbufcounter = 0;

void setup()
{
  Wire.begin();
  pinMode(receiverPin, INPUT);
  pinMode(pmbledPin, OUTPUT);
  pinMode(syncledPin, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  Serial.begin(115200);
  Serial.println();

  Serial.println(F("START POCSAG DECODER"));

  enable_paritycheck = EEPROM.read(0);
  debugLevel = EEPROM.read(1);
  ecc_mode = EEPROM.read(2);
  enable_led = EEPROM.read(3);
  invert_signal = EEPROM.read(4);
  if (ecc_mode > 0) setupecc();
  print_config();
  disable_syncled();
  disable_pmbled();
  start_flank();
  Timer1.initialize(bitPeriod);
  Serial.println(strRTCDateTime());
}

void loop() {
  switch (state) {
    case STATE_WAIT_FOR_PRMB:
      if (buffer == prmbWord) {
        state = STATE_WAIT_FOR_SYNC;
        if (enable_led) enable_pmbled();
      }
      break;

    case STATE_WAIT_FOR_SYNC:
      if (buffer == syncWord) {
        if (enable_led) enable_syncled();
        bitcounter = 0;
        state = STATE_PROCESS_BATCH;
      } else {
        if (bitcounter > BITCOUNTERLENGTH) {
          bitcounter = 0;
          if (batchcounter > 0) {
            if (state != STATE_PROCESS_MESSAGE) {
              if (enable_led) disable_syncled();
              if (enable_led) disable_pmbled();
            }
            state = STATE_PROCESS_MESSAGE;
          } else {
            state = STATE_WAIT_FOR_PRMB;
            if (enable_led) disable_syncled();
            if (enable_led) disable_pmbled();
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
      if (enable_led) disable_syncled();
      if (enable_led) disable_pmbled();
      start_flank();
      break;
  }

  if (Serial.available()) {
    while (Serial.available()) {
      serbuf[serbufcounter] = Serial.read();
      Serial.print(serbuf[serbufcounter]);
      if (serbuf[serbufcounter] == '\r') {
        process_serial_input();
        memset(serbuf, 0, sizeof(serbuf));
        serbufcounter = 0;
      } else if (serbufcounter < sizeof(serbuf) - 1) serbufcounter++;
    }
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
    if (wordbuffer[i] == 0) continue;
    if (debugLevel > 0) {
      if (debugLevel == 2 || (debugLevel == 1 && i < 2))
        Serial.print(String(F("\r\nwordbuffer[")) + String(i) + F("] = ") + String(wordbuffer[i]) + F(";"));
    }

    if (wordbuffer[i] == idleWord) continue;

    if (enable_paritycheck) {
      if (parity(wordbuffer[i]) == 1) {
        if (debugLevel == 2) Serial.println(String(F("wordbuffer[")) + String(i) + F("] = ") + String(wordbuffer[i]) + F("; PE"));
        continue;
      }
    }

    if (ecc_mode > 0) {
      unsigned long preEccWb = wordbuffer[i];
      for (int obcnt = 0; obcnt < 32; obcnt++)
        ob[obcnt] = bitRead(wordbuffer[i], 31 - obcnt);

      int ecdcount = ecd();

      if (ecdcount == 3) decode_errorcount++;

      if (decode_errorcount >= decode_errorcount_max) {
        Serial.print("\r\ndecode_wordbuffer process cancelled! too much errors. errorcount > " + String(decode_errorcount_max));
        break;
      }

      if (ecdcount < ecc_mode + 1)
        for (int obcnt = 0; obcnt < 32; obcnt++)
          bitWrite(wordbuffer[i], 31 - obcnt, (int)ob[obcnt]);

      if (debugLevel > 1) {
        Serial.print(" // (" + String(ecdcount) + ") ");
        if (preEccWb != wordbuffer[i])
          Serial.print("*");
      }
    }

    if (bitRead(wordbuffer[i], 31) == 0) {
      if  ((i > 0 && wordbuffer[i - 1] == idleWord || address_counter == 0) && (parity(wordbuffer[i]) != 1)) {
        address[address_counter] = extract_address(i);
        if (debugLevel == 2) Serial.print(" //" + String(F("Adresse ")) + String(address[address_counter]) + F(" gefunden, address_counter = ") + String(address_counter));
        function[address_counter] = extract_function(i);
        if (address_counter > 0) {
          print_message(address[address_counter - 1], function[address_counter - 1], message);
        }
        eot = false;
        memset(message, 0, sizeof(message));
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

  decode_errorcount = 0;

  if (address_counter > 0) {
    print_message(address[address_counter - 1], function[address_counter - 1], message);
    if (debugLevel == 2)  Serial.print(String(F("\r\naddress_counter = ")) + String(address_counter));
  }
  Serial.println("\r\n======== [" + strRTCDateTime() + "] ========");
}
