/*
   ESP8266 LLMNR responder sample
   Copyright (C) 2017 Stephen Warren <swarren@wwwdotorg.org>

   Based on:
   ESP8266 Multicast DNS (port of CC3000 Multicast DNS library)
   Version 1.1
   Copyright (c) 2013 Tony DiCola (tony@tonydicola.com)
   ESP8266 port (c) 2015 Ivan Grokhotkov (ivan@esp8266.com)
   MDNS-SD Suport 2015 Hristo Gochkov
   Extended MDNS-SD support 2016 Lars Englund (lars.englund@gmail.com)
   Instructions:
   - Update WiFi SSID and password as necessary.
   - Flash the sketch to the ESP8266 board.
   - Windows:
     - No additional software is necessary.
     - Point your browser to http://esp8266/, you should see a response. In most
       cases, it is important that you manually type the "http://" to force the
       browser to search for a hostname to connect to, rather than perform a web
       search.
     - Alternatively, run the following command from the command prompt:
       ping esp8266
   - Linux:
     - To validate LLMNR, install the systemd-resolve utility.
     - Execute the following command:
       systemd-resolve -4 -p llmnr esp8266
     - It may be possible to configure your system to use LLMNR for all name
       lookups. However, that is beyond the scope of this description.

*/

#include <ESP8266WiFi.h>
#include <ESP8266LLMNR.h>
#include <ESP8266WebServer.h>
#include <WiFiClient.h>
//#include "DHT.h" //DHT Bibliothek laden

//LCD try
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Actual device, the IC2 address may change.
LiquidCrystal_I2C lcd(0x27, 16, 2);

#define LED_PIN D3
#define RELAY_PIN D4

//#define DHTPIN D5 //Der Sensor wird an PIN 2 angeschlossen
//#define DHTTYPE DHT11    // Es handelt sich um den DHT11 Sensor
//DHT dht(DHTPIN, DHTTYPE);

// WIFI Access Home / AndroidAP define this in array to allow trying different Wlan networks
char* ssids[] = {"KabelBox-A210", "AndroidAP"};
char* passwords[] = {"14237187131701431551", "fasfasnar"};
// TODO Check how to get length of array in C++
byte wifiIndex = 0;
boolean wlGiveUp = false;
// ORDER of array should match SSID[0]->Password[0] // NEL 3162681x

float temperature, humidity;
unsigned long previousMillis = 0; // When was the sensor last read
const long interval = 10000;      // Wait N seconds before reading again
const int deepSleepAfter = 6;     // 6 will go to sleep in a minute
int countTenSec = 0;              // Counter to go to sleep after N*10 seconds
int ledOnPwm = 100; // LCD backlight led
int ledOnPwmHigh = 800;
int actualLedPwm = ledOnPwm;
char* displayMode = "weather"; // Remember Strings go always with "" not with ''

ESP8266WebServer server(80);

void setup(void) {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  analogWrite(LED_PIN, 500);
  digitalWrite(RELAY_PIN, HIGH);

  lcd.init();
  lcd.backlight();
  lcd.print("MINI-DISPLAY NEL");

  // Connect to WiFi network
  WiFi.mode(WIFI_STA);

  int countTryConnection = 30;      // loop N times per Wlan before giving up

  while (WiFi.status() != WL_CONNECTED) {
    char* ssid = ssids[wifiIndex];
    char* password = passwords[wifiIndex];
    Serial.print("Try connecting to:" + String(ssid)); Serial.println();

    if (countTryConnection % 10 == 0) {
      WiFi.begin(ssid, password);
      lcd.setCursor(0, 1);
      lcd.print("Try:" + String(ssid) + " ");

      wifiIndex++;
      if (wifiIndex > 1) wifiIndex = 0;
    }
    if (countTryConnection == 0) {
      lcd.clear();
      lcd.print("Could not connect");
      delay(1000);
      handle_sleep();
    }
    // Wait for connection
    delay(1000);
    lcd.print(".");
    Serial.println(countTryConnection);

    countTryConnection--;
  }
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  // Show IP in display
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("Connected");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP());

  // Start LLMNR responder
  LLMNR.begin("esp8266");
  // Start HTTP server
  server.onNotFound(handle_http_not_found);
  server.on("/", handle_http_root);
  server.on("/4/1", handle4low);
  server.on("/4/2", handle4high);
  server.on("/4/0", handle4off);
  server.on("/lcd-write", handleLcdWrite);

  server.on("/relay/on", handleRelayOn);
  server.on("/relay/off", handleRelayOff);

  server.on("/mode/sleep", []() {
    server.send(200, "text/plain", "CPU deep sleep enabled. Press Reset to wake up");
    handle_sleep();
  });
  server.on("/mode/sleep/wifi", []() {
    server.send(200, "text/plain", "WIFI sleep enabled. Press Reset to wake up");
    handle_wifi_sleep();
  });

  server.begin();

  Serial.println("HTTP server started");
  delay(2500);
  analogWrite(LED_PIN, ledOnPwm);
}

void handle_sleep() {
  lcd.clear();
  lcd.print("DEEP SLEEP Press");
  analogWrite(LED_PIN, actualLedPwm);
  delay(500);
  lcd.setCursor(0, 1);
  lcd.print("reset to disable");
  backlightDim(actualLedPwm);
  lcd.noBacklight();
  ESP.deepSleep(0);
}
void handle_wifi_sleep() {
  lcd.clear();
  lcd.print("WIFI SLEEP");
  backlightDim(actualLedPwm);
  WiFi.disconnect();
}

void handle_http_not_found() {
  server.send(404, "text/plain", "Not Found");
}

void handle_http_root() {

  String headers = "<head><link rel=\"stylesheet\" href=\"https://maxcdn.bootstrapcdn.com/bootstrap/4.0.0/css/bootstrap.min.css\">";
  headers += "<meta name='viewport' content='width=device-width,initial-scale=1'></head>";
  String html = "<body><div class='container-fluid'><div class='row'>";
  html += "<div class='row'><div class='col-md-6'><h4>Mini-display options:</h4><br>";
  html += "<a class='btn btn-success btn-lg btn-block' href='/relay/on' target='frame'>220v ON</a><br>";
  html += "<a class='btn btn-dark btn-lg btn-block' href='/relay/off' target='frame'>220v OFF</a><br>";

  html += "<a class='btn btn-primary form-control' href='4/1' target='frame'>LCD backlight Low</a><br>";
  html += "<a class='btn btn-primary form-control' href='4/2' target='frame'>LCD backlight High</a><br>";
  html += "<a class='btn btn-primary form-control' href='4/0' target='frame'>LCD backlight Off</a><br><br>";

  html += "</div><div class='col-md-6'><h4>Message to Display:</h4>";
  html += "<br><form action='/lcd-write' target='frame' method='POST'>";
  html += "<input type='text' name='l1' maxlength=16 class='form-control'><br>";
  html += "<input type='text' name='l2' maxlength=16 class='form-control'><br>";
  html += "<input type='submit' value='Send to display' class='form-control'><form><br>";

  html += "<a class='btn btn-warning form-control' title='Keep relay switch state' href='/mode/sleep/wifi' target='frame'>Wifi sleep Mode</a>";
  html += "<a class='btn btn-danger form-control' href='/mode/sleep' target='frame'>Deep sleep Mode</a>";
  html += "</div></div></div></body>";
  html += "<iframe name='frame'></iframe>";
  server.send(200, "text/html", headers + html);
}

void handle4off() {
  analogWrite(LED_PIN, 0);
  server.send(200, "text/html");
}
void handle4low() {
  analogWrite(LED_PIN, ledOnPwm);
  actualLedPwm = ledOnPwm;
  server.send(200, "text/html");
}
void handle4high() {
  analogWrite(LED_PIN, ledOnPwmHigh);
  actualLedPwm = ledOnPwmHigh;
  server.send(200, "text/html");
}

void handleRelayOff() {
  lcd.clear();
  lcd.print("220v OFF");
  digitalWrite(RELAY_PIN, HIGH);
  analogWrite(LED_PIN, actualLedPwm);
  delay(200);
  backlightDim(actualLedPwm);
  server.send(200, "text/html");
}

void handleRelayOn() {
  lcd.clear();
  lcd.print("220v ON ");
  digitalWrite(RELAY_PIN, LOW);
  analogWrite(LED_PIN, actualLedPwm);
  delay(200);
  backlightDim(actualLedPwm);

  server.send(200, "text/html");
}

void handleLcdWrite() {
  displayMode = "message";
  analogWrite(LED_PIN, ledOnPwm + 300);

  // Analizo el POST iterando cada value
  if (server.args() > 0) {
    lcd.clear();
    for (byte i = 0; i < server.args(); i++) {
      if (server.argName(i) == "l1") {
        lcd.setCursor(0, 0);
        lcd.print(server.arg(i));
      }
      if (server.argName(i) == "l2") {
        lcd.setCursor(0, 1);
        lcd.print(server.arg(i));
      }
    }
  }

  server.send(200, "text/html");
  delay(1000);
  analogWrite(LED_PIN, ledOnPwm);
}

// Dim backlight from brightness to 0
void backlightDim(int brightness) {
  Serial.println(brightness);
  while (brightness > 1) {
    analogWrite(LED_PIN, brightness);
    delay(5);
    brightness--;
  }
  //Serial.println(brightness);
}

void loop(void) {
  server.handleClient();
}

