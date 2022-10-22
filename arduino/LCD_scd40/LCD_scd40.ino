#include <OneBitDisplay.h>
#include <Wire.h>

#include "SparkFun_SCD4x_Arduino_Library.h" //Click here to get the library: http://librarymanager/All#SparkFun_SCD4x
SCD4x mySensor;
// CASE Dimensions:  43.6 x 34 mm. LCD library
OBDISP obd;
#define FLIP180 0
#define INVERT 0
#define BITBANG 1
#define DC_PIN 4
#define CS_PIN 3
#define RESET_PIN 8
#define MOSI_PIN 5
// Was 2 before in Nano
#define CLK_PIN 6
#define VSS_PIN 12
#define LED_PIN 11


// PWM to make the ON/OFF softer for backlight
void turnOnLed() {
  for (uint8_t pwm = 20; pwm<200; pwm+=10) {
    analogWrite(LED_PIN, pwm);
    delay(20);
  }
}
void turnOffLed() {
  for (uint8_t pwm = 200; pwm>0; pwm-=10) {
    analogWrite(LED_PIN, pwm);
    delay(30);
  }
  digitalWrite(LED_PIN, 0);
}

void setup() {
  Serial.begin(9600);
  pinMode(LED_PIN, OUTPUT);

  // Note: If I don't pass here VSS (supposedly GND?) as iLed I get the characters mirrored (??)
  //void obdSPIInit(OBDISP *pOBD, int iType, int iDC, int iCS, int iReset, int iMOSI, int iCLK, int iLED, int bFlip, int bInvert, int bBitBang, int32_t iSpeed)
  obdSPIInit(&obd, LCD_UC1701, DC_PIN, CS_PIN, RESET_PIN, MOSI_PIN, CLK_PIN, VSS_PIN, FLIP180, 0, BITBANG, 0);
  obdFill(&obd, 0, 1);
    //           iScrollX, int x, int y
  obdWriteString(&obd, 0, 0, 0, (char *)"FASANI CORP", FONT_8x8, 0, 1);
    //           iScrollX, int x, int y
  delay(150);
  obdWriteString(&obd, 1, 0, 40, (char *)"Initializing...", FONT_8x8, 0, 1);
  // Read SCD40 values, etc...
  turnOnLed();
  Wire.begin();
    if (mySensor.begin() == false)
  {
    obdWriteString(&obd, 1, 10, 0, (char *)"Sensor not found", FONT_8x8, 0, 1);
    while (1);
  }

  

    //But first, we need to stop periodic measurements otherwise startLowPowerPeriodicMeasurement will fail
  if (mySensor.stopPeriodicMeasurement() == true)
  {
    Serial.println(F("Periodic measurement is disabled!"));
  }  

  //Now we can enable low power periodic measurements
  if (mySensor.startLowPowerPeriodicMeasurement() == true)
  {
    Serial.println(F("Low power mode enabled!"));
  }

  Serial.println("OneBitDisplay init done");
}

uint8_t progressbar = 0;
uint8_t bar_y = 63;

void loop() {
    if (mySensor.readMeasurement()) // readMeasurement will return true when fresh data is available
  {
    progressbar = 0;
    obdFill(&obd, 0, 1);
      //           iScrollX, int x, int y
    obdWriteString(&obd, 0, 48, 0, (char *)"FASANICORP", FONT_8x8, 0, 1);
    char co2[6];
    char temp[30];
    char hum[16];
    itoa(mySensor.getCO2(), co2, 10);

    itoa(mySensor.getHumidity(), hum, 10);
    
    obdWriteString(&obd, 0, 0, 14, co2, FONT_12x16, 0, 1);
    obdWriteString(&obd, 0, 48, 22, (char*) "CO2 (ppm)", FONT_8x8, 0, 1);

    //4 is mininum width, 3 is precision; float value is copied onto buff
    dtostrf(mySensor.getTemperature(), 2, 1, temp);
    strcat(temp, (char*)"  Celsius");
    obdWriteString(&obd, 0, 0, 38, temp, FONT_8x8, 0, 1);

    strcat(hum, (char*)"%   Humedad");
    obdWriteString(&obd, 0, 0, 50, hum, FONT_8x8, 0, 1);
  }
  
  progressbar++;
  obdDrawLine(&obd, 0, bar_y, progressbar, bar_y, 1, 1);
  obdDrawLine(&obd, 0, bar_y-1, progressbar, bar_y-1, 0, 1);
  obdDrawLine(&obd, 0, bar_y-2, progressbar, bar_y-2, 0, 1);
  obdDrawLine(&obd, 0, bar_y-3, progressbar, bar_y-3, 0, 1);
  obdDrawLine(&obd, 0, bar_y-4, progressbar, bar_y-4, 0, 1);
  obdDrawLine(&obd, 0, bar_y-5, progressbar, bar_y-5, 0, 1);
  delay(200);

}
