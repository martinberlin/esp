/*
   Web to LCD (ESP8266 Only, no Arduino) by www.varind.com 2016. This code is public domain, enjoy!
   Latest code: https://github.com/varind/ESP8266-Web-To-LCD
   Project page: : http://www.variableindustries.com/web-to-lcd-2/

   LCD 1 VSS -> 200 ohm -> +5V
   LCD 2 GND -> GND
   LCD 3 VO Pin -> center pin of 10k Potentiometer (ends to +5V & GND)
   LCD 4 RS pin -> ESP-12  pin 13
   LCD 5 R/W pin -> GND
   LCD 6 Enable pin -> ESP-12 pin 12
   LCD 11 D4 pin -> ESP-12  pin 14
   LCD 12 D5 pin -> ESP-12  pin 5
   LCD 13 D6 pin -> ESP-12  pin 4
   LCD 14 D7 pin -> ESP-12  pin 2
   LCD 15 A (backlight) -> 220 ohm -> +5V
   LCD 16 K (backlight) -> GND

   ESP-12 VCC -> +3.3V
   ESP-12 GND -> GND
   ESP-12 pin CH_PD -> 10K -> +3.3V

   To connect the ESP8266 to your network:
   -Connect your computer to the Wireless AP defined in ESPssid (default: "ESP LCD")
   -Use password set in ESPpassword (default: "PICK A PASSWORD")
   -Use a web browser to go to what is set in apIP (default "192.168.0.1")
   -Click on "cofig wifi settings"
   -Select your home/office access point and enter your password

   To upload new binary to the ESP8266:
   -Connect your computer to the same Wireless AP as the ESP8266
   -Use a web browser and navigate to what is set to host + .local (default "esp.local")
   -Select the new binary to install
   -Click upload
   -Info on mDNS at: https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266mDNS
*/
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>
#include <EEPROM.h>


ESP8266WebServer server(80);
ESP8266HTTPUpdateServer httpUpdater;
IPAddress apIP(192, 168, 0, 2);
IPAddress netMsk(255, 255, 255, 0);

bool debug = true;

const char *ESPssid     = "espAP";
const char *ESPpassword = "123456789";
const char* host        = "esp";
byte lcdRows            = 4;
byte lcdCols            = 20;

/* Don't set these wifi credentials. They are configurated at runtime and stored on EEPROM */
char ssid[32] = "KabelBox-A210";
char password[32] = "14237187131701431551";



//#define BBCNEWS    // Does not work
//#define NYTNEWS  // Does not work
//#define ABCNEWS
#define WEATHER
//#define BIGWEATHER
//#define STOCK
//#define BIGSTOCK

//----------------------------------------------------------------------------------|
//                                  BBC NEWS
//----------------------------------------------------------------------------------|
#ifdef BBCNEWS // http://feeds.bbci.co.uk/news/rss.xml?edition=us
char dataServer[] = "feeds.bbci.co.uk";         // base URL of site to connect to
char dataPage[] = "/news/rss.xml?edition=us";   // specific paget to conenct to
const char *elements[]  = {"<title><![CDATA["};          // list of elements to search for
char *elementPreLabels[]  = {"BBC NEWS: "};     // labels to display before data
char *elementPostLabels[]  = {""};              // labels to display after data
const char dataEnd = '/';                       // character that marks the end of data
int normalStartBias = 0;                 // characters after element start that data begins
int normalEndBias = -4;                  // characters before end charater that data ends
const char elementCols[] = {1};                 // formatting for display
#define LONGDATA                                // use "longdata" formatting (useful for news)
#define INSTANCENUMBER 2                        // find this instance of the element on the page
#define SPECIALREPLACE                          // replace special characters
const byte maxDataLength = 80;                  // longest possible length of element data
char charBuf[150];                              // total length of elements plus extra for +IPD reprints
const long reloadDelay = 20000;                // time between page reloads
//#define CUSTOMNEWLINECHAR '\r'                // always looks for \n but we may want to find onother character
//#define BIGTEXT                               // use big font for data display
//#define BIGLABELS                             // use big font for data labels
//#define GRID                                  // align to grid
//#define CUSTOMBIAS                            // if certain data elements need custom start/end postitions
#ifdef CUSTOMBIAS
const char *customBiasElements[] = {""};        // the elements that need custom start/end positions
int customStartBias = 0;                 // characters after element start that data begins
int customEndBias = 0;                   // characters before end charater that data ends
#endif
#endif
//----------------------------------------------------------------------------------|
//                                  NEW YORK TIMES NEWS
//----------------------------------------------------------------------------------|
#ifdef NYTNEWS // http://rss.nytimes.com/services/xml/rss/nyt/HomePage.xml
char dataServer[] = "rss.nytimes.com";
char dataPage[] = "/services/xml/rss/nyt/HomePage.xml";
const char *elements[]  = {"<title>"};
char *elementPreLabels[]  = {"BREAKING: "};
char *elementPostLabels[]  = {""};
const char dataEnd = '/';
int normalStartBias = 0;
int normalEndBias = -1;
const char elementCols[] = {1};

#define LONGDATA
#define INSTANCENUMBER 3
#define SPECIALREPLACE
const byte maxDataLength = 100;
char charBuf[200];
const long reloadDelay = 120000;
#endif

//----------------------------------------------------------------------------------|
//                                  ABC NEWS
//----------------------------------------------------------------------------------|
#ifdef ABCNEWS // http://feeds.abcnews.com/abcnews/topstories
char dataServer[] = "feeds.abcnews.com";
char dataPage[] = "/abcnews/topstories";
const char *elements[]  = {"<title><![CDATA["};
char *elementPreLabels[]  = {"BREAKING: "};
char *elementPostLabels[]  = {""};
const char dataEnd = ']';
int normalStartBias = 0;
int normalEndBias = 0;
const char elementCols[] = {1};

#define LONGDATA
#define INSTANCENUMBER 1
#define SPECIALREPLACE
const byte maxDataLength = 80;
char charBuf[80];
const long reloadDelay = 120000;
#endif

//----------------------------------------------------------------------------------|
//                                  STOCKS
//----------------------------------------------------------------------------------|
#ifdef STOCK  // http://finance.google.com/finance/info?client=ig&q=MCHP
char dataServer[] = "finance.google.com";
char dataPage[] = "/finance/info?client=ig&q=MCHP";
const char *elements[]  = {"\"lt\"", "\"t\"", "\"l_cur\"", "\"c\"", "\"cp\""};
char *elementPreLabels[]  = {"", "", "$", "", ""};
char *elementPostLabels[]  = {"", ":", "", " ", "%"};
const char dataEnd = '\0';
int normalStartBias = 4;
int normalEndBias = -2;
const char elementCols[] = {1, 2, 2, 1};
const byte maxDataLength = 25;
char charBuf[50];
long reloadDelay = 60000;
#endif
//----------------------------------------------------------------------------------|
//                                  STOCKS (BIG FONT)
//----------------------------------------------------------------------------------|
#ifdef BIGSTOCK  // http://finance.google.com/finance/info?client=ig&q=MCHP
char dataServer[] = "finance.google.com";
char dataPage[] = "/finance/info?client=ig&q=MCHP";
const char *elements[]  = {"\"t\"", "\"l_cur\""};
char *elementPreLabels[]  = {"", ""};
char *elementPostLabels[]  = {"", ""};
const char dataEnd = '\n';
int normalStartBias = 4;
int normalEndBias = -1;
const char elementCols[] = {1, 2};
#define BIGTEXT
const byte maxDataLength = 8;
char charBuf[50];
long reloadDelay = 60000;
#endif
//----------------------------------------------------------------------------------|
//                                  WEATHER
//----------------------------------------------------------------------------------|
#ifdef WEATHER // http://w1.weather.gov/xml/current_obs/KPDX.xml"
char dataServer[] = "w1.weather.gov";
char dataPage[] = "/xml/current_obs/KPDX.xml";
const char *elements[]  = {"temp_f", "relative_humidity", "pressure_in", "wind_dir", "wind_mph", "<weather"};
char *elementPreLabels[]  = {"", " ", "Pressure: ", "", " @ ", ""};
char *elementPostLabels[]  = {"F ", "% Humidity", "in", "", "MPH", ""};
const char dataEnd = '/';
int normalStartBias = 1;
int normalEndBias = -1;
const char elementCols[] = {2, 1, 2, 1};
const byte maxDataLength = 21;
char charBuf[50];
long reloadDelay = 60000;
#endif
//----------------------------------------------------------------------------------|
//                                  WEATHER  (BIG FONT)
//----------------------------------------------------------------------------------|
#ifdef BIGWEATHER // http://w1.weather.gov/xml/current_obs/KPDX.xml"
char dataServer[] = "w1.weather.gov";
char dataPage[] = "/xml/current_obs/KPDX.xml";
const char *elements[]  = {"temp_f", "relative_humidity", "wind_mph"};
char *elementPreLabels[]  = {"", "", ""};
char *elementPostLabels[]  = {"F ", "%", "MPH"};
const char dataEnd = '/';
int normalStartBias = 1;
int normalEndBias = -1;
const char elementCols[] = {2, 1};
#define BIGTEXT
//#define BIGLABELS
const byte maxDataLength = 5;
char charBuf[20];
long reloadDelay = 60000;
#endif
//----------------------------------------------------------------------------------


String line;
byte memPos = 0; // Memory offset
const byte elementsArrayLength = sizeof(elements) / sizeof(*elements);
const byte colArrayLength = sizeof(elementCols) / sizeof(*elementCols);

char *elementValues[elementsArrayLength + 1];
byte char_x = 0, char_y = 0;
long timer = 0;
bool jumpStart = true;
bool clientConnected = false;

boolean connect;
long lastConnectTry = 0;
int status = WL_IDLE_STATUS;

#ifdef CUSTOMBIAS
const byte customElementsArrayLength = sizeof(customBiasElements) / sizeof(*customBiasElements);
#endif
#ifdef INSTANCENUMBER
byte dataInstance = 0;
#endif

void setup() {
  Serial.begin(115200);

  WiFi.softAPConfig(apIP, apIP, netMsk);
  WiFi.softAP(ESPssid, ESPpassword);
  delay(500); // Without delay I've seen the IP address blank

  /* Setup web pages: root, wifi config pages, and not found. */
  server.on("/", handleRoot);
  server.on("/wifi", handleWifi);
  server.on("/wifisave", handleWifiSave);
  server.on("/output", handleOutput);
  server.onNotFound ( handleNotFound );

  httpUpdater.setup(&server);
  server.begin(); // Web server start

  Serial.println(">> HTTP server started");
  loadCredentials(); // Load WLAN credentials from network
  connect = strlen(ssid) > 0; // Request WLAN connect if there is a SSID

}

void loop() {
  connectRequest();
  server.handleClient();
  getAndDisplay();
}

void getAndDisplay(){
    if (WiFi.status() == WL_CONNECTED) {
    if (timer + reloadDelay < millis() || jumpStart) {
      getData();
      if (clientConnected) {
        printDataToLCD();
      } else {
        Serial.print("CLIENT DISCONNECTED");
        delay(50);
      }
      timer = millis();
      jumpStart = false;
    }
  } else {
    Serial.print(" WIFI DISCONNECTED ");
    delay(50);
  }
}

void getData() {
  WiFiClient client;
  Serial.print(">> Connecting to ");
  Serial.println(dataServer);
  
  if (!client.connect(dataServer, 80)) {
    Serial.println(">> Connection Failed !");
    clientConnected = false;
    client.stop();
    return;
  } else {
    clientConnected = true;
  }

  String url = dataPage;

  Serial.print(">> Requesting URL: ");
  Serial.println(dataPage);

  Serial.println("");
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + dataServer + "\r\nUser-Agent: Mozilla/4.0\r\n" +
               "Connection: close\r\n\r\n");
  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      Serial.println(">> Client Timeout !");
      client.stop();
      return;
    }
  }

  memPos = 0;
  while (client.available()) {

#ifdef INSTANCENUMBER
    if (dataInstance == INSTANCENUMBER) {
      client.flush();
      dataInstance = 0;
      break;
    }
#endif

    line = client.readStringUntil('\n');

#ifdef SPECIALREPLACE
    line.replace("&#x2019;", "\'");                        //replace special characters
    line.replace("&#39;", "\'");
    line.replace("&apos;", "\'");
    line.replace("’", "\'");
    line.replace("‘", "\'");
    line.replace("&amp;", "&");
    line.replace("&quot;", "\"");
    line.replace("&gt;", ">");
    line.replace("&lt;", "<");
#endif

    int elementPos;
    int elementLength;
    int endElement;
    int startBias, endBias;

    Serial.println(line);

    for (byte i = 0; i < elementsArrayLength; i++) {
      elementLength = strlen (elements[i]);
      elementPos = line.indexOf(elements[i]);
      endElement = line.indexOf(dataEnd);

      if (elementPos > -1)  {                                       // Found the element!!!

#ifdef INSTANCENUMBER
        dataInstance++;
#endif

#ifdef CUSTOMBIAS                                  // Check for custom bias
        bool customFlag = false;
        for (byte k = 0; k < customElementsArrayLength; k++) {
          if (elements[i] == customBiasElements[k]) {
            startBias = customStartBias;
            endBias = customEndBias;
            customFlag = true;
            break;
          }
        }
        if (!customFlag) {
          startBias = normalStartBias;
          endBias = normalEndBias;
        }
#else
        startBias = normalStartBias;
        endBias = normalEndBias;
#endif
        int lineLength = line.length();
        if (endElement == -1) {
          if (debug)Serial.println(F(">> Can't find end element: Using line end"));
          int endPos = endElement + endBias;
          line = line.substring(elementPos + elementLength + startBias, lineLength + endBias);
        } else {
          line = line.substring(elementPos + elementLength + startBias, endElement + endBias);
        }
        if (line == "")line = "-";        // if no value, replace with a dash
        line.toCharArray(charBuf + memPos, maxDataLength);
        elementValues[i] = charBuf + memPos;
        memPos = memPos + line.length() + 1;

        if (debug) {
          Serial.print(">> FOUND ");
          Serial.print(elements[i]);
          Serial.print("  VALUE=");
          Serial.print(elementValues[i]);
          Serial.print("  LENGTH=");
          Serial.println(line.length());

          Serial.print(">> MEMORY BUF From ");
          Serial.print(memPos - line.length() - 1);
          Serial.print(" To ");
          Serial.println(memPos);
        }
      }
    }
    line = ""; // to flush the value.
  }
  Serial.println();
  Serial.println("closing connection");
  client.stop();
}



void printDataToLCD() {
  if (debug) Serial.println("<---------PRINT DATA TO LCD--------->");
  byte x = 0;
  byte y = 0;
  byte cols = 0;
  byte dataLength = 0;

#ifdef LONGDATA
  byte longDataLength = 0;
  char text[maxDataLength + 20];   // +20 for labels
  text[0] = 0;
#endif

  for (byte el = 0; el < elementsArrayLength; el++) {
    dataLength = 0;
    for (byte col = 0; col < elementCols[cols]; col++) {
      if (y == lcdRows) break;     // we are off the screen, just stop

      dataLength = dataLength + strlen (elementPreLabels[el]) + strlen (elementValues[el]) + strlen (elementPostLabels[el]);

#ifdef GRID
      
#endif


#ifdef LONGDATA
      longDataLength = longDataLength + dataLength;
      byte offset;
      byte charNum;
      strcat(text, elementPreLabels[el]);
      strcat(text, elementValues[el]);
      strcat(text, elementPostLabels[el]);

      if (el == elementsArrayLength - 1) {
        if (longDataLength > lcdCols * lcdRows) longDataLength = lcdCols * lcdRows + 3;
        offset = 0;
        for (byte rowNum = 0; rowNum < lcdRows; rowNum++) {
          
          charNum = rowNum * lcdCols + offset;
          while (charNum < ((rowNum + 1) *lcdCols) + offset) {
            if (charNum - offset == rowNum * lcdCols && charNum < longDataLength && text[charNum] == ' ') {
              charNum++;
              offset++;
              longDataLength--;
            }
            //if (charNum - offset >= longDataLength) lcd.write(254);
            // if (charNum - offset >= longDataLength) lcd.print("x");
            //else if (charNum - offset < longDataLength)lcd.print(text[charNum]);
            Serial.print(text[charNum]);
            charNum++;
          }
        }
      }
#endif

#ifdef BIGTEXT
      char_x = x; char_y = y;
#ifdef BIGLABELS
      printBigCharacters(elementPreLabels[el], char_x, char_y);
#endif

      char_x = char_x + strlen (elementPreLabels[el]);
#ifdef BIGLABELS
#else
      char_x = char_x + strlen (elementPostLabels[el]);
#endif
      x = x + char_x;
#endif



      if (el == elementsArrayLength - 1 || col == elementCols[cols] - 1) {


        x = 0;
        break;
      }
      el++;
    }
    cols++;
    y++;
#ifdef BIGTEXT
    y++;
#endif
  }
}

void MDNSSetup(){


  int timeout = 0;
  bool MDNSStatus = true;
  while (!MDNS.begin(host)) {
    
    Serial.println(">> Error setting up MDNS responder!");
    delay(1000);
    timeout++;
    if (timeout == 20) {
      Serial.println(">> Setting up MDNS failed!");

      MDNSStatus = false;
      delay(500);
      break;
    }
  }
  if (MDNSStatus){
  MDNS.addService("http", "tcp", 80);
  Serial.println();
  Serial.println(">> HTTPUpdateServer ready!");
  Serial.printf(">> Open http://%s.local/update in your browser\n", host);
  Serial.println("");
  }
}

void connectWifi() {
  Serial.println(">> Connecting as wifi client...");

  WiFi.disconnect();
  WiFi.begin ( ssid, password );
  int connRes = WiFi.waitForConnectResult();
  Serial.print (">> connRes: ");
  Serial.println (connRes);
}

void connectRequest() {
  if (connect) {
    Serial.println (">> Connect requested");
    connect = false;
    connectWifi();
    lastConnectTry = millis();
  }
  {
    int s = WiFi.status();
    if (s == 0 && millis() > (lastConnectTry + 60000) ) {
      /* If WLAN disconnected and idle try to connect */
      /* Don't set retry time too low as retry interfere the softAP operation */
      connect = true;
    }
    if (status != s) { // WLAN status change
      Serial.print (">> Status: ");
      Serial.println (s);
      status = s;
      if (s == WL_CONNECTED) {
        /* Just connected to WLAN */
        Serial.println("");
        Serial.print(">> Connected to ");
        Serial.println(ssid);
        Serial.print(">> IP address: ");
        Serial.println(WiFi.localIP());

        delay(3000);

        MDNSSetup();
        
      } else if (s == WL_NO_SSID_AVAIL) {
        WiFi.disconnect();
      }
    }
  }
}

/** Load WLAN credentials from EEPROM */
void loadCredentials() {
  EEPROM.begin(512);
  EEPROM.get(0, ssid);
  EEPROM.get(0 + sizeof(ssid), password);
  char ok[2 + 1];
  EEPROM.get(0 + sizeof(ssid) + sizeof(password), ok);
  EEPROM.end();
  if (String(ok) != String("OK")) {
    ssid[0] = 0;
    password[0] = 0;
  }


  Serial.println(">> Recovered credentials:");
  Serial.print(">> SSID: ");
  Serial.println(ssid);
  Serial.print(">> PASS: ");
  Serial.println(strlen(password) > 0 ? "********" : "<no password>");
}

/** Store WLAN credentials to EEPROM */
void saveCredentials() {
  EEPROM.begin(512);
  EEPROM.put(0, ssid);
  EEPROM.put(0 + sizeof(ssid), password);
  char ok[2 + 1] = "OK";
  EEPROM.put(0 + sizeof(ssid) + sizeof(password), ok);
  EEPROM.commit();
  EEPROM.end();
}


/** IP to String? */
String toStringIp(IPAddress ip) {
  String res = "";
  for (int i = 0; i < 3; i++) {
    res += String((ip >> (8 * i)) & 0xFF) + ".";
  }
  res += String(((ip >> 8 * 3)) & 0xFF);
  return res;
}





void handleOutput() {
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.
  server.sendContent(
    "<html><head><meta http-equiv=\"refresh\" content=\"30\" /></head><body>"
    "<h1>Varind LCD Output</h1>"
  );

  for (byte el = 0; el < elementsArrayLength; el++) {
    server.sendContent(String()
                       + "<table border=0><tr><td align=right>" + elementPreLabels[el]
                       + "</td><td>" + elementValues[el]
                       + "</td><td align=left>" + elementPostLabels[el] + "</td></tr>");
  }

  server.sendContent("</body></html>");
  server.client().stop();
}







/** Handle root or redirect to captive portal */
void handleRoot() {

  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.
  server.sendContent(
    "<html><head></head><body>"
    "<h1>Varind LCD</h1>"
  );
  if (server.client().localIP() == apIP) {
    server.sendContent(String("<p>You are connected through the soft AP: ") + ESPssid + "</p>");
  } else {
    server.sendContent(String("<p>You are connected through the wifi network: ") + ssid + "</p>");
  }
  server.sendContent(
    "<p>You may want to <a href='/wifi'>config the wifi connection</a>.</p>"
    "<p>Or <a href='/output'>view the output</a>.</p>"
    "</body></html>"
  );
  server.client().stop(); // Stop is needed because we sent no content length
}



/** Wifi config page handler */
void handleWifi() {
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.
  server.sendContent(
    "<html><head></head><body>"
    "<h1>Wifi config</h1>"
  );
  if (server.client().localIP() == apIP) {
    server.sendContent(String("<p>You are connected through the soft AP: ") + ESPssid + "</p>");
  } else {
    server.sendContent(String("<p>You are connected through the wifi network: ") + ssid + "</p>");
  }
  server.sendContent(
    "\r\n<br />"
    "<table><tr><th align='left'>SoftAP config</th></tr>"
  );
  server.sendContent(String() + "<tr><td>SSID " + String(ESPssid) + "</td></tr>");
  server.sendContent(String() + "<tr><td>IP " + toStringIp(WiFi.softAPIP()) + "</td></tr>");
  server.sendContent(
    "</table>"
    "\r\n<br />"
    "<table><tr><th align='left'>WLAN config</th></tr>"
  );
  server.sendContent(String() + "<tr><td>SSID " + String(ssid) + "</td></tr>");
  server.sendContent(String() + "<tr><td>IP " + toStringIp(WiFi.localIP()) + "</td></tr>");
  server.sendContent(
    "</table>"
    "\r\n<br />"
    "<table><tr><th align='left'>WLAN list (refresh if any missing)</th></tr>"
  );
  Serial.println(">> scan start");
  int n = WiFi.scanNetworks();
  Serial.println(">> scan done");
  if (n > 0) {
    for (int i = 0; i < n; i++) {
      server.sendContent(String() + "\r\n<tr><td>SSID " + WiFi.SSID(i) + String((WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : " *") + " (" + WiFi.RSSI(i) + ")</td></tr>");
    }
  } else {
    server.sendContent(String() + "<tr><td>No WLAN found</td></tr>");
  }
  server.sendContent(
    "</table>"
    "\r\n<br /><form method='POST' action='wifisave'><h4>Connect to network:</h4>"
    "<input type='text' placeholder='network' name='n'/>"
    "<br /><input type='password' placeholder='password' name='p'/>"
    "<br /><input type='submit' value='Connect/Disconnect'/></form>"
    "<p>You may want to <a href='/'>return to the home page</a>.</p>"
    "</body></html>"
  );
  server.client().stop(); // Stop is needed because we sent no content length
}

/** Handle the WLAN save form and redirect to WLAN config page again */
void handleWifiSave() {
  Serial.println(">> wifi save");
  server.arg("n").toCharArray(ssid, sizeof(ssid) - 1);
  server.arg("p").toCharArray(password, sizeof(password) - 1);
  server.sendHeader("Location", "wifi", true);
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.send ( 302, "text/plain", "");  // Empty content inhibits Content-length header so we have to close the socket ourselves.
  server.client().stop(); // Stop is needed because we sent no content length
  saveCredentials();
  connect = strlen(ssid) > 0; // Request WLAN connect with new credentials if there is a SSID
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += ( server.method() == HTTP_GET ) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for ( uint8_t i = 0; i < server.args(); i++ ) {
    message += " " + server.argName ( i ) + ": " + server.arg ( i ) + "\n";
  }
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.send ( 404, "text/plain", message );
}


//---------------------- Large LCD Characters ------------------------//  Originally by Michael Pilcher, modified by Ben Lipsey
//  http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1265696343


byte UB[8] = //1
{
  B11111,
  B11111,
  B11111,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000
};
byte RT[8] = //2
{
  B11100,
  B11110,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111
};
byte LL[8] = //3
{
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B01111,
  B00111
};
byte LB[8] = //4
{
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B11111,
  B11111,
  B11111
};
byte LR[8] = //5
{
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11110,
  B11100
};
byte UMB[8] =  //6
{
  B11111,
  B11111,
  B11111,
  B00000,
  B00000,
  B00000,
  B11111,
  B11111
};
byte LMB[8] =  //7
{
  B11111,
  B00000,
  B00000,
  B00000,
  B00000,
  B11111,
  B11111,
  B11111
};
byte LT[8] =  //8
{
  B00111,
  B01111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111
};



