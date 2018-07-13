/*########################   Weather Display  #############################
 * Receives and displays the weather forecast from the Weather Underground and then displays using a 
 * JSON decoder wx data to display on a web page using a webserver.
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
#include <Fonts/FreeSans24pt7b.h>
#include <Fonts/FreeSans18pt7b.h>


const char* ssid     = "AndroidAP";
const char* password = "fasfasnar";

const char* domainName= "carlos";  // mDNS: carlos.local

// TCP server at port 80 will respond to HTTP requests
ESP8266WebServer server(80);

String City          = "Berlin";
String Country       = "DE";                     // Your country ES=Spain use %20 for spaces (should be urlencoded)   
boolean skipLoadingScreen = true;                   // Skips loading screen and makes it faster

//################# LIBRARIES ##########################
String version = "1.1";       // Version of this program
//################ VARIABLES ###########################

// pins_arduino.h, e.g. WEMOS D1 Mini
//static const uint8_t SS    = D8;
//static const uint8_t MOSI  = D7 //DIN;
//BUSY = D2; --> updated to D2
//DIN  = D7;
// GxIO_SPI(SPIClass& spi, int8_t cs, int8_t dc, int8_t rst = -1, int8_t bl = -1);
GxIO_Class io(SPI, D0, D3, D4); 
// GxGDEP015OC1(GxIO& io, uint8_t rst = D4, uint8_t busy = D2);
GxEPD_Class display(io, D4, D6 ); 

//------ NETWORK VARIABLES---------
// Use your own API key by signing up for a free developer account at http://www.wunderground.com/weather/api/
String API_key       = "ecfde31ed95eb892";            // See: http://www.wunderground.com/weather/api/d/docs (change here with your KEY)

String Conditions    = "conditions";                  // See: http://www.wunderground.com/weather/api/d/docs?d=data/index&MR=1
char   wxserver[]    = "api.wunderground.com";        // Address for WeatherUnderGround
unsigned long        lastConnectionTime = 0;          // Last time you connected to the server, in milliseconds

//unsigned long  startMillis = millis();
const unsigned long  serverDownTime = millis() + 30*60*1000; // Min / Sec / Millis Delay between updates, in milliseconds, WU allows 500 requests per-day maximum, set to every 10-mins or 144/day
String Units      =  "M"; // M for Metric, X for Mixed and I for Imperial

//################ PROGRAM VARIABLES and OBJECTS ################
// Conditions
String WDay0, Day0, Icon0, High0, Low0, Conditions0, Pop0, Averagehumidity0,
       WDay1, Day1, Icon1, High1, Low1, Conditions1, Pop1, Averagehumidity1,
       WDay2, Day2, Icon2, High2, Low2, Conditions2, Pop2, Averagehumidity2,
       WDay3, Day3, Icon3, High3, Low3, Conditions3, Pop3, Averagehumidity3;
 

// Astronomy
String  DphaseofMoon, Sunrise, Sunset, Moonrise, Moonset, Moonlight;

String currCondString; // string to hold received API weather data
String currentTime;

const unsigned char thermo_icon[] = { // 64x24
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfc, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xdf, 0xff, 
0xf9, 0x9f, 0xff, 0xff, 0xff, 0xff, 0xdf, 0xff, 0xfb, 0xdf, 0xff, 0xff, 0xf3, 0xff, 0xdf, 0xff, 0xfb, 0x5f, 0xff, 0xff, 0xe9, 0xff, 0x0f, 0xff, 
0xfb, 0x5f, 0xfc, 0x7f, 0xed, 0xbf, 0x0f, 0xff, 0xfa, 0x5e, 0x18, 0x3f, 0xed, 0x7e, 0x07, 0xff, 0xfb, 0x5e, 0xd9, 0x9f, 0xed, 0x7e, 0x07, 0xff, 
0xfb, 0x5e, 0xd3, 0xdf, 0xe3, 0x7c, 0x03, 0xff, 0xfa, 0x5e, 0x13, 0xff, 0xf2, 0xfc, 0x03, 0xdf, 0xfb, 0x5f, 0xf7, 0xff, 0xfe, 0xfc, 0x13, 0xdf, 
0xfb, 0x5f, 0xf7, 0xff, 0xfd, 0x9c, 0x07, 0xdf, 0xfa, 0x5f, 0xf7, 0xff, 0xfd, 0x4e, 0x07, 0x8f, 0xfb, 0x5f, 0xf3, 0xdf, 0xfb, 0x6f, 0x0f, 0x8f, 
0xfb, 0x5f, 0xf1, 0x9f, 0xfb, 0x6f, 0xff, 0x07, 0xfa, 0x5f, 0xf8, 0x3f, 0xfb, 0x6f, 0xff, 0x07, 0xfa, 0x1f, 0xfc, 0x7f, 0xf7, 0x1f, 0xff, 0x03, 
0xf0, 0x0f, 0xff, 0xff, 0xff, 0x9f, 0xfe, 0x03, 0xe0, 0x07, 0xff, 0xff, 0xff, 0xff, 0xfe, 0x03, 0xe0, 0x07, 0xff, 0xff, 0xff, 0xff, 0xfe, 0x13, 
0xe0, 0x07, 0xff, 0xff, 0xff, 0xff, 0xfe, 0x07, 0xf0, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0x07, 0xf8, 0x1f, 0xff, 0xff, 0xff, 0xff, 0xff, 0x8f };

const unsigned char probrain_icon[] = { // 32x24
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf8, 0x1f, 0xff, 0xff, 0xe0, 0x07, 0xff, 0xff, 0xc7, 0xe0, 0x7f, 
0xff, 0xdf, 0xf8, 0x3f, 0xff, 0x1f, 0xff, 0x9f, 0xfc, 0x3f, 0xff, 0xcf, 0xe0, 0x7f, 0xdf, 0xc7, 0xc0, 0xff, 0x9f, 0xc3, 0x9f, 0xff, 0x1f, 0xf9, 
0x3f, 0xff, 0x1f, 0xfc, 0x3f, 0xfe, 0x1f, 0xfc, 0x3f, 0xfb, 0x1f, 0xfc, 0x9f, 0xf3, 0x3b, 0xf9, 0xc0, 0x63, 0xf3, 0x03, 0xe0, 0x63, 0xe3, 0x87, 
0xff, 0xc3, 0xe3, 0xff, 0xff, 0xe7, 0xc3, 0xff, 0xff, 0xe7, 0xe3, 0xff, 0xff, 0xff, 0xe7, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

WiFiClient client; // wifi client object
#include "imglib/gridicons_align_right.h"

void setup() { 
  Serial.begin(115200);
  StartWiFi(ssid,password);
  
  display.init();
  //display.setRotation(3); // Right setup to get KEY1 on top. Funny to comment it and see how it works ;)
  display.setFont(&FreeSans18pt7b);
  
  if (skipLoadingScreen == false) {
    currentTime = obtain_time();
    display.fillScreen(GxEPD_BLACK); // No need to do this. Init cleans screen 
    display.setTextColor(GxEPD_WHITE);
    display.setCursor(0, 12);
    display.println("\n\r            FASAREK CORP\n\r");
    display.println("Time: "+currentTime);
    display.update();
  }
  display.setTextColor(GxEPD_BLACK);
 // obtain_forecast("forecast");
 // DisplayForecast();

  Serial.print("currentTime = "+currentTime);
  
// Start HTTP server
  server.onNotFound(handle_http_not_found);
  server.on("/", handle_http_root);  
  server.on("/display-write", handleDisplayWrite);
  server.on("/display-clean", handleDisplayClean);
  server.on("/deep-sleep", handleDeepSleep);
  server.on("/light-sleep", handleLightSleep);
  delay(2000);
  server.begin(); // not needed?
  // Moved to loop()
  //ESP.deepSleep(0); // ESP Wemos deep sleep. Wakes up and starts the complete sketch so it makes no sense to make a loop here
  }


void DisplayForecast(){ // Display is 264x176 resolution
  //display.fillScreen(GxEPD_WHITE);
  display.setTextColor(GxEPD_BLACK);
  display.setCursor(0,12);
  DisplayWXicon(14,15, Icon0);  DisplayWXicon(77,0, "thermo"); DisplayWXicon(139,0, "probrain");
  

  display.setCursor(176,12); display.println(Day0);
  display.setFont(NULL);
  display.setCursor(233,23); display.println(currentTime); // HH:mm
  display.setTextColor(GxEPD_BLACK);
  
  display.setCursor(75,42);  display.println(Conditions0);
  
  display.setCursor(50,40);   display.println(High0 + "/" + Low0);
  display.setCursor(105,40);  display.println(Averagehumidity0 + "%");
  display.setCursor(148,40);  display.println(Pop0 + "%");
 
  DisplayWXicon(13,76, Icon1); DisplayWXicon(75,60, "thermo"); DisplayWXicon(139,60, "probrain");
  display.setCursor(175,72);  display.println(Day1);
  display.setCursor(75,105);  display.println(Conditions1);
  
  display.setCursor(50,100);  display.println(High1 + "/" + Low1);
  display.setCursor(105,100); display.println(Averagehumidity1 + "%");
  display.setCursor(148,100); display.println(Pop1 + "%");
  
  DisplayWXicon(10,142, Icon2); DisplayWXicon(75,118, "thermo"); DisplayWXicon(139,118, "probrain");
  display.setCursor(175,132); display.println(Day2);
  
  display.setCursor(75,162); display.println(Conditions2);
  display.setCursor(50,157);  display.println(High2 + "/" + Low2);
  display.setCursor(105,157); display.println(Averagehumidity2 + "%");
  display.setCursor(148,157); display.println(Pop2 + "%"); 
  display.update(); 
}

/* Avialble symbols
 MostlyCloudy(x,y,scale)
 MostlySunny(x,y,scale)
 Rain(x,y,scale)
 Cloudy(x,y,scale)
 Sunny(x,y,scale)
 ExpectRain(x,y,scale)
 Tstorms(x,y,scale)
 Snow(x,y,scale)
 Fog(x,y,scale)
 Nodata(x,y,scale)
*/
 
void DisplayWXicon(int x, int y, String IconName){
  int scale = 10; // Adjust size as necessary
  Serial.println(IconName);
  if      (IconName == "rain"            || IconName == "nt_rain")             Rain(x,y, scale);
  else if (IconName == "chancerain"      || IconName == "nt_chancerain")       ExpectRain(x,y,scale);
  else if (IconName == "snow"            || IconName == "nt_snow"         ||
           IconName == "flurries"        || IconName == "nt_flurries"     ||
           IconName == "chancesnow"      || IconName == "nt_chancesnow"   ||
           IconName == "chanceflurries"  || IconName == "nt_chanceflurries")   Snow(x,y,scale);
  else if (IconName == "sleet"           || IconName == "nt_sleet"        ||
           IconName == "chancesleet"     || IconName == "nt_chancesleet")      Snow(x,y,scale);
  else if (IconName == "sunny"           || IconName == "nt_sunny"        ||
           IconName == "clear"           || IconName == "nt_clear")            Sunny(x,y,scale);
  else if (IconName == "partlysunny"     || IconName == "nt_partlysunny"  ||
           IconName == "mostlysunny"     || IconName == "nt_mostlysunny")      MostlySunny(x,y,scale);
  else if (IconName == "cloudy"          || IconName == "nt_cloudy"       ||
           IconName == "mostlycloudy"    || IconName == "nt_mostlycloudy" ||
           IconName == "partlycloudy"    || IconName == "nt_partlycloudy")     Cloudy(x,y,scale);           
  else if (IconName == "tstorms"         || IconName == "nt_tstorms"      ||
           IconName == "chancetstorms"   || IconName == "nt_chancetstorms")    Tstorms(x,y,scale);
  else if (IconName == "fog"             || IconName == "nt_fog"          ||
           IconName == "hazy"            || IconName == "nt_hazy")             Fog(x,y,scale);
  else if (IconName == "thermo")
           display.drawBitmap(x,y, thermo_icon,64,24, GxEPD_BLACK);
  else if (IconName == "probrain")
           display.drawBitmap(x,y, probrain_icon,32,24, GxEPD_BLACK);
  else     Nodata(x,y,scale);
}

String obtain_time() {
  String host = "slosarek.eu";
  String url = "/api/time.php";
    
  // Use WiFiClientSecure class if you need to create TLS connection
  //WiFiClient httpclient;
  
  String request;
  request  = "GET "+url+" HTTP/1.1\r\n";
  request += "Accept: */*\r\n";
  request += "Host: " +host+ "\r\n";
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
  while(client.available()){
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


void obtain_forecast (String forecast_type) {
  static char RxBuf[8704];
  String request;
  request  = "GET /api/" + API_key + "/"+ forecast_type + "/q/" + Country + "/" + City + ".json HTTP/1.1\r\n";
  request += F("User-Agent: Weather Webserver v");
  request += version;
  request += F("\r\n");
  request += F("Accept: */*\r\n");
  request += "Host: " + String(wxserver) + "\r\n";
  request += F("Connection: close\r\n");
  request += F("\r\n");
  Serial.println(request);
  Serial.print(F("Connecting to ")); Serial.println(wxserver);
  WiFiClient httpclient;
  if (!httpclient.connect(wxserver, 80)) {
    Serial.println(F("connection failed"));
    httpclient.flush();
    httpclient.stop();
    return;
  }
  httpclient.print(request); //send the http request to the server
  httpclient.flush();
  // Collect http response headers and content from Weather Underground, discarding HTTP headers, the content is JSON formatted and will be returned in RxBuf.
  uint16_t respLen = 0;
  bool     skip_headers = true;
  String   rx_line;
  while (httpclient.connected() || httpclient.available()) {
    if (skip_headers) {
      rx_line = httpclient.readStringUntil('\n'); //Serial.println(rx_line);
      if (rx_line.length() <= 1) { // a blank line denotes end of headers
        skip_headers = false;
      }
    }
    else {
      int bytesIn;
      bytesIn = httpclient.read((uint8_t *)&RxBuf[respLen], sizeof(RxBuf) - respLen);
      //Serial.print(F("bytesIn ")); Serial.println(bytesIn);
      if (bytesIn > 0) {
        respLen += bytesIn;
        if (respLen > sizeof(RxBuf)) respLen = sizeof(RxBuf);
      }
      else if (bytesIn < 0) {
        Serial.print(F("read error "));
        Serial.println(bytesIn);
      }
    }
    delay(1);
  }
  httpclient.stop();

  if (respLen >= sizeof(RxBuf)) {
    Serial.print(F("RxBuf overflow "));
    Serial.println(respLen);
    delay(1000);
    return;
  }
  RxBuf[respLen++] = '\0'; // Terminate the C string
  Serial.print(F("respLen ")); Serial.println(respLen); Serial.println(RxBuf);
  if (forecast_type == "forecast"){
    showWeather_forecast(RxBuf); 
  }
  if (forecast_type == "astronomy"){
    showWeather_astronomy(RxBuf); 
  }
}

bool showWeather_astronomy(char *json) {
  StaticJsonBuffer<1*1024> jsonBuffer;
  char *jsonstart = strchr(json, '{'); // Skip characters until first '{' found
  //Serial.print(F("jsonstart ")); Serial.println(jsonstart);
  if (jsonstart == NULL) {
    Serial.println(F("JSON data missing"));
    return false;
  }
  json = jsonstart;
  // Parse JSON
  JsonObject& root = jsonBuffer.parseObject(json);
  if (!root.success()) {
    Serial.println(F("jsonBuffer.parseObject() failed"));
    return false;
  }
  // Extract weather info from parsed JSON
  JsonObject& current = root["moon_phase"];
  String percentIlluminated = current["percentIlluminated"];
  String phaseofMoon = current["phaseofMoon"];
  int SRhour         = current["sunrise"]["hour"];
  int SRminute       = current["sunrise"]["minute"];
  int SShour         = current["sunset"]["hour"];
  int SSminute       = current["sunset"]["minute"];
  int MRhour         = current["moonrise"]["hour"];
  int MRminute       = current["moonrise"]["minute"];
  int MShour         = current["moonset"]["hour"];
  int MSminute       = current["moonset"]["minute"];
  Sunrise   = (SRhour<10?"0":"")+String(SRhour)+":"+(SRminute<10?"0":"")+String(SRminute);
  Sunset    = (SShour<10?"0":"")+String(SShour)+":"+(SSminute<10?"0":"")+String(SSminute);
  Moonrise  = (MRhour<10?"0":"")+String(MRhour)+":"+(MRminute<10?"0":"")+String(MRminute);
  Moonset   = (MShour<10?"0":"")+String(MShour)+":"+(MSminute<10?"0":"")+String(MSminute);
  Moonlight = percentIlluminated;
  DphaseofMoon = phaseofMoon;
  return true;
}

bool showWeather_forecast(char *json) {
  DynamicJsonBuffer jsonBuffer(8704);
  char *jsonstart = strchr(json, '{');
  Serial.print(F("jsonstart ")); Serial.println(jsonstart);
  if (jsonstart == NULL) {
    Serial.println(F("JSON data missing"));
    return false;
  }
  json = jsonstart;

  // Parse JSON
  JsonObject& root = jsonBuffer.parseObject(json);
  if (!root.success()) {
    Serial.println(F("jsonBuffer.parseObject() failed"));
    return false;
  }

  JsonObject& forecast = root["forecast"]["simpleforecast"];
  String wday0  = forecast["forecastday"][0]["date"]["weekday_short"];  WDay0 = wday0;
  int    day0   = forecast["forecastday"][0]["date"]["day"]; day0<10?(Day0="0"+String(day0)):(Day0=String(day0));
  String mon0   = forecast["forecastday"][0]["date"]["monthname_short"];
  String year0  = forecast["forecastday"][0]["date"]["year"];           Day0 += "-" + mon0 + "-" + year0.substring(2); 
  String icon0  = forecast["forecastday"][0]["icon"];                   Icon0 = icon0;
  String high0  = forecast["forecastday"][0]["high"]["celsius"];        High0 = high0;
  String low0   = forecast["forecastday"][0]["low"]["celsius"];         Low0  = low0;
  String conditions0 = forecast["forecastday"][0]["conditions"];        Conditions0  = conditions0;
  String pop0        = forecast["forecastday"][0]["pop"];               Pop0  = pop0;
  String averagehumidity0 = forecast["forecastday"][0]["avehumidity"];  Averagehumidity0 = averagehumidity0;

  String wday1  = forecast["forecastday"][1]["date"]["weekday_short"];  WDay1 = wday1;
  int    day1   = forecast["forecastday"][1]["date"]["day"]; day1<10?(Day1="0"+String(day1)):(Day1=String(day1));
  String mon1   = forecast["forecastday"][1]["date"]["monthname_short"];
  String year1  = forecast["forecastday"][1]["date"]["year"];           Day1 += "-" + mon1 + "-" + year1.substring(2); 
  String icon1  = forecast["forecastday"][1]["icon"];                   Icon1 = icon1;
  String high1  = forecast["forecastday"][1]["high"]["celsius"];        High1 = high1;
  String low1   = forecast["forecastday"][1]["low"]["celsius"];         Low1  = low1;
  String conditions1 = forecast["forecastday"][1]["conditions"];        Conditions1  = conditions1;
  String pop1   = forecast["forecastday"][1]["pop"];                    Pop1  = pop1;
  String averagehumidity1 = forecast["forecastday"][1]["avehumidity"];  Averagehumidity1 = averagehumidity1;
  
  String wday2  = forecast["forecastday"][2]["date"]["weekday_short"];  WDay2 = wday2;
  int    day2   = forecast["forecastday"][2]["date"]["day"]; day2<10?(Day2="0"+String(day2)):(Day2=String(day2));
  String mon2   = forecast["forecastday"][2]["date"]["monthname_short"];
  String year2  = forecast["forecastday"][2]["date"]["year"];           Day2 += "-" + mon2 + "-" + year2.substring(2); 
  String icon2  = forecast["forecastday"][2]["icon"];                   Icon2 = icon2;
  String high2  = forecast["forecastday"][2]["high"]["celsius"];        High2 = high2;
  String low2   = forecast["forecastday"][2]["low"]["celsius"];         Low2  = low2;
  String conditions2 = forecast["forecastday"][2]["conditions"];        Conditions2  = conditions2;
  String pop2   = forecast["forecastday"][2]["pop"];                    Pop2  = pop2;
  String averagehumidity2 = forecast["forecastday"][2]["avehumidity"];  Averagehumidity2 = averagehumidity2;

  String wday3  = forecast["forecastday"][3]["date"]["weekday_short"];  WDay3 = wday3;
  int    day3   = forecast["forecastday"][3]["date"]["day"]; day3<10?(Day3="0"+String(day3)):(Day3=String(day3));
  String mon3   = forecast["forecastday"][3]["date"]["monthname_short"];
  String year3  = forecast["forecastday"][3]["date"]["year"];           Day3 += "-" + mon3 + "-" + year3.substring(2); 
  String icon3  = forecast["forecastday"][3]["icon"];                   Icon3 = icon3;
  String high3  = forecast["forecastday"][3]["high"]["celsius"];        High3 = high3;
  String low3   = forecast["forecastday"][3]["low"]["celsius"];         Low3  = low3;
  String conditions3 = forecast["forecastday"][3]["conditions"];        Conditions3  = conditions3;
  String pop3   = forecast["forecastday"][3]["pop"];                    Pop3  = pop3;
  String averagehumidity3 = forecast["forecastday"][3]["avehumidity"];  Averagehumidity3 = averagehumidity3;

  return true;
}

int StartWiFi(const char* ssid, const char* password){
 int connAttempts = 0;
 Serial.println("\r\nConnecting to: "+String(ssid));
 WiFi.begin(ssid, password);
 while (WiFi.status() != WL_CONNECTED ) {
   delay(500); Serial.print(".");
   if(connAttempts > 30) {
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
    while(1) { 
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
  display.fillCircle(x-scale*3, y, scale, GxEPD_BLACK);                           // Left most circle
  display.fillCircle(x+scale*3, y, scale, GxEPD_BLACK);                           // Right most circle
  display.fillCircle(x-scale, y-scale, scale*1.4, GxEPD_BLACK);                  // left middle upper circle
  display.fillCircle(x+scale*1.5, y-scale*1.3, scale*1.75, GxEPD_BLACK);          // Right middle upper circle
  display.fillRect(x-scale*3, y-scale, scale*6, scale*2+1,GxEPD_BLACK);           // Upper and lower lines
  //Clear cloud inner
  display.fillCircle(x-scale*3, y, scale-linesize, GxEPD_WHITE);                  // Clear left most circle
  display.fillCircle(x+scale*3, y, scale-linesize, GxEPD_WHITE);                  // Clear right most circle
  display.fillCircle(x-scale, y-scale, scale*1.4-linesize, GxEPD_WHITE);         // left middle upper circle
  display.fillCircle(x+scale*1.5, y-scale*1.3, scale*1.75-linesize, GxEPD_WHITE); // Right middle upper circle
  display.fillRect(x-scale*3, y-scale+linesize, scale*6, scale*2-linesize*2+1, GxEPD_WHITE); // Upper and lower lines
}

void addrain(int x, int y, int scale){
  y = y + scale/2;
  for (int i = 0; i < 6; i++){
    display.drawLine(x-scale*4+scale*i*1.3+0, y+scale*1.9, x-scale*3.5+scale*i*1.3+0, y+scale,GxEPD_BLACK);
    display.drawLine(x-scale*4+scale*i*1.3+1, y+scale*1.9, x-scale*3.5+scale*i*1.3+1, y+scale,GxEPD_BLACK);
    display.drawLine(x-scale*4+scale*i*1.3+2, y+scale*1.9, x-scale*3.5+scale*i*1.3+2, y+scale,GxEPD_BLACK);
  }
}

void addsnow(int x, int y, int scale){
  int dxo, dyo, dxi, dyi;
  for (int flakes = 0; flakes < 5;flakes++){
    for (int i = 0; i <360; i = i + 45) {
      dxo = 0.5*scale * cos((i-90)*3.14/180); dxi = dxo*0.1;
      dyo = 0.5*scale * sin((i-90)*3.14/180); dyi = dyo*0.1;
      display.drawLine(dxo+x+0+flakes*1.5*scale-scale*3,dyo+y+scale*2,dxi+x+0+flakes*1.5*scale-scale*3,dyi+y+scale*2,GxEPD_BLACK); 
    }
  }
}

void addtstorm(int x, int y, int scale){
  y = y + scale/2;
  for (int i = 0; i < 5; i++){
    display.drawLine(x-scale*4+scale*i*1.5+0, y+scale*1.5, x-scale*3.5+scale*i*1.5+0, y+scale,GxEPD_BLACK);
    display.drawLine(x-scale*4+scale*i*1.5+1, y+scale*1.5, x-scale*3.5+scale*i*1.5+1, y+scale,GxEPD_BLACK);
    display.drawLine(x-scale*4+scale*i*1.5+2, y+scale*1.5, x-scale*3.5+scale*i*1.5+2, y+scale,GxEPD_BLACK);
    display.drawLine(x-scale*4+scale*i*1.5, y+scale*1.5+0, x-scale*3+scale*i*1.5+0, y+scale*1.5+0,GxEPD_BLACK);
    display.drawLine(x-scale*4+scale*i*1.5, y+scale*1.5+1, x-scale*3+scale*i*1.5+0, y+scale*1.5+1,GxEPD_BLACK);
    display.drawLine(x-scale*4+scale*i*1.5, y+scale*1.5+2, x-scale*3+scale*i*1.5+0, y+scale*1.5+2,GxEPD_BLACK);
    display.drawLine(x-scale*3.5+scale*i*1.4+0, y+scale*2.5, x-scale*3+scale*i*1.5+0, y+scale*1.5,GxEPD_BLACK);
    display.drawLine(x-scale*3.5+scale*i*1.4+1, y+scale*2.5, x-scale*3+scale*i*1.5+1, y+scale*1.5,GxEPD_BLACK);
    display.drawLine(x-scale*3.5+scale*i*1.4+2, y+scale*2.5, x-scale*3+scale*i*1.5+2, y+scale*1.5,GxEPD_BLACK);
  }
}

void addsun(int x, int y, int scale) {
  int linesize = 3;
  int dxo, dyo, dxi, dyi;
  display.fillCircle(x, y, scale,GxEPD_BLACK);
  display.fillCircle(x, y, scale-linesize,GxEPD_WHITE);
  for (float i = 0; i <360; i = i + 45) {
    dxo = 2.2*scale * cos((i-90)*3.14/180); dxi = dxo * 0.6;
    dyo = 2.2*scale * sin((i-90)*3.14/180); dyi = dyo * 0.6;
    if (i == 0   || i == 180) {
      display.drawLine(dxo+x-1,dyo+y,dxi+x-1,dyi+y,GxEPD_BLACK);
      display.drawLine(dxo+x+0,dyo+y,dxi+x+0,dyi+y,GxEPD_BLACK); 
      display.drawLine(dxo+x+1,dyo+y,dxi+x+1,dyi+y,GxEPD_BLACK);
    }
    if (i == 90  || i == 270) {
      display.drawLine(dxo+x,dyo+y-1,dxi+x,dyi+y-1,GxEPD_BLACK);
      display.drawLine(dxo+x,dyo+y+0,dxi+x,dyi+y+0,GxEPD_BLACK); 
      display.drawLine(dxo+x,dyo+y+1,dxi+x,dyi+y+1,GxEPD_BLACK); 
    }
    if (i == 45  || i == 135 || i == 225 || i == 315) {
      display.drawLine(dxo+x-1,dyo+y,dxi+x-1,dyi+y,GxEPD_BLACK);
      display.drawLine(dxo+x+0,dyo+y,dxi+x+0,dyi+y,GxEPD_BLACK); 
      display.drawLine(dxo+x+1,dyo+y,dxi+x+1,dyi+y,GxEPD_BLACK); 
    }
  }
}

void addfog(int x, int y, int scale){
  int linesize = 4;  
  for (int i = 0; i < 5; i++){
    display.fillRect(x-scale*3, y+scale*1.5, scale*6, linesize, GxEPD_BLACK); 
    display.fillRect(x-scale*3, y+scale*2,   scale*6, linesize, GxEPD_BLACK); 
    display.fillRect(x-scale*3, y+scale*2.5, scale*6, linesize, GxEPD_BLACK); 
  }
}

void MostlyCloudy(int x, int y, int scale){ 
  addsun(x-scale*1.8,y-scale*1.8,scale); 
  addcloud(x,y,scale); 
}

void MostlySunny(int x, int y, int scale){ 
  addcloud(x,y,scale); 
  addsun(x-scale*1.8,y-scale*1.8,scale); 
}
 
void Rain(int x, int y, int scale){ 
  addcloud(x,y,scale); 
  addrain(x,y,scale); 
} 

void Cloudy(int x, int y, int scale){
  addcloud(x,y,scale);
}

void Sunny(int x, int y, int scale){
  scale = scale * 1.5;
  addsun(x,y,scale);
}

void ExpectRain(int x, int y, int scale){
  addsun(x-scale*1.8,y-scale*1.8,scale); 
  addcloud(x,y,scale);
  addrain(x,y,scale);
}

void Tstorms(int x, int y, int scale){
  addcloud(x,y,scale);
  addtstorm(x,y,scale);
}

void Snow(int x, int y, int scale){
  addcloud(x,y,scale);
  addsnow(x,y,scale);
}

void Fog(int x, int y, int scale){
  addcloud(x,y,scale);
  addfog(x,y,scale);
}

void handle_http_not_found() {
  server.send(404, "text/plain", "Not Found");
}

void handle_http_root() {

  String headers = "<head><link rel=\"stylesheet\" href=\"https://maxcdn.bootstrapcdn.com/bootstrap/4.0.0/css/bootstrap.min.css\">";
  headers += "<meta name='viewport' content='width=device-width,initial-scale=1'></head>";
  String html = "<body><div class='container-fluid'><div class='row'>";
  html += "<div class='col-md-6'><h4>carlos.local</h4><br>";
  html += "<h5>Message to Display:</h5>";
  html += "<br><form action='/display-write' target='frame' method='POST'>";
  html += "<label for='title'><input id='title' name='title' class='form-control'><br>";
  html += "<textarea name='text' rows=6 class='form-control'></textarea>";
  html += "<input type='submit' value='Send to display' class='btn btn-success'><form><br>";
  html += "<a href='/display-clean' target='frame'>Clean screen</a><br>";
  html += "<a href='/light-sleep' target='frame'>Light sleep</a><br>";
  html += "<a href='/deep-sleep' target='frame'>Deep sleep</a>";
  html += "</div></div></div></body>";
  html += "<iframe name='frame'></iframe>";
  server.send(200, "text/html", headers + html);
}


void Nodata(int x, int y, int scale){
  if (scale == 10) display.setTextSize(3); else display.setTextSize(1);
  display.setCursor(x,y);
  display.println("?");
  display.setTextSize(1);
}

void handleDeepSleep() {
  ESP.deepSleep(20e6);
  server.send(200, "text/html", "Going to deep-sleep");
}

void handleLightSleep() {
  WiFi.forceSleepBegin(60000000);
  server.send(200, "text/html", "WIFI to light-sleep");
}

void handleDisplayClean() {
  display.fillScreen(GxEPD_WHITE);
  // Test if german characters don't come because of a web encoding issue
  // Characters are not there :( 
  display.setCursor(0,33);
  display.print("ä ö ü Ä Ö Ü €");
  display.update();
  server.send(200, "text/html", "Clearing display");
}
void handleDisplayWrite() {
  display.fillScreen(GxEPD_WHITE);
  
  // Analizo el POST iterando cada value
  if (server.args() > 0) {
    for (byte i = 0; i < server.args(); i++) {
      if (server.argName(i) == "title") {
        display.setFont(&FreeSans24pt7b);
        display.setCursor(0,33);
        display.print(server.arg(i));
      }
      if (server.argName(i) == "text") {
        display.setFont(&FreeSans18pt7b);
        display.setCursor(0,63);
        display.print(server.arg(i));
      }
    }
  }
  display.update();
  server.send(200, "text/html", "Text sent to display");
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

