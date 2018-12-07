/**
 * Simple demo to show md5 calculation
 * Testing MD5 and a medium sized JPEG
 * https://github.com/esp8266/Arduino/blob/master/cores/esp8266/md5.h
 */
#include "SPIFFS.h"
#include <MD5Builder.h>

MD5Builder _md5;

void setup() {
  Serial.begin(115200);

if(!SPIFFS.begin(true)){
        Serial.println("An Error has occurred while mounting SPIFFS");
        return;
   }
   String filename = "/1.jpg";
    File file = SPIFFS.open(filename, FILE_READ);

    if(!file){
        Serial.println("There was an error opening the file");
        return;
    }
    
 size_t fileSize = file.size();
 Serial.println("Reading "+filename+" from SPIFFS");  

  long startTime = millis();

_md5.begin();

int b = 0;
String content;
String ch;
int chunkSize = 1024;
int fileRemaining = fileSize;
uint8_t buf[chunkSize]; //char
 Serial.println("Size: "+String(fileSize)+" bytes");  
 
while (file.available()) {
  
  if (fileRemaining-chunkSize<0) {
    chunkSize = fileSize - (chunkSize*b);
  }
  fileRemaining-=chunkSize;
  file.read(buf, chunkSize);
  //Serial.println("In loop: "+String(b)+" chunkSize: "+String(chunkSize));
 
  
  _md5.add(buf, chunkSize);
  delay(0);
  b++;
}
_md5.calculate();

long elapsedTime = millis()-startTime;

  Serial.println("md5:");
  Serial.println( _md5.toString() );
Serial.println("Miliseconds taken:"+String(elapsedTime));
  
  //Serial.println("total bytes readed: "+String(b));
}
void loop() {
  // put your main code here, to run repeatedly:

}
