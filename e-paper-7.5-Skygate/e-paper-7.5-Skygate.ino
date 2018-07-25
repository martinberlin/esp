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
//needed for library
#include <DNSServer.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager

#include <WiFiClient.h>
#include <SPI.h>
#include <GxEPD.h>
#include <GxGDEW075T8/GxGDEW075T8.cpp>
#include <GxIO/GxIO_SPI/GxIO_SPI.cpp>
#include <GxIO/GxIO.cpp>
// FONT used for title / message
#include <Fonts/quicksand_bold_webfont14pt8b.h>
#include <Fonts/quicksand_bold_webfont18pt8b.h>

//Converting fonts with ümlauts: ./fontconvert *.ttf 18 32 252

// No MORE ssid / passwords! Use wifiManager
//const char* password = "14237187131701431551";
// Default config mode Access point
const char* configModeAP = "Display-autoconnect";
// mDNS: display.local
const char* domainName = "display"; 
String message;
// Makes a div id="m" containing response message to dissapear after 3 seconds
String javascriptFadeMessage = "<script>setTimeout(function(){document.getElementById('m').innerHTML='';},3000);</script>";
#define SD_BUFFER_PIXELS 20 // Image example
// TCP server at port 80 will respond to HTTP requests
ESP8266WebServer server(80);

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

//unsigned long  startMillis = millis();
const unsigned long  serverDownTime = millis() + 30 * 60 * 1000; // Min / Sec / Millis Delay between updates, in milliseconds, WU allows 500 requests per-day maximum, set to every 10-mins or 144/day


WiFiClient client; // wifi client object

void setup() {
  Serial.begin(115200);

  display.init();
  display.setRotation(2); // Rotates display N times clockwise
  display.setFont(&quicksand_bold_webfont14pt8b);
 
  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  wifiManager.autoConnect(configModeAP);
  wifiManager.setAPCallback(configModeCallback);
  
  display.setTextColor(GxEPD_BLACK);

/*
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
  // Add service to MDNS-SD
  MDNS.addService("http", "tcp", 80);
  */
  // Start HTTP server
  server.onNotFound(handle_http_not_found);
  // Routing
  server.on("/", handle_http_root);
  server.on("/display-write", handleDisplayWrite);
  server.on("/web-image", handleWebToDisplay);
  server.on("/display-clean", handleDisplayClean);
  server.on("/deep-sleep", handleDeepSleep);
  server.begin();
}

void configModeCallback (WiFiManager *myWiFiManager) {
  message = "Display can't connect to WiFi. Entering config mode\nPlease connect to: "+String(configModeAP)+"\n";
  message = "And browse" + String(WiFi.softAPIP())+ " to configure the display WiFi connection.";
  Serial.println(message);
  displayMessage(message);
  
  Serial.println(WiFi.softAPIP());

  Serial.println(myWiFiManager->getConfigPortalSSID());
}

void handle_http_not_found() {
  server.send(404, "text/plain", "Not Found");
}

void handle_http_root() {

  String headers = "<head><link rel=\"stylesheet\" href=\"https://maxcdn.bootstrapcdn.com/bootstrap/4.0.0/css/bootstrap.min.css\">";
  headers += "<meta name='viewport' content='width=device-width,initial-scale=1'></head>";
  String html = "<body><main role='main'><div class='container-fluid'><div class='row'>";
  html += "<div class='col-md-10'><h4>" + String(domainName) + ".local</h4>";
  html += "<br><form id='f' action='/web-image' target='frame' method='POST'>";
  html += "<label for='url'>Parse Url:</label><input placeholder='http://' id='url' name='url' type='url' class='form-control'>";
  html += "<div class='row'><div class='col-md-4'>";
  html += "<input type='submit' value='Website screenshot' class='btn btn-dark'>";
  html += "</div><div class='col-md-6'>";
  html += "<select name='zoom' class='form-control'>";
  html += "<option value='2'>2</option>";
  html += "<option value='1.5'>1.5</option>";
  html += "<option value='1.4'>1.4</option>";
  html += "<option value='1.2'>1.2 20% zoomed</option>";
  html += "<option value='' selected>zoom</option>";
  html += "<option value='1'>1 no zoom</option>";
  html += "<option value='.9'>.9 10% smaller</option>";
  html += "<option value='.85'>.85</option>";
  html += "<option value='.8'>.8</option>";
  html += "<option value='.7'>.7</option>";
  html += "<option value='.6'>.6</option>";
  html += "<option value='.5'>.5 half size</option></select>";
  html += "</div><div class='col-md-2'>";
  html += "<input type='button' onclick='document.getElementById(\"url\").value=\"\"' value='Clean Url' class='btn btn-default'></div></div></form>";
  html += "<form id='f2' action='/display-write' target='frame' method='POST'>";
  html += "<label for='title'>Title:</label><input id='title' name='title' class='form-control'><br>";
  html += "<textarea placeholder='Content' name='text' rows=4 class='form-control'></textarea>";
  html += "<input type='submit' value='Send to display' class='btn btn-success'></form>";

  html += "<a class='btn btn-default' role='button' target='frame' href='/display-clean'>Clean screen</a><br>";
  html += "<iframe name='frame'></iframe>";
  html += "<a href='/deep-sleep' target='frame'>Deep sleep</a><br>";
  html += "</div></div></div></main>";

  html += "</body>";

  server.send(200, "text/html", headers + html);
}

void handleDeepSleep() {
  server.send(200, "text/html", "Going to deep-sleep. Reset to wake up");
  delay(1);
  ESP.deepSleep(20e6);
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

      if (server.argName(i) == "title") {
        display.setFont(&quicksand_bold_webfont18pt8b);
        display.setCursor(10, 43);
        display.print(server.arg(i));
      }
      if (server.argName(i) == "text") {
        display.setFont(&quicksand_bold_webfont14pt8b);
        display.setCursor(10, 75);
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
          //display.setFont(&OpenSans_Regular10pt8b);
        }
        if (lines_count == 2) {
          //display.setFont(&OpenSans_Regular7pt8b);
        }
        display.println(rx_line);
        lines_count++;
        //Serial.println(String(lines_count));
      }
    }
  }
  //response.trim();
}
void handleWebToDisplay() {
  String url = "";
  String zoom = ".8";
  if (server.args() > 0) {
    for (byte i = 0; i < server.args(); i++) {
      if (server.argName(i) == "url" && server.arg(i) != "") {
        url = server.arg(i);
      }
      if (server.argName(i) == "zoom" && server.arg(i) != "") {
        zoom = server.arg(i);
      }
    }
  }
    if (url == "") {
      display.println("No Url received");
      display.update();
      return;
    }
  String host = "slosarek.eu";
  String image = "/api/web-image/?u=" + url + "&z=" + zoom;
   
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
 
  uint32_t startTime = millis();
  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      Serial.println(">>> Client Timeout !");
      client.stop();
      return;
    }
  }
  uint8_t buffer[3 * 10]; // pixel buffer, size for r,g,b
  int displayWidth = display.width();
  int displayHeight= display.height();

  long bytesRead = 32; // summing the whole read headers
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
    
      Serial.print("BMP read. File size: "); Serial.println(fileSize);
      Serial.print("Image Offset: "); Serial.println(imageOffset);
      Serial.print("Header size: "); Serial.println(headerSize);
      Serial.print("Width * Height: "); Serial.print(String(width) + " x " + String(height));
      Serial.print(" / Bit Depth: "); Serial.println(depth);
      Serial.print("Planes: "); Serial.println(planes);Serial.print("Format: "); Serial.println(format);
    
    if ((planes == 1) && (format == 0)) { // uncompressed is handled

      // BMP rows are padded (if needed) to 4-byte boundary
      //uint32_t rowSize = (width * depth / 8 + 3) & ~3;
      if (height < 0)
      {
        height = -height;
        flip = false;
      }
      uint16_t w = width;
      uint16_t h = height;
      // Attempt to move pointer where image starts
      client.readBytes(buffer, imageOffset-bytesRead); 
      size_t buffidx = sizeof(buffer); // force buffer load
      
      for (uint16_t row = 0; row < h; row++) // for each line
      {
        uint8_t bits;
        
        for (uint16_t col = 0; col < w; col++) // for each pixel
        {
          // Time to read more pixel data?
          if (buffidx >= sizeof(buffer))
          {
            client.readBytes(buffer, sizeof(buffer));
            buffidx = 0; // Set index to beginning
          }
          switch (depth)
          {
            case 1: // one bit per pixel b/w format
              {
                valid = true;
                if (0 == col % 8)
                {
                  bits = buffer[buffidx++];
                  bytesRead++;
                }
                uint16_t bw_color = bits & 0x80 ? GxEPD_BLACK : GxEPD_WHITE;
                display.drawPixel(col, displayHeight-row, bw_color);
                //Serial.println("c:"+ String(col));
                bits <<= 1;
              }
              break;
            case 4: // 4 work in progress
              {
                if (0 == col % 2) {
                  bits = buffer[buffidx++];
                  bytesRead++;
                }   
                bits <<= 1;           
                bits <<= 1;
                uint16_t bw_color = bits > 0x80 ? GxEPD_WHITE : GxEPD_BLACK;
                display.drawPixel(col, displayHeight-row, bw_color);
                bits <<= 1;
                bits <<= 1;
              }
              break;
            case 24: // standard BMP format
              {
                uint16_t b = buffer[buffidx++];
                uint16_t g = buffer[buffidx++];
                uint16_t r = buffer[buffidx++];
                uint16_t bw_color = ((r + g + b) / 3 > 0xFF  / 2) ? GxEPD_WHITE : GxEPD_BLACK;
                display.drawPixel(col, displayHeight-row, bw_color);
                bytesRead = bytesRead +3;
              }
              break;
          }
        } // end pixel
      } // end line

       server.send(200, "text/html", "<div id='m'>Image sent to display</div>"+javascriptFadeMessage);
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
  //Serial.print(result, HEX);
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

// Displays message doing a partial update
void displayMessage(String message) {
  display.setTextColor(GxEPD_WHITE);
  display.fillRect(0,0,display.width(),60,GxEPD_BLACK);
  display.setCursor(15, 25);
  display.print(message);
  display.updateWindow(0,0,display.width(),60, true); // Attempt partial update
  display.update();
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

