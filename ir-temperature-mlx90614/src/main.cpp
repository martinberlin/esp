#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MLX90614.h>
/* Note this is a simple demo for an IR temperature sensor MLX90614 from firma Melexis (There may be clones too)
   As a big difference with SPI, on I2C you need to know the address of the peripheral you want to address, 
   so it's not 0x5A 
   just turn scanMode to true and find out what is the address.
   Note that it needs two pull-up 10K resistances for both SDA and SDL (clock) pins otherwise won't work.
   For more info check: https://learn.adafruit.com/using-melexis-mlx90614-non-contact-sensors/wiring-and-test

   ESP32 board used:    https://heltec.org/project/wifi-kit-32
*/
#include <U8g2lib.h>

Adafruit_MLX90614 mlx = Adafruit_MLX90614();

// OLED display I2C Settings are for Heltec board, change it to suit yours in case to use another board or just output via Serial
U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R2, /* clock=*/ 15, /* data=*/ 4, /* reset=*/ 16);

bool print_ambient = false; // Prints also ambient temperature in the bottom
bool scanMode = false;      // Only scans I2C, does not print sensor in Oled


void setMainFont(){
  // Big font for the main temperature
  u8g2.setFont(u8g2_font_7Segments_26x42_mn);
}

void setup() {
  Serial.begin(115200);

  // Use pullups of 10K to 3.3 on both I2C pins: 21 & 22
  // Using internal pullups didn't worked out for me. And also CLK is both directions in I2C protocol, cannot be declared as output
  //pinMode(22, INPUT_PULLUP); 
  //pinMode(21, INPUT_PULLUP);
  if (scanMode) {
    Wire.begin(21,22,50000); // 21 SDA  22 SDL
  }
  u8g2.begin();
  
  setMainFont();

  mlx.begin();
}

void Scanner(){
  Serial.println ("I2C scanner. Scanning ...");
  uint8_t error, address;
  int nDevices = 0;
  for(address = 1; address < 127; address++ ) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0) {
      Serial.print("I2C device found at address 0x");
      if (address<16) {
        Serial.print("0");
      }
      Serial.println(address,HEX);
      nDevices++;
    }    
  }
  if (nDevices == 0) {
    Serial.println("No I2C devices found\n");
  }
  else {
    Serial.println("done\n");
  }

  delay(2000);          
}

void loop() {

  if (!scanMode){  
    u8g2.setCursor(1, 42);
    u8g2.print( mlx.readObjectTempC() );

    if (print_ambient) {
      u8g2.setCursor(1, 60); 
      u8g2.setFont(u8g2_font_amstrad_cpc_extended_8u);
      u8g2.print((String)mlx.readAmbientTempC()+" CELSIUS");    
      // Sorry there is no dot  (.) in this font, change to another in setMainFont() if you wish 
      setMainFont();
    } 
    
    u8g2.sendBuffer();
    delay(300);
  } else {
    Scanner();
  }
  
}
