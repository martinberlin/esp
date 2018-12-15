/*

  GraphicsTest.ino

  Universal 8bit Graphics Library (https://github.com/olikraus/u8g2/)

*/
#include <U8g2lib.h>
#include <Wire.h>

U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, /* clock=*/ 15, /* data=*/ 4, /* reset=*/ 16);

long total;
long processed;
void setup(void) {
  Serial.begin(115200);
  u8g2.begin();
  u8g2.setDrawColor(1);
  u8g2.setFont(u8g2_font_pcsenior_8r);
  //u8g2.clearBuffer();
  total = 100000;
  processed = 0;
}

void progressBar(long processed, long total, char *message) {
 int width = round( processed*128 / total );
 u8g2.drawBox(127, 1,  1, 4);  // end of upload
 u8g2.drawBox(0, 1, width, 4);
 u8g2.drawStr(0, 18, message);
 u8g2.sendBuffer();
 Serial.println(width);
}


void loop(void) {

  
  if (processed%10==0) {
    progressBar(processed, total, "Uploading");
  }
  if (processed>=total) processed=0;
  delay(100);
  processed = processed+1000;
}


