// ArduCAM Mini demo (C)2017 Lee
// Web: http://www.ArduCAM.com
// This program is a demo of how to use most of the functions
// of the library with ArduCAM ESP8266 2MP/5MP camera.
// This demo was made for ArduCAM ESP8266 2MP/5MP Camera.
// It can take photo and send to the Web.
// It can take photo continuously as video streaming and send to the Web.
// The demo sketch will do the following tasks:
// 1. Set the camera to JPEG output mode.
// 2. if server.on("/capture", HTTP_GET, serverCapture),it can take photo and send to the Web.
// 3.if server.on("/stream", HTTP_GET, serverStream),it can take photo continuously as video
//streaming and send to the Web.

// This program requires the ArduCAM V4.0.0 (or later) library and ArduCAM ESP8266 2MP/5MP camera
// and use Arduino IDE 1.6.8 compiler or above
#include <WiFiManager.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <Wire.h>
#include <ArduCAM.h>
#include <SPI.h>
#include "memorysaver.h"
#include "Button2.h";

//This demo can only work on OV5642_MINI_5MP_PLUS platform.
#if !(defined (OV5642_MINI_5MP_PLUS))
#error Please select the hardware platform and camera module in the ../libraries/ArduCAM/memorysaver.h file
#endif

// UPLOAD Settings
  String host = "api.slosarek.eu";
  String url = "/camera-uploads/upload-caf.php";
  String start_request = "";
  String boundary = "XXXsecuritycamXXX";
  String end_request = "\n--"+boundary+"--\n";
  

// set GPIO16 as the slave select :
const int CS = 16;
// OV5640_2048x1536 120Kb
int jpegSize = OV5642_1280x960; 
// When timelapse is on will capture picture every N minutes
boolean captureTimeLapse;
boolean isStreaming = false;

const unsigned long timeLapse = 5 * 60 * 1000UL; // 5 minutes
static unsigned long lastTimeLapse;
// Outputs
Button2 buttonShutter = Button2(D3);
const int ledStatus = D4;

WiFiManager wm;
WiFiClient client;
// Default config mode Access point
const char* configModeAP = "CAM-autoconnect";
String message;
char* localDomain = "cam"; // mDNS: cam.local

// Makes a div id="m" containing response message to dissapear after 3 seconds
String javascriptFadeMessage = "<script>setTimeout(function(){document.getElementById('m').innerHTML='';},6000);</script>";


long full_length;
static const size_t bufferSize = 4096;
static uint8_t buffer[bufferSize] = {0xFF};
uint8_t temp = 0, temp_last = 0;
int i = 0;
bool is_header = false;

ESP8266WebServer server(80);

#if defined (OV5642_MINI_5MP_PLUS) || defined (OV5642_MINI_5MP) || defined (OV5642_MINI_5MP_BIT_ROTATION_FIXED) ||(defined (OV5642_CAM))
ArduCAM myCAM(OV5642, CS);
#endif


void setup() {
    // set the Chip Select and ledStatus as an output:
  pinMode(CS, OUTPUT);
  pinMode(ledStatus, OUTPUT);

  std::vector<const char *> menu = {"wifi","wifinoscan","info","sep","restart"};
  wm.setMenu(menu);

  wm.setMinimumSignalQuality(40);
   // Callbacks that need to be defined before autoconnect to send a message to display (config and save config)
  wm.setAPCallback(configModeCallback);
  wm.setSaveConfigCallback(saveConfigCallback);
  wm.setDebugOutput(true); 
  wm.autoConnect(configModeAP);

// Button events
 buttonShutter.setReleasedHandler(shutterReleased); // Takes picture
 buttonShutter.setDoubleClickHandler(shutterDoubleClick);
 buttonShutter.setLongClickHandler(shutterLongClick);
  
while (WiFi.status() != WL_CONNECTED) {
   delay(500);
   Serial.print(".");
}
  Serial.println("Connected. LocalIP: "+ String( WiFi.localIP() ) );
  
  uint8_t vid, pid;
  uint8_t temp;
#if defined(__SAM3X8E__)
  Wire1.begin();
#else
  Wire.begin();
#endif
  Serial.begin(115200);
  Serial.println(F("ArduCAM Start!"));

  // set the CS as an output:
  pinMode(CS, OUTPUT);

  // initialize SPI:
  SPI.begin();
  SPI.setFrequency(4000000); //4MHz

  //Check if the ArduCAM SPI bus is OK
  myCAM.write_reg(ARDUCHIP_TEST1, 0x55);
  temp = myCAM.read_reg(ARDUCHIP_TEST1);
  if (temp != 0x55) {
    Serial.println(F("SPI1 interface Error!"));
    while (1);
  }
  myCAM.clear_bit(ARDUCHIP_GPIO,GPIO_PWDN_MASK); //disable low power
  delay(100);
//Check if the camera module type is OV5642
  myCAM.wrSensorReg16_8(0xff, 0x01);
  myCAM.rdSensorReg16_8(OV5642_CHIPID_HIGH, &vid);
  myCAM.rdSensorReg16_8(OV5642_CHIPID_LOW, &pid);
   if((vid != 0x56) || (pid != 0x42))
   Serial.println("Can't find OV5642 module!");
   else
   Serial.println("OV5642 detected.");
   
   myCAM.set_format(JPEG);
   myCAM.InitCAM();
   myCAM.write_reg(ARDUCHIP_TIM, VSYNC_LEVEL_MASK);   //VSYNC is active HIGH
   myCAM.OV5642_set_JPEG_size(jpegSize);
delay(500);

   // Set up mDNS responder:
  // - first argument is the domain name, in this example
  //   the fully-qualified domain name is "esp8266.local"
  // - second argument is the IP address to advertise
  //   we send our IP address on the WiFi network
  if (!MDNS.begin(localDomain)) {
    Serial.println("Error setting up MDNS responder!");
    while(1) { 
      delay(1000);
    }
  }
  // Add service to MDNS-SD
  MDNS.addService("http", "tcp", 80);
  
  Serial.println("mDNS responder started");
  // Start the server
  server.on("/capture", HTTP_GET, serverCapture);
  server.on("/stream", HTTP_GET, serverStream);
  server.on("/timelapse/start", HTTP_GET, serverStartTimelapse);
  server.on("/timelapse/stop", HTTP_GET, serverStopTimelapse);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println(F("Server started"));

  lastTimeLapse = millis() + timeLapse;  // Initialize timeLapse
  }

void start_capture() {
  myCAM.clear_fifo_flag();
  myCAM.start_capture();
}


String camCapture(ArduCAM myCAM) {
  
  uint32_t len  = myCAM.read_fifo_length();
  
  if (len == 0 ){
    Serial.println(F("fifo_length = 0"));
    return "Could not read fifo (length is 0)";
  }
  myCAM.CS_LOW();
  myCAM.set_fifo_burst();

  if (client.connect(host, 80)) { 
    while(client.available()) {
      String line = client.readStringUntil('\r');
    }  // Empty wifi receive bufffer

  start_request = start_request + 
  "\n--"+boundary+"\n" + 
  "Content-Disposition: form-data; name=\"upload\"; filename=\"CAM.JPG\"\n" + 
  "Content-Transfer-Encoding: binary\n\n";
  
   
    Serial.println("start_request len: "+String(start_request.length()));
    Serial.println("end_request len: "+String(end_request.length()));
    Serial.println("camCapture fifo_length="+String(len));
    
    full_length = start_request.length() + len + end_request.length();
    Serial.print(full_length);

    Serial.println("POST "+url+" HTTP/1.1");
    Serial.println("Host: "+host);
    Serial.println("Content-Length: "+String(full_length)); Serial.println();
    client.println("POST "+url+" HTTP/1.1");
    client.println("Host: "+host);
    client.println("Content-Type: multipart/form-data; boundary="+boundary);
    client.print("Content-Length: "); client.println(full_length);
      client.println();
    client.print(start_request);

  // Read image data from Arducam mini and send away to internet
  static uint8_t buffer[bufferSize] = {0xFF};
  while (len) {
      size_t will_copy = (len < bufferSize) ? len : bufferSize;
      SPI.transferBytes(&buffer[0], &buffer[0], will_copy);
      if (!client.connected()) break;
      client.write(&buffer[0], will_copy);
      len -= will_copy;
      delay(1);
      }
  
     client.println(end_request);
     //client.flush();
     myCAM.CS_HIGH(); 

  bool   skip_headers = true;
  String rx_line;
  String response;
  
  // Read all the lines of the reply from server and print them to Serial
    int timeout = millis() + 5000;
  while (client.available() == 0) {
    if (timeout - millis() < 0) {
      Serial.println(">>> Client Timeout !");
      client.stop();
      return "Timeout of 5 seconds reached";
    }
  }
  while(client.available()) {
    rx_line = client.readStringUntil('\r');
    Serial.println( rx_line );
    if (rx_line.length() <= 1) { // a blank line denotes end of headers
        skip_headers = false;
      }
      // Collect http response
     if (!skip_headers) {
            response += rx_line;
     }
  }
  response.trim();
  Serial.println( " RESPONSE after headers: " );
  Serial.println( response );
  client.stop();
  return response;
  } else {
    Serial.println("ERROR: Could not connect to "+host);
    return "ERROR Could not connect to host";
  }
}


void serverCapture() {
  digitalWrite(ledStatus, HIGH);
  
  isStreaming = false;
  start_capture();
  Serial.println(F("CAM Capturing"));

  int total_time = 0;

  total_time = millis();
  while (!myCAM.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK));
  total_time = millis() - total_time;
  Serial.print(F("capture total_time used (in miliseconds):"));
  Serial.println(total_time, DEC);

  total_time = 0;

  Serial.println(F("CAM Capture Done."));
  total_time = millis();
  String imageUrl = camCapture(myCAM);
  total_time = millis() - total_time;
  Serial.print(F("send total_time used (in miliseconds):"));
  Serial.println(total_time, DEC);
  Serial.println(F("CAM send Done."));

  digitalWrite(ledStatus, LOW);
  server.send(200, "text/html", "<div id='m'>Photo taken! "+imageUrl+
              "<br><img src='"+imageUrl+"' width='400'></div>"+ javascriptFadeMessage);
}


void serverStream() {
  isStreaming = true;
  WiFiClient client = server.client();

  String response = "HTTP/1.1 200 OK\r\n";
  response += "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n\r\n";
  server.sendContent(response);

  while (isStreaming) {
    start_capture();
    while (!myCAM.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK));
    size_t len = myCAM.read_fifo_length();

    if (len == 0 ) //0 kb
    {
      Serial.println(F("Size is 0."));
      continue;
    }
    myCAM.CS_LOW();
    myCAM.set_fifo_burst();
    if (!client.connected()) {
      client.stop(); is_header = false; break;
    }
    response = "--frame\r\n";
    response += "Content-Type: image/jpeg\r\n\r\n";
    server.sendContent(response);
    
    while ( len-- )
    {
      temp_last = temp;
      temp =  SPI.transfer(0x00);

      //Read JPEG data from FIFO
      if ( (temp == 0xD9) && (temp_last == 0xFF) ) //If find the end ,break while,
      {
        buffer[i++] = temp;  //save the last  0XD9
        //Write the remain bytes in the buffer
        myCAM.CS_HIGH();;
        if (!client.connected()) {
          client.stop(); is_header = false; break;
        }
        client.write(&buffer[0], i);
        is_header = false;
        i = 0;
      }
      if (is_header == true)
      {
        //Write image data to buffer if not full
        if (i < bufferSize)
          buffer[i++] = temp;
        else
        {
          //Write bufferSize bytes image data to file
          myCAM.CS_HIGH();
          if (!client.connected()) {
            client.stop(); is_header = false; break;
          }
          client.write(&buffer[0], bufferSize);
          i = 0;
          buffer[i++] = temp;
          myCAM.CS_LOW();
          myCAM.set_fifo_burst();
        }
      }
      else if ((temp == 0xD8) & (temp_last == 0xFF))
      {
        is_header = true;
        buffer[i++] = temp_last;
        buffer[i++] = temp;
      }
    }
    if (!client.connected()) {
      client.stop(); is_header = false; break;
    }
  }
}

/**
 * This is the home page and also the page that comes when a 404 is generated
 */
void handleNotFound() {
  String headers = "<head><link rel=\"stylesheet\" href=\"https://maxcdn.bootstrapcdn.com/bootstrap/4.0.0/css/bootstrap.min.css\">";
  headers += "<meta name='viewport' content='width=device-width,initial-scale=1'></head>";
  String html = "<body><main role='main'><div class='container-fluid'><div class='row'>";
  html += "<div class='col-md-12'><h4>" + String(localDomain) + ".local</h4>";
  
  html += "<a href='/timelapse/start'>Start timelapse</a> &nbsp;&nbsp;&nbsp";
  html += "<a href='/timelapse/stop'>Stop timelapse</a>";
  
  html += "<form id='f2' action='/stream' target='frame' method='GET'>";
  html += "<input type='submit' value='Stream' class='btn btn-light'>";
  html += "</form>";
  
  html += "<form id='f1' action='/capture' target='frame' method='GET'>";
  html += "<div class='row'><div class='col-md-12'>";
  html += "<input type='submit' value='PHOTO' class='btn btn-lg btn-dark form-control'>";
  html += "</div></div></form>";
  
  html += "<iframe name='frame' width='800' height='400'></iframe>";
  html += "</div></div></div></main>";
  html += "</body>";
  server.send(200, "text/html", headers + html);

}

void serverStartTimelapse() {
    digitalWrite(ledStatus, HIGH);
    Serial.println("long click: Enable timelapse");
    captureTimeLapse = true;
    lastTimeLapse = millis() + timeLapse;
    server.send(200, "text/html", "<div id='m'>Start Timelapse</div>"+ javascriptFadeMessage);
}

void serverStopTimelapse() {
    digitalWrite(ledStatus, LOW);
    Serial.println("Disable timelapse");
    captureTimeLapse = false;
    server.send(200, "text/html", "<div id='m'>Stop Timelapse</div>"+ javascriptFadeMessage);
}


void loop() {
  server.handleClient();
  buttonShutter.loop();
  if (captureTimeLapse && millis() >= lastTimeLapse) {
    lastTimeLapse += timeLapse;
    serverCapture();
    Serial.println("Timelapse captured");
  }
}


void configModeCallback (WiFiManager *myWiFiManager) {
  digitalWrite(ledStatus, HIGH);
  message = "CAM can't get online. Entering config mode. Please connect to access point "+String(configModeAP);
  Serial.println(message);
  Serial.println(myWiFiManager->getConfigPortalSSID());
}

void saveConfigCallback() {
  message = "WiFi configuration saved. ";
  message += "On next restart will connect automatically. Display is online: ";
  message += "http://cam.local or http://"+WiFi.localIP().toString();
  Serial.println(message);
  Serial.println(WiFi.localIP().toString());
}

void shutterDoubleClick(Button2& btn) {
    digitalWrite(ledStatus, LOW);
    Serial.println("Disable timelapse");
    captureTimeLapse = false;
}
void shutterReleased(Button2& btn) {
    digitalWrite(ledStatus, LOW);
    Serial.println("Released");
    captureTimeLapse = false;
    serverCapture();
}
void shutterLongClick(Button2& btn) {
    serverStartTimelapse();
}
