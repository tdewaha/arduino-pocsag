void rtcReadTime() {
  Wire.beginTransmission(RTC_I2C_ADDRESS);
  Wire.write(0);
  Wire.endTransmission();
  Wire.requestFrom(RTC_I2C_ADDRESS, 7);
  sekunde    = bcdToDec(Wire.read() & 0x7f);
  minute     = bcdToDec(Wire.read());
  stunde     = bcdToDec(Wire.read() & 0x3f);
  bcdToDec(Wire.read());
  tag        = bcdToDec(Wire.read());
  monat      = bcdToDec(Wire.read());
  jahr       = bcdToDec(Wire.read()) + 2000;
}

void rtcWriteTime(int jahr, int monat, int tag, int stunde, int minute, int sekunde) {
  Wire.beginTransmission(RTC_I2C_ADDRESS);
  Wire.write(0);
  Wire.write(decToBcd(sekunde));
  Wire.write(decToBcd(minute));
  Wire.write(decToBcd(stunde));
  Wire.write(decToBcd(0));
  Wire.write(decToBcd(tag));
  Wire.write(decToBcd(monat));
  Wire.write(decToBcd(jahr - 2000));
  Wire.endTransmission();
}

byte decToBcd(byte val) {
  return ( (val / 10 * 16) + (val % 10) );
}

byte bcdToDec(byte val) {
  return ( (val / 16 * 10) + (val % 16) );
}

int getIntFromString (char *stringWithInt, byte num) {
  char *tail;
  while (num > 0) {
    num--;
    while ((!isdigit (*stringWithInt)) && (*stringWithInt != 0)) {
      stringWithInt++;
    }
    tail = stringWithInt;
    while ((isdigit(*tail)) && (*tail != 0)) {
      tail++;
    }

    if (num > 0) {
      stringWithInt = tail;
    }
  }
  return (strtol(stringWithInt, &tail, 10));
}

String strRTCDateTime() {
  if (enable_rtc) {
    rtcReadTime();
    String result = "";
    if (tag < 10) {
      result += "0";
    }
    result += tag;
    result += ".";
    if (monat < 10) {
      result += "0";
    }
    result += monat;
    result += ".";
    result += jahr;
    result += " ";
    if (stunde < 10) {
      result += "0";
    }
    result += stunde;
    result += ":";
    if (minute < 10) {
      result += "0";
    }
    result += minute;
    result += ":";
    if (sekunde < 10) {
      result += "0";
    }
    result += sekunde;
    return result;
  } else return "00.00.0000 00:00:00";
}

boolean checkDateTime(int jahr, int monat, int tag, int stunde, int minute, int sekunde) {
  boolean result = false;
  if (jahr > 2000) {
    result = true;
  } else {
    return false;
  }

  if (jahr % 400 == 0 || (jahr % 100 != 0 && jahr % 4 == 0)) {
    daysInMonth[1] = 29;
  }

  if (monat < 13) {
    if ( tag <= daysInMonth[monat - 1] ) {
      result = true;
    }
  } else {
    return false;
  }

  if (stunde < 24 && minute < 60 && sekunde < 60 && stunde >= 0 && minute >= 0 && sekunde >= 0) {
    result = true;
  } else {
    return false;
  }

  return result;
}

