#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

void setup() {
  u8g2.begin();
}

void loop() {
  const char* line1 = "THE WIRED WORLD";
  const char* line2 = "OF QASIM";

  u8g2.setFont(u8g2_font_ncenB08_tr); // Medium font for 1.3" OLED

  // Fade-in
  for (int i = 0; i < 3; i++) {
    u8g2.clearBuffer();
    u8g2.sendBuffer();
    delay(100);
  }

  // Typing animation
  String current1 = "";
  String current2 = "";

  // --- Animate first line ---
  for (int i = 0; i < strlen(line1); i++) {
    current1 += line1[i];
    u8g2.clearBuffer();
    int x1 = (128 - u8g2.getStrWidth(current1.c_str())) / 2; // center align
    u8g2.drawStr(x1, 28, current1.c_str());
    u8g2.sendBuffer();
    delay(100);
  }

  delay(400);

  // --- Animate second line ---
  for (int i = 0; i < strlen(line2); i++) {
    current2 += line2[i];
    u8g2.clearBuffer();
    int x1 = (128 - u8g2.getStrWidth(line1)) / 2;       // first line center
    int x2 = (128 - u8g2.getStrWidth(current2.c_str())) / 2; // second line center
    u8g2.drawStr(x1, 28, line1);
    u8g2.drawStr(x2, 50, current2.c_str());
    u8g2.sendBuffer();
    delay(120);
  }

  delay(500);

  // // --- Scroll-out left animation ---
  // for (int shift = 0; shift < 130; shift++) {
  //   u8g2.clearBuffer();
  //   int x1 = ((128 - u8g2.getStrWidth(line1)) / 2) - shift;
  //   int x2 = ((128 - u8g2.getStrWidth(line2)) / 2) - shift;
  //   u8g2.drawStr(x1, 28, line1);
  //   u8g2.drawStr(x2, 50, line2);
  //   u8g2.sendBuffer();
  //   delay(20);
  // }

  delay(1000);
}
