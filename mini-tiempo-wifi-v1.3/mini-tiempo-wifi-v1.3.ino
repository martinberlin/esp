/*
 * ESP8266 LLMNR responder sample
 * Copyright (C) 2017 Stephen Warren <swarren@wwwdotorg.org>
 *
 * Based on:
 * ESP8266 Multicast DNS (port of CC3000 Multicast DNS library)
 * Version 1.1
 * Copyright (c) 2013 Tony DiCola (tony@tonydicola.com)
 * ESP8266 port (c) 2015 Ivan Grokhotkov (ivan@esp8266.com)
 * MDNS-SD Suport 2015 Hristo Gochkov
 * Extended MDNS-SD support 2016 Lars Englund (lars.englund@gmail.com)
 *
 * License (MIT license):
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */
/*
 * This is an example of an HTTP server that is accessible via http://esp8266/
 * (or perhaps http://esp8266.local/) thanks to the LLMNR responder.
 *
 * Instructions:
 * - Update WiFi SSID and password as necessary.
 * - Flash the sketch to the ESP8266 board.
 * - Windows:
 *   - No additional software is necessary.
 *   - Point your browser to http://esp8266/, you should see a response. In most
 *     cases, it is important that you manually type the "http://" to force the
 *     browser to search for a hostname to connect to, rather than perform a web
 *     search.
 *   - Alternatively, run the following command from the command prompt:
 *     ping esp8266
 * - Linux:
 *   - To validate LLMNR, install the systemd-resolve utility.
 *   - Execute the following command:
 *     systemd-resolve -4 -p llmnr esp8266
 *   - It may be possible to configure your system to use LLMNR for all name
 *     lookups. However, that is beyond the scope of this description.
 *
 */

#include <ESP8266WiFi.h>
#include <ESP8266LLMNR.h>
#include <ESP8266WebServer.h>
#include <WiFiClient.h>
#include "DHT.h" //DHT Bibliothek laden

//LCD try
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Actual device, the IC2 address may change.
LiquidCrystal_I2C lcd(0x27, 16, 2);

#define LED_PIN D3
#define DHTPIN D5 //Der Sensor wird an PIN 2 angeschlossen    
#define DHTTYPE DHT11    // Es handelt sich um den DHT11 Sensor
DHT dht(DHTPIN, DHTTYPE); 

// WIFI Access Home / AndroidAP
//const char* ssid = "KabelBox-A210";
//const char* password = "14237187131701431551";
const char* ssid = "AndroidAP";
const char* password = "fasfasnar";

float temperature, humidity;
unsigned long previousMillis = 0; // When was the sensor last read
const long interval = 10000;      // Wait N seconds before reading again
const int deepSleepAfter = 6;     // 6 will go to sleep in a minute
int countTenSec = 0;              // Counter to go to sleep after N*10 seconds
int countTryConnection = 50;      // loop N times before giving up
int ledOnPwm = 100; // LCD backlight led
int ledOnPwmHigh = 800;
char* displayMode = "weather"; // Remember Strings go always with "" not with ''

ESP8266WebServer server(80);

void setup(void) {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  analogWrite(LED_PIN, 500);
  
  lcd.init();
  lcd.backlight();
  lcd.print("Mini-Tiempo WIFI");
  
  dht.begin();
  
  // Connect to WiFi network
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  lcd.setCursor(0, 1);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    lcd.print(".");
    countTryConnection--;
    if (countTryConnection == 0) {
       lcd.setCursor(0, 0);
       lcd.print(String(ssid) + " failed");
       lcd.setCursor(0, 1);
       lcd.print("Giving up");
       handle_sleep();
    }
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP());

  // Start LLMNR responder
  LLMNR.begin("esp8266");
  Serial.println("LLMNR responder started");
  // Start HTTP server
  server.onNotFound(handle_http_not_found);
  server.on("/", handle_http_root);
  server.on("/4/1", handle4low);
  server.on("/4/2", handle4high);
  server.on("/4/0", handle4off);
  server.on("/lcd-write", handleLcdWrite);
  
  server.on("/temp", [](){
    read_sensor();
    server.send(200, "text/plain", String(temperature));
  });

  server.on("/humi", [](){
    read_sensor();
    server.send(200, "text/plain", String(humidity));
});

  server.on("/mode/weather", [](){
    displayMode = "weather";
    read_sensor();
    server.send(200, "text/plain");
  });
  
  server.begin();
  
  Serial.println("HTTP server started");
  delay(2500);
  analogWrite(LED_PIN, ledOnPwm);
}

void read_sensor() {
  // Wait at least 2 seconds seconds between measurements.
  // If the difference between the current time and last time you read
  // the sensor is bigger than the interval you set, read the sensor.
  // Works better than delay for things happening elsewhere also.
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    countTenSec++;
    // Save the last time you read the sensor
    previousMillis = currentMillis;

    // Reading temperature and humidity takes about 250 milliseconds!
    // Sensor readings may also be up to 2 seconds 'old' (it's a very slow sensor)
    humidity = dht.readHumidity();        // Read humidity as a percent
    temperature = dht.readTemperature();  // Read temperature as Celsius

    // Check if any reads failed and exit early (to try again).
    if (isnan(humidity) || isnan(temperature)) {
      Serial.println("Failed to read from DHT sensor!");
      return;
    }

    // No idea why prints always a float ending in .00
    //Serial.print("Humidity: "+ String(humidity));
    //Serial.print(" %\t");
    //Serial.println((float)temperature);
    //Serial.println(" °C");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Temp: "+ String(temperature));
    lcd.setCursor(0, 1);
    lcd.print("Humi: "+ String(int(humidity))+"%");

    if (countTenSec > deepSleepAfter) {
      // Go to deep sleep after N seconds x10
      handle_sleep();
    }
  }
}

void handle_sleep() {
  lcd.print(" SLEEP");
  for (ledOnPwm; ledOnPwm=1; ledOnPwm--) {
    analogWrite(LED_PIN, ledOnPwm);
    delay(50);
  }
  ESP.deepSleep(0);
}

void handle_http_not_found() {
  server.send(404, "text/plain", "Not Found");
}

void handle_http_root() {
  read_sensor();
  String headers = "<head><link rel=\"stylesheet\" href=\"https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/css/bootstrap.min.css\"></head>";
  String html = "<body><div class='container'><div class='row'>";
  html += "<div class='row'><div class='col-md-4'><h3>Server options:</h3><br>";
  html += "<a class='btn btn-primary' href='4/1' target='frame'>LCD backlight Low</a><br>";
  html += "<a class='btn btn-primary' href='4/2' target='frame'>LCD backlight High</a><br>";
  html += "<a class='btn btn-primary' href='4/0' target='frame'>LCD backlight Off</a><br><br>";
  html += "<a class='btn btn-default' href='/temp' target='blank'>API read temperature</a><br>";
  html += "<a class='btn btn-default' href='/humi' target='blank'>API read humidity</a><br>";
  html += "</div><div class='col-md-2'><h3>DHT Sensor</h3>";
  html += "Temperature: "+String(temperature)+" Celsius<br>";
  html += "Humidity: "+String(humidity)+" %<br>";

  html += "<br><form action='/lcd-write' target='frame' method='POST'>";
  html += "<input type='text' name='l1' maxlength=16 class='form-control'><br>";
  html += "<input type='text' name='l2' maxlength=16 class='form-control'><br>";
  html += "<input type='submit' value='Send to display' class='form-control'><form>";
  html += "<a class='btn btn-default form-control' href='/mode/weather' target='frame'>Back to mode weather</a>";
  html += "</div></div></div></body>";
  html += "<iframe name='frame' class='hidden'></iframe>";
  server.send(200, "text/html", headers + html);
}

void handle4off() {
  analogWrite(LED_PIN, 0);
  server.send(200, "text/html");
}
void handle4low() {
  analogWrite(LED_PIN, ledOnPwm);
  server.send(200, "text/html");
}
void handle4high() {
  analogWrite(LED_PIN, ledOnPwmHigh);
  server.send(200, "text/html");
}
void handleLcdWrite() {
  displayMode = "message";
  analogWrite(LED_PIN, ledOnPwm+300);
  
  // Analizo el POST iterando cada value
  if (server.args()>0) {
    lcd.clear();
    for (byte i=0; i < server.args(); i++) {
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

void loop(void) {
  server.handleClient();
  
  if (displayMode == "weather") {
    read_sensor();
  }
}

