void print_config() {
  String strpcheck =  ((enable_paritycheck == true) ? F("Parity Check           ON")     : F("Parity Check           OFF"));
  String strecc =                                     F("ECC-Mode               OFF");
  if (ecc_mode == 1) strecc =                         F("ECC-Mode               1 bit");
  if (ecc_mode == 2) strecc =                         F("ECC-Mode               2 bit");
  if (ecc_mode == 3) strecc =                         F("ECC-Mode               >2 bit");
  String strled =  ((enable_led == true) ?            F("LED                    ON")     : F("LED                    OFF"));
  String strfsa =  ((enable_fsa == true) ?       String("Field Strength Alarm   ON (" + String(fsa_timeout_minutes) + " min.)")     : F("Field Strength Alarm   OFF"));
  String strdebug =                                   F("Debug Level            OFF (0)");
  if (debugLevel == 1) strdebug =                     F("Debug Level            CW 0+1 ");
  if (debugLevel == 2) strdebug =                     F("Debug Level            ALL (2)");
  String strinvert =  ((invert_signal == FALLING) ?   F("Input Level            NORMAL") : F("Input Level            INV."));

  Serial.println(String(F("******** current config ********\r\n"))+strpcheck + F("\r\n") + strdebug + F("\r\n") + strecc + F("\r\n") + strled + F("\r\n") + strinvert + F("\r\n") + strfsa);
}

void process_serial_input() {
  if (serialbuffer_counter > 0) {
    Serial.println();
    if (strstr(serialbuffer, "time")) {
      if (serialbuffer_counter > 23) {
        tag = getIntFromString (serialbuffer, 1);
        monat = getIntFromString (serialbuffer, 2);
        jahr = getIntFromString (serialbuffer, 3);
        stunde = getIntFromString (serialbuffer, 4);
        minute = getIntFromString (serialbuffer, 5);
        sekunde = getIntFromString (serialbuffer, 6);

        if (!checkDateTime(jahr, monat, tag, stunde, minute, sekunde)) {
          Serial.println(serialbuffer);
          Serial.println(F("Beispiel: time 08.11.2017 10:54:00"));
          return;
        }
        rtcWriteTime(jahr, monat, tag, stunde, minute, sekunde);
        Serial.println(F("Zeit und Datum wurden auf neue Werte gesetzt."));
      }
      Serial.println(strRTCDateTime());
      return;
    }
    if (strstr(serialbuffer, "h") || strstr(serialbuffer, "?")) 
      Serial.println(F("******** help config ********\r\np0/1    = Parity Check dis-/enabled\r\nd0/1/2  = Debug Level\r\ne0/1    = ECC dis-/enabled\r\nl0/1    = LEDs dis-/enabled\r\ni0/1    = Input normal/inverted\r\nftnnn   = Field Strength Alarm (nnn minutes; 0 = off)\r\ntime    = time dd.mm.yyyy hh:mm:ss"));
    if (strstr(serialbuffer, "p0")) {
      EEPROM.write(0, false);
      enable_paritycheck = false;
    }
    if (strstr(serialbuffer, "p1")) {
      EEPROM.write(0, true);
      enable_paritycheck = true;
    }
    if (strstr(serialbuffer, "d0")) {
      EEPROM.write(1, 0);
      debugLevel = 0;
    }
    if (strstr(serialbuffer, "d1")) {
      EEPROM.write(1, 1);
      debugLevel = 1;
    }
    if (strstr(serialbuffer, "d2")) {
      EEPROM.write(1, 2);
      debugLevel = 2;
    }
    if (strstr(serialbuffer, "e0")) {
      EEPROM.write(2, 0);
      ecc_mode = 0;
    }
    if (strstr(serialbuffer, "e1")) {
      EEPROM.write(2, 1);
      ecc_mode = 1;
      setupecc();
    }
    if (strstr(serialbuffer, "e2")) {
      EEPROM.write(2, 2);
      ecc_mode = 2;
      setupecc();
    }
    if (strstr(serialbuffer, "e3")) {
      EEPROM.write(2, 3);
      ecc_mode = 3;
    }
    if (strstr(serialbuffer, "l0")) {
      EEPROM.write(3, false);
      enable_led = false;
    }
    if (strstr(serialbuffer, "l1")) {
      EEPROM.write(3, true);
      enable_led = true;
    }
    if (strstr(serialbuffer, "i0")) {
      EEPROM.write(4, FALLING);
      invert_signal = FALLING;
    }
    if (strstr(serialbuffer, "i1")) {
      EEPROM.write(4, RISING);
      invert_signal = RISING;
    }
    if (strstr(serialbuffer, "ft")) {
      fsa_timeout_minutes = getIntFromString(serialbuffer, 1);
      EEPROM.write(5, fsa_timeout_minutes);
    }
    eeprom_read();
    print_config();
  }
}

void eeprom_read() {
  enable_paritycheck = EEPROM.read(0);
  debugLevel = EEPROM.read(1);
  ecc_mode = EEPROM.read(2);
  enable_led = EEPROM.read(3);
  invert_signal = EEPROM.read(4);
  fsa_timeout_minutes = EEPROM.read(5);
  enable_fsa = (fsa_timeout_minutes != 0);
}
