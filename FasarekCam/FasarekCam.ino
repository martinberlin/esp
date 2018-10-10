// ArduCAM Mini demo (C)2017 Lee
// Web: http://www.ArduCAM.com
// This program is a demo of how to use most of the functions
// of the library with ArduCAM ESP8266 2MP camera.

// The demo sketch will do the following tasks:
// 1. Set the camera to JPEG output mode.
// 2. if server.on("/capture", HTTP_GET, serverCapture),it can take photo and send to the Web.
// 3.if server.on("/stream", HTTP_GET, serverStream),it can take photo continuously as video
//streaming and send to the Web.

// This program requires the ArduCAM V4.0.0 (or later) library and ArduCAM ESP8266 2MP/5MP camera
#include <EEPROM.h>
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

//Note on memorysaver selected:
// #define OV2640_MINI_2MP
// And on Step 2: #define OV2640_CAM
//This demo can only work on OV2640_MINI_2MP or ARDUCAM_SHIELD_V2 platform.
#if !(defined (OV2640_MINI_2MP))
#error Please select the hardware platform and camera module in the ../libraries/ArduCAM/memorysaver.h file
#endif
// set GPIO16 as the slave select :
const int CS = 16;
// OV2640_800x600 OV2640_1280x1024 -> last max resolution working OV2640_1600x1200 -> Max. resolution 2MP
int jpegSize = OV2640_1600x1200; 
// When timelapse is on will capture picture every N minutes
boolean captureTimeLapse;
boolean isStreaming = false;
static unsigned long lastTimeLapse;

// Settings that are saved in EPROM
String upload_host;
String upload_path;
String slave_cam_ip;
unsigned long timelapse; // Now set in WiFi Manager

//flag for saving data
bool shouldSaveConfig = false;

// Outputs
Button2 buttonShutter = Button2(D3);
const int ledStatus = D4;

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

// UPLOAD Settings
  String start_request = "";
  String boundary = "_cam_";
  String end_request = "\n--"+boundary+"--\n";
 
uint8_t temp = 0, temp_last = 0;
int i = 0;
bool is_header = false;

ESP8266WebServer server(80);

ArduCAM myCAM(OV2640, CS);

//Classes to set custom params
class IntParameter : public WiFiManagerParameter {
public:
    IntParameter(const char *id, const char *placeholder, long value, const uint8_t length = 10)
        : WiFiManagerParameter("") {
        init(id, placeholder, String(value).c_str(), length, "", WFM_LABEL_BEFORE);
    }

    long getValue() {
        return String(WiFiManagerParameter::getValue()).toInt();
    }
};
class StringParameter : public WiFiManagerParameter {
public:
    StringParameter(const char *id, const char *placeholder, String value, const uint8_t length = 150)
        : WiFiManagerParameter("") {
        init(id, placeholder, String(value).c_str(), length, "", WFM_LABEL_BEFORE);
    }

    String getValue() {
        return String(WiFiManagerParameter::getValue());
    }
};

struct Settings {
    int timelapse = 60;
    String upload_host = "api.slosarek.eu";
    String upload_path = "/camera-uploads/upload.php?f=Tests";
    String slave_cam_ip = "";
} settings;


void saveConfigCallback() {
  digitalWrite(ledStatus, HIGH);
  shouldSaveConfig = true;
  Serial.println("shouldSaveConfig SAVE");
  Serial.println(WiFi.localIP().toString());
}

void setup() {
  Serial.begin(115200);
  // Save test:
  //storeStruct(&settings, sizeof(settings));
  // set the CS as an output:
  pinMode(CS, OUTPUT);
  pinMode(ledStatus, OUTPUT);
  loadStruct(&settings, sizeof(settings));
  
  std::vector<const char *> menu = {"wifi","wifinoscan","info","sep","restart"};

  WiFiManager wm;
  wm.setSaveConfigCallback(saveConfigCallback);
  
  wm.setMenu(menu);

  // Add custom parameters to WiFi Manager: upload_host  upload_script  timelapse_seconds  slave_camera_ip
  // id/name placeholder/prompt default length

  IntParameter param_timelapse( "timelapse",         "Timelapse in secs",  settings.timelapse);
  StringParameter param_slave_cam_ip("slave_cam_ip", "Slave cam ip/ping", settings.slave_cam_ip);
  StringParameter param_upload_host("upload_host",   "API host for upload", settings.upload_host);
  StringParameter param_upload_path("upload_path",   "Path to API endoint", settings.upload_path);
  // Add the defined parameters to wm
  wm.addParameter(&param_timelapse);
  wm.addParameter(&param_slave_cam_ip);
  wm.addParameter(&param_upload_host);
  wm.addParameter(&param_upload_path);
  wm.setMinimumSignalQuality(40);
  wm.setAPCallback(configModeCallback);
  wm.setDebugOutput(true); 
  
  timelapse = param_timelapse.getValue() * 1000UL;
  slave_cam_ip = param_slave_cam_ip.getValue();
  upload_host = param_upload_host.getValue();
  upload_path = param_upload_path.getValue();
  settings.timelapse = param_timelapse.getValue();
  if (settings.slave_cam_ip != param_slave_cam_ip.getValue() ||
      settings.timelapse    != param_timelapse.getValue()) {
    shouldSaveConfig = true;
  }

  
  settings.slave_cam_ip = slave_cam_ip;
  settings.upload_host = upload_host;
  settings.upload_path = upload_path;

// SAVE Config to EEPROM
  if (shouldSaveConfig) {
     Serial.println("SAVE THE CONFIGURATION");
     storeStruct(&settings, sizeof(settings));
  }

  wm.autoConnect(configModeAP);
// Button events
 buttonShutter.setReleasedHandler(shutterReleased); // Takes picture
 buttonShutter.setLongClickHandler(shutterLongClick); // Starts timelapse
  
  uint8_t vid, pid;
  uint8_t temp;
#if defined(__SAM3X8E__)
  Wire1.begin();
#else
  Wire.begin();
#endif

  Serial.println("ArduCAM Start!");
  
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
#if defined (OV2640_MINI_2MP) || defined (OV2640_CAM)
  //Check if the camera module type is OV2640
  myCAM.wrSensorReg8_8(0xff, 0x01);
  myCAM.rdSensorReg8_8(OV2640_CHIPID_HIGH, &vid);
  myCAM.rdSensorReg8_8(OV2640_CHIPID_LOW, &pid);
  if ((vid != 0x26 ) && (( pid != 0x41 ) || ( pid != 0x42 )))
    Serial.println(F("Can't find OV2640 module!"));
  else
    Serial.println(F("OV2640 detected."));
#endif

  //Change to JPEG capture mode and initialize the OV2640 module
  myCAM.set_format(JPEG);
  myCAM.InitCAM();
  myCAM.OV2640_set_JPEG_size(jpegSize); 
  Serial.println("JPEG_Size:"+String(jpegSize));

  myCAM.clear_fifo_flag();

   // Set up mDNS responder:
  // - first argument is the domain name, in this example
  //   the fully-qualified domain name is "esp8266.local"
  // - second argument is the IP address to advertise
  //   we send our IP address on the WiFi network
  if (!MDNS.begin(localDomain)) {
    Serial.println("Error setting up MDNS responder!");
    while(1) { 
      delay(500);
    }
  }
  // Add service to MDNS-SD
  MDNS.addService("http", "tcp", 80);
  
  Serial.println("mDNS responder started");
  // Start the server
  server.on("/capture", HTTP_GET, serverCapture);
  server.on("/stream", HTTP_GET, serverStream);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println(F("Server started"));

  lastTimeLapse = millis() + timelapse;  // Initialize timelapse
  }

void start_capture() {
  myCAM.clear_fifo_flag();
  myCAM.start_capture();
}


String camCapture(ArduCAM myCAM) {
  
  uint32_t len  = myCAM.read_fifo_length();
  
  if (len == 0 ) //0 kb
  {
    Serial.println(F("fifo_length = 0"));
    return "Could not read fifo (length is 0)";
  }
  // Check if gpio 16 (Chip Select) is not taking a picture already
  if (digitalRead(CS) == LOW) {
    Serial.println("CS is LOW: Taking a picture already");
    return "Taking a picture already";
  }
  myCAM.CS_LOW();
  myCAM.set_fifo_burst();
  SPI.transfer(0xFF);

  if (client.connect(upload_host, 80)) { 
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

    Serial.println("POST "+upload_path+" HTTP/1.1");
    Serial.println("Host: "+upload_host);
    Serial.println("Content-Length: "+String(full_length)); Serial.println();
    client.println("POST "+upload_path+" HTTP/1.1");
    client.println("Host: "+upload_host);
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
    Serial.println("ERROR: Could not connect to "+upload_host);
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


void loop() {
  server.handleClient();
  buttonShutter.loop();
  if (captureTimeLapse && millis() >= lastTimeLapse) {
    lastTimeLapse += timelapse;
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

void shutterReleased(Button2& btn) {
    digitalWrite(ledStatus, LOW);
    Serial.println("Released");
    captureTimeLapse = false;
    serverCapture();
}
void shutterLongClick(Button2& btn) {
    digitalWrite(ledStatus, HIGH);
    Serial.println("long click: Enable timelapse");
    captureTimeLapse = true;
    lastTimeLapse = millis() + timelapse;
}

// Found this magic here: https://github.com/esp8266/Arduino/issues/1539
void storeStruct(void *data_source, size_t size)
{
  EEPROM.begin(size * 2);
  for(size_t i = 0; i < size; i++)
  {
    char data = ((char *)data_source)[i];
    EEPROM.write(i, data);
  }
  EEPROM.commit();
}

void loadStruct(void *data_dest, size_t size)
{
    EEPROM.begin(size * 2);
    for(size_t i = 0; i < size; i++)
    {
        char data = EEPROM.read(i);
        ((char *)data_dest)[i] = data;
    }
}
