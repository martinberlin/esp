/*########################   Weather Display  #############################
   Receives and displays the weather forecast from the Weather Underground and then displays using a
   JSON decoder wx data to display on a web page using a webserver.
  Weather author at http://dsbird.org.uk

  IP address:
  192.168.43.115
  MAC:
  84:F3:EB:5A:46:9B

*/

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>     // https://github.com/bblanchon/ArduinoJson
#include <WiFiClient.h>
#include "time.h"
#include <SPI.h>
#include <GxEPD.h>
#include <GxGDEW075T8/GxGDEW075T8.cpp>
#include <GxIO/GxIO_SPI/GxIO_SPI.cpp>
#include <GxIO/GxIO.cpp>
#include <pgmspace.h>
// FONT used for title / message
#include <Fonts/OpenSans_Regular7pt8b.h>
#include <Fonts/OpenSans_Regular10pt8b.h>

const char* ssid     = "AndroidAP";
const char* password = "fasfasnar";

const char* domainName = "display"; // mDNS: display.local

#define SD_BUFFER_PIXELS 20 // Image example
// TCP server at port 80 will respond to HTTP requests
ESP8266WebServer server(80);

String City          = "Berlin";
String Country       = "Germany";                     // Your country ES=Spain use %20 for spaces (should be urlencoded)
boolean skipLoadingScreen = true;                   // Skips loading screen and makes it faster

//################# LIBRARIES ##########################
String version = "1.1";       // Version of this program
//################ VARIABLES ###########################

// pins_arduino.h, e.g. WEMOS D1 Mini
//CLK  = D8; D
//DIN  = D7; D
//BUSY = D6; D
//CS   = D0; D
//RST  = D4;
//DC   = D3;
// GxIO_SPI(SPIClass& spi, int8_t cs, int8_t dc, int8_t rst = -1, int8_t bl = -1);
GxIO_Class io(SPI, D0, D3, D4);
// GxGDEP015OC1(GxIO& io, uint8_t rst = D4, uint8_t busy = D2);
GxEPD_Class display(io, D4, D6 );

unsigned long  lastConnectionTime = 0;          // Last time you connected to the server, in milliseconds

//unsigned long  startMillis = millis();
const unsigned long  serverDownTime = millis() + 30 * 60 * 1000; // Min / Sec / Millis Delay between updates, in milliseconds, WU allows 500 requests per-day maximum, set to every 10-mins or 144/day


WiFiClient client; // wifi client object

void setup() {
  Serial.begin(115200);
  StartWiFi(ssid, password);

  display.init();
  //display.setRotation(3); // Right setup to get KEY1 on top. Funny to comment it and see how it works ;)
  display.setFont(&OpenSans_Regular10pt8b);

  if (skipLoadingScreen == false) {
    //currentTime = obtain_time();
    display.fillScreen(GxEPD_BLACK); // No need to do this. Init cleans screen
    display.setTextColor(GxEPD_WHITE);
    display.setCursor(0, 12);
    display.println("\n\r            FASAREK CORP\n\r");
    //display.println("Time: " + currentTime);
    display.update();
  }
  display.setTextColor(GxEPD_BLACK);
  // Does not work, creates endless loop
  //  obtain_forecast("forecast");
  //  DisplayForecast();

  //  Serial.print("currentTime = "+currentTime);

  // Start HTTP server
  server.onNotFound(handle_http_not_found);
  server.on("/", handle_http_root);
  server.on("/display-write", handleDisplayWrite);
  server.on("/display-clean", handleDisplayClean);
  server.on("/deep-sleep", handleDeepSleep);
  server.on("/image-test",handleImageTest);
  
  delay(1000);
  server.begin(); // not needed?
  // Moved to loop()
  //ESP.deepSleep(0); // ESP Wemos deep sleep. Wakes up and starts the complete sketch so it makes no sense to make a loop here
}

String obtain_time() {
  String host = "slosarek.eu";
  String url = "/api/time.php";

  // Use WiFiClientSecure class if you need to create TLS connection
  //WiFiClient httpclient;

  String request;
  request  = "GET " + url + " HTTP/1.1\r\n";
  request += "Accept: */*\r\n";
  request += "Host: " + host + "\r\n";
  request += "Connection: close\r\n";
  request += "\r\n";
  Serial.println(request);

  if (! client.connect(host, 80)) {
    Serial.println("connection failed");
    client.flush();
    client.stop();
    return "connection failed";
  }
  client.print(request); //send the http request to the server
  client.flush();

  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      Serial.println(">>> Client Timeout !");
      client.stop();
      return "Client timeout";
    }
  }

  bool   skip_headers = true;
  String rx_line;
  String response;

  // Read all the lines of the reply from server and print them to Serial
  while (client.available()) {
    rx_line = client.readStringUntil('\r');
    if (rx_line.length() <= 1) { // a blank line denotes end of headers
      skip_headers = false;
    }
    // Collect http response
    if (!skip_headers) {
      response += rx_line;
    }
  }
  response.trim();
  return response;
}

int StartWiFi(const char* ssid, const char* password) {
  int connAttempts = 0;
  Serial.println("\r\nConnecting to: " + String(ssid));
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED ) {
    delay(500); Serial.print(".");
    if (connAttempts > 30) {
      Serial.println("ERROR connecting to WiFi failed");
      return -5;
    }
    connAttempts++;
  }
  // wifi_set_sleep_type(LIGHT_SLEEP_T);
  Serial.println("WiFi connected\r\nIP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("MAC: ");
  Serial.println(WiFi.macAddress());

  // Set up mDNS responder:
  // - first argument is the domain name, in this example
  //   the fully-qualified domain name is "esp8266.local"
  // - second argument is the IP address to advertise
  //   we send our IP address on the WiFi network
  if (!MDNS.begin(domainName)) {
    Serial.println("Error setting up MDNS responder!");
    while (1) {
      delay(1000);
    }
  }
  Serial.println("mDNS responder started");

  // Start TCP (HTTP) server
  server.begin();
  Serial.println("TCP server started");

  // Add service to MDNS-SD
  MDNS.addService("http", "tcp", 80);
  return 1;
}

void clear_screen() {
  display.fillScreen(GxEPD_WHITE);
  display.update();
}


//###########################################################################
// Symbols are drawn on a relative 10x15 grid and 1 scale unit = 1 drawing unit
void addcloud(int x, int y, int scale) {
  int linesize = 3;
  //Draw cloud outer
  display.fillCircle(x - scale * 3, y, scale, GxEPD_BLACK);                       // Left most circle
  display.fillCircle(x + scale * 3, y, scale, GxEPD_BLACK);                       // Right most circle
  display.fillCircle(x - scale, y - scale, scale * 1.4, GxEPD_BLACK);            // left middle upper circle
  display.fillCircle(x + scale * 1.5, y - scale * 1.3, scale * 1.75, GxEPD_BLACK); // Right middle upper circle
  display.fillRect(x - scale * 3, y - scale, scale * 6, scale * 2 + 1, GxEPD_BLACK); // Upper and lower lines
  //Clear cloud inner
  display.fillCircle(x - scale * 3, y, scale - linesize, GxEPD_WHITE);            // Clear left most circle
  display.fillCircle(x + scale * 3, y, scale - linesize, GxEPD_WHITE);            // Clear right most circle
  display.fillCircle(x - scale, y - scale, scale * 1.4 - linesize, GxEPD_WHITE); // left middle upper circle
  display.fillCircle(x + scale * 1.5, y - scale * 1.3, scale * 1.75 - linesize, GxEPD_WHITE); // Right middle upper circle
  display.fillRect(x - scale * 3, y - scale + linesize, scale * 6, scale * 2 - linesize * 2 + 1, GxEPD_WHITE); // Upper and lower lines
}

void addtstorm(int x, int y, int scale) {
  y = y + scale / 2;
  for (int i = 0; i < 5; i++) {
    display.drawLine(x - scale * 4 + scale * i * 1.5 + 0, y + scale * 1.5, x - scale * 3.5 + scale * i * 1.5 + 0, y + scale, GxEPD_BLACK);
    display.drawLine(x - scale * 4 + scale * i * 1.5 + 1, y + scale * 1.5, x - scale * 3.5 + scale * i * 1.5 + 1, y + scale, GxEPD_BLACK);
    display.drawLine(x - scale * 4 + scale * i * 1.5 + 2, y + scale * 1.5, x - scale * 3.5 + scale * i * 1.5 + 2, y + scale, GxEPD_BLACK);
    display.drawLine(x - scale * 4 + scale * i * 1.5, y + scale * 1.5 + 0, x - scale * 3 + scale * i * 1.5 + 0, y + scale * 1.5 + 0, GxEPD_BLACK);
    display.drawLine(x - scale * 4 + scale * i * 1.5, y + scale * 1.5 + 1, x - scale * 3 + scale * i * 1.5 + 0, y + scale * 1.5 + 1, GxEPD_BLACK);
    display.drawLine(x - scale * 4 + scale * i * 1.5, y + scale * 1.5 + 2, x - scale * 3 + scale * i * 1.5 + 0, y + scale * 1.5 + 2, GxEPD_BLACK);
    display.drawLine(x - scale * 3.5 + scale * i * 1.4 + 0, y + scale * 2.5, x - scale * 3 + scale * i * 1.5 + 0, y + scale * 1.5, GxEPD_BLACK);
    display.drawLine(x - scale * 3.5 + scale * i * 1.4 + 1, y + scale * 2.5, x - scale * 3 + scale * i * 1.5 + 1, y + scale * 1.5, GxEPD_BLACK);
    display.drawLine(x - scale * 3.5 + scale * i * 1.4 + 2, y + scale * 2.5, x - scale * 3 + scale * i * 1.5 + 2, y + scale * 1.5, GxEPD_BLACK);
  }
}

void handle_http_not_found() {
  server.send(404, "text/plain", "Not Found");
}

void handle_http_root() {

  String headers = "<head><link rel=\"stylesheet\" href=\"https://maxcdn.bootstrapcdn.com/bootstrap/4.0.0/css/bootstrap.min.css\">";
  headers += "<meta name='viewport' content='width=device-width,initial-scale=1'></head>";
  String html = "<body><div class='container-fluid'><div class='row'>";
  html += "<div class='col-md-10'><h4>" + String(domainName) + ".local</h4>";
  html += "<br><form action='/display-write' target='frame' method='POST'>";
  html += "<label for='url'>Parse Url:</label><input placeholder='http://' id='url' name='url' class='form-control'><br>";
  html += "<input type='submit' value='Website text to display' class='btn btn-dark'>";
  html += "<input type='button' value='Clean Url' onclick='document.getElementById(\"url\").value = \"\"' class='btn btn-default'><br><br>";
  html += "<label for='title'>Title:</label><input onblur='document.getElementById(\"url\").value = \"\"' id='title' name='title' class='form-control'><br>";

  html += "<a href='/image-test' target='frame'>Download test image</a><br>";
  html += "<textarea placeholder='Content' name='text' rows=6 class='form-control'></textarea>";
  html += "<input type='submit' value='Send to display' class='btn btn-success'><form>";

  html += "<a class='btn btn-default' role='button' target='frame' href='/display-clean'>Clean screen</a><br><br><br>";
  html += "<a href='/deep-sleep' target='frame'>Deep sleep</a>";
  html += "</div></div></div></body>";
  html += "<iframe name='frame'></iframe>";
  server.send(200, "text/html", headers + html);
}

void handleDeepSleep() {
  ESP.deepSleep(20e6);
  server.send(200, "text/html", "Going to deep-sleep");
}


void handleDisplayClean() {
  display.fillScreen(GxEPD_WHITE);
  display.update();
  server.send(200, "text/html", "Clearing display");
}

void handleDisplayWrite() {
  display.fillScreen(GxEPD_WHITE);

  // Analizo el POST iterando cada value
  if (server.args() > 0) {
    for (byte i = 0; i < server.args(); i++) {

      if (server.argName(i) == "url" && server.arg(i) != "") {
        display.setCursor(0, 23);
        url_to_display(server.arg(i));
        display.update();
        break;
      }

      if (server.argName(i) == "title") {
        display.setFont(&OpenSans_Regular10pt8b);
        display.setCursor(0, 33);
        display.print(server.arg(i));
      }
      if (server.argName(i) == "text") {
        display.setFont(&OpenSans_Regular7pt8b);
        display.setCursor(0, 63);
        display.print(server.arg(i));
      }
    }
  }
  display.update();
  server.send(200, "text/html", "Text sent to display");
}

void url_to_display(String url) {
  String host = "slosarek.eu";
  String api = "/api/htm2txt.php?pre=1&u=";

  String request;
  request  = "GET " + api + url + " HTTP/1.1\r\n";
  request += "Accept: */*\r\n";
  request += "Host: " + host + "\r\n";
  request += "Connection: close\r\n";
  request += "\r\n";
  Serial.println(request);

  if (! client.connect(host, 80)) {
    Serial.println("connection failed");
    client.flush();
    client.stop();
  }
  client.print(request); //send the http request to the server
  client.flush();

  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      Serial.println(">>> Client Timeout !");
      client.stop();
    }
  }

  bool   skip_headers = true;
  String rx_line;
  String response;
  int lines_count = 0;
  // Read all the lines of the reply from server and print them to Serial
  while (client.available()) {
    rx_line = client.readStringUntil('\n');
    if (rx_line.length() <= 1) { // a blank line denotes end of headers
      skip_headers = false;
    }

    // Collect http response
    if (!skip_headers) {
      response += rx_line;
      rx_line.trim();

      if (rx_line != "" && rx_line != "-") {
        if (lines_count < 2) {
          display.setFont(&OpenSans_Regular10pt8b);
        }
        if (lines_count == 2) {
          display.setFont(&OpenSans_Regular7pt8b);
        }
        display.println(rx_line);
        lines_count++;
        //Serial.println(String(lines_count));
      }
    }
  }
  //response.trim();
}


void handleImageTest() {
  String host = "slosarek.eu";
  String image = "/api/uploads/luckycloud.bmp";

  String request;
  request  = "GET " + image + " HTTP/1.1\r\n";
  request += "Accept: */*\r\n";
  request += "Host: " + host + "\r\n";
  request += "Connection: close\r\n";
  request += "\r\n";
  Serial.println(request);

  if (! client.connect(host, 80)) {
    Serial.println("connection failed");
    client.flush();
    client.stop();
    return;
  }
  client.print(request); //send the http request to the server
  client.flush();
  display.fillScreen(GxEPD_WHITE);
  
  uint8_t x = 0;
  uint8_t y = 0;
  bool valid = false; // valid format to be handled
  bool flip = false; // bitmap is stored bottom-to-top
  uint32_t pos = 0;
  uint32_t startTime = millis();
  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      Serial.println(">>> Client Timeout !");
      client.stop();
      return;
    }
  }

  bool   skip_headers = true;
  String rx_line;
  String response;
  long bytesRead = 0;
  
  // Read all the lines of the reply from server and print them to Serial
  while (client.available()) {

  if (read16() == 0x4D42) { // BMP signature
    uint32_t fileSize = read32();
    uint32_t creatorBytes = read32();
    uint32_t imageOffset = read32(); // Start of image data
    uint32_t headerSize = read32();
    uint32_t width  = read32();
    uint32_t height = read32();
    uint16_t planes = read16();
    uint16_t depth = read16(); // bits per pixel
    uint32_t format = read32();
    
    if ((planes == 1) && (format == 0)) { // uncompressed is handled
      Serial.print("File size: "); Serial.println(fileSize);
      Serial.print("Image Offset: "); Serial.println(imageOffset);
      Serial.print("Header size: "); Serial.println(headerSize);
      Serial.print("Width * Height: "); Serial.print(String(width) + " x " + String(height));
      Serial.print(" / Bit Depth: "); Serial.println(depth);
      Serial.print("Planes: "); Serial.println(planes);Serial.print("Format: "); Serial.println(format);
      // BMP rows are padded (if needed) to 4-byte boundary
      uint32_t rowSize = (width * depth / 8 + 3) & ~3;
      if (height < 0)
      {
        height = -height;
      }
      uint16_t w = width;
      uint16_t h = height;
      if ((x + w - 1) >= display.width())  w = display.width()  - x;
      if ((y + h - 1) >= display.height()) h = display.height() - y;
      
      for (uint16_t row = 0; row < h; row++) // for each line
      {
        uint8_t bits;
        
        for (uint16_t col = 0; col < w; col++) // for each pixel
        {
          
          switch (depth)
          {
            case 1: // one bit per pixel b/w format
              {
                valid = true;
                if (0 == col % 8)
                {
                  bits = client.read();
                  bytesRead++;
                }
                uint16_t bw_color = bits & 0x80 ? GxEPD_WHITE : GxEPD_BLACK;
                display.drawPixel(col, row, bw_color);
                bits <<= 1;
              }
              break;
              
            case 24: // standard BMP format
              {
                valid = true;
                uint16_t b = client.read();
                uint16_t g = client.read();
                uint16_t r = client.read();
                uint16_t bw_color = ((r + g + b) / 3 > 0xFF  / 2) ? GxEPD_WHITE : GxEPD_BLACK;
                display.drawPixel(col, row, bw_color);
                bytesRead = bytesRead +3;
              }
              break;
          }
        } // end pixel
      } // end line

       server.send(200, "text/html", "Image sent to display");
       Serial.println("Bytes read:"+ String(bytesRead));
       display.update();
       client.stop();
       break;
       
    } else {
      display.print("Compressed BMP files are not handled");
      display.update();
    }
//}


      }
  }
         
}


uint16_t read16()
{
  // BMP data is stored little-endian, same as Arduino.
  uint16_t result;
  ((uint8_t *)&result)[0] = client.read(); // LSB
  ((uint8_t *)&result)[1] = client.read(); // MSB
  return result;
}

uint32_t read32()
{
  // BMP data is stored little-endian, same as Arduino.
  uint32_t result;
  ((uint8_t *)&result)[0] = client.read(); // LSB
  ((uint8_t *)&result)[1] = client.read();
  ((uint8_t *)&result)[2] = client.read();
  ((uint8_t *)&result)[3] = client.read(); // MSB
  return result;
}

void loop() {

  // Add  milisec comparison to make server work for 1 min / 90 sec
  if (millis() < serverDownTime) {
    server.handleClient();
  } else {
    Serial.println(" Server going down");
    display.powerDown();
    ESP.deepSleep(0);
  }

}

