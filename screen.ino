void initScreen() {
  tft.initR(INITR_GREENTAB);
  tft.fillScreen(ST7735_BLACK);
  tft.setRotation(3);
  tft.setCursor(0, 0);
  tft.setTextColor(ST7735_WHITE);
  tft.setTextWrap(true);
  tft.setTextSize(1);
  tft.print("Hello!");
}

char checkUmlaut(byte ascii) {
  switch (ascii) {
    case '{': return  0x84; break; // ä
    case '|': return  0x94; break; // ö
    case '}': return  0x81; break; // ü
    //case '}': return  0x8E; break; // Ä
    //case '}': return  0x99; break; // Ü
    //case '}': return  0x9A; break; // Ö
    case '~' : return  0xE0; break; // ß
    default:  return  ascii; break;
  }
}
String convert_tft_message(char message[MSGLENGTH], int msglen) {
  String msg = "";
  for (int i=0; i < msglen; i++) {
    msg += checkUmlaut(message[i]);  
  }
  return msg;
}

void tft_message(unsigned long address, byte function, char message[MSGLENGTH], int msglen)
{
  tft.fillScreen(ST7735_BLACK);
  tft.setCursor(0, 0);
  tft.setTextColor(ST7735_WHITE);
  tft.setTextWrap(true);
  tft.setTextSize(1);
  String text = "";
  if (function == 0) tft.setTextColor(ST7735_RED);
  if (function == 1) tft.setTextColor(ST7735_BLUE);
  if (function == 2) tft.setTextColor(ST7735_GREEN);
  if (function == 3) tft.setTextColor(ST7735_YELLOW);
  text += functions[function];
  tft.print(text + " ");
  text = "";
  tft.setTextColor(ST7735_WHITE);
  if (address < 1000000) text += "0";
  text += address;
  text += "\n";
  tft.print(text);
  text = "";
  tft.println("");
  tft.setTextSize(1);
  text += convert_tft_message(message,msglen);
  tft.print(text);

}

