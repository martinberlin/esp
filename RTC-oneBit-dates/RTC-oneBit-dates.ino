//
// OneBitDisplay library simple demo with RTC
//
// Demonstrates how to initialize and use a few functions of the library
// If your MCU has enough RAM, enable the backbuffer to see a demonstration
// of the speed difference between drawing directly on the display versus
// deferred rendering, followed by a "dump" of the memory to the display
//
#include <OneBitDisplay.h>
#include <DS3231.h>
// is required when using with nRF52840 based board for Serial port implementation
#include <Adafruit_TinyUSB.h>
/* Vectors belong to a C++ library
   called STL so we need to import
   it first. They are use here only 
   to save important dates */
#include <vector>
using namespace std;

// Struct to store important days (Birthdays and any other)
struct Day_alert {
   uint8_t month;
   uint8_t day;
   String note;
};
// FIX for arduino: Otherwise there is a silly error
namespace std {
  void __throw_length_error(char const*) {
    Serial.println("vector __throw_length_error");
  }
}
vector<Day_alert> date_vector;

DS3231 myRTC;

// Weekdays and months translatables. [0] is empty since day & month start on 1.
char weekday_t[][12] = { "", "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };

char month_t[][12] = { "", "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};


#define USE_BACKBUFFER
static uint8_t ucBackBuffer[1024];
// Use -1 for the Wire library default pins
// or specify the pin numbers to use with the Wire library or bit banging on any GPIO pins
// These are the pin numbers for the M5Stack Atom Grove port I2C (reversed SDA/SCL for straight through wiring)
#define SDA_PIN -1
#define SCL_PIN -1
// Set this to -1 to disable or the GPIO pin number connected to the reset
// line of your display if it requires an external reset
#define RESET_PIN -1
// let OneBitDisplay figure out the display address
#define OLED_ADDR -1
// don't rotate the display
#define FLIP180 0
// don't invert the display
#define INVERT 0
// Bit-Bang the I2C bus
#define USE_HW_I2C 1

// Change these if you're using a different OLED display
#define MY_OLED OLED_128x64
#define OLED_WIDTH 128
#define OLED_HEIGHT 64
OBDISP obd;

String vector_find(uint8_t day, uint8_t month) {
  String ret = "";

  for(uint16_t i = 0; i < date_vector.size(); i++) {
    if (date_vector[i].month == month && date_vector[i].day == day) {
      return date_vector[i].note;
    }
  }
  return ret;
}

void vector_add(const Day_alert & data) {
    date_vector.push_back(data);
}

void setup() {
  Serial.begin(115200);
  while ( !Serial ) delay(10);   // for nrf52840 with native usb
  
  // Make a list of some important dates
  Day_alert dayv;
  dayv.day   = 12;
  dayv.month = 7;
  dayv.note = "Feliz cumple JAVI!";
  vector_add(dayv);
  
  dayv.day   = 4;
  dayv.month = 4;
  dayv.note = "Feliz cumple Paulina";
  vector_add(dayv);
  
  dayv.day   = 26;
  dayv.month = 6;
  dayv.note = "Go to work Bitch";
  vector_add(dayv);
  
int rc;
// The I2C SDA/SCL pins set to -1 means to use the default Wire library
rc = obdI2CInit(&obd, MY_OLED, OLED_ADDR, FLIP180, INVERT, USE_HW_I2C, SDA_PIN, SCL_PIN, RESET_PIN, 800000L); // use standard I2C bus at 400Khz
  if (rc != OLED_NOT_FOUND)
  {
    char *msgs[] = {(char *)"SSD1306 @ 0x3C", (char *)"SSD1306 @ 0x3D",(char *)"SH1106 @ 0x3C",(char *)"SH1106 @ 0x3D"};
    obdFill(&obd, 0, 1);
    obdSetTextWrap(&obd, true);
    obdSetBackBuffer(&obd, ucBackBuffer);
  }
} /* setup() */

bool century = false;
bool h12Flag;
bool pmFlag;

void animation_close(uint8_t number) {
  Serial.printf("anim %d\n", number);
  uint16_t x = 0;
  uint16_t y = 0;
  obdFill(&obd, 0,1);
  switch (number) {
    case 0:
          for (x=0; x<OLED_WIDTH-1; x+=2)
        {
          obdDrawLine(&obd, x, 0, x, OLED_HEIGHT-1, 1, 1);
        }
    case 1:
      x = random(1000);
      for (uint16_t i=0; i<3000+x; i++)
      {
        x = random(OLED_WIDTH);
        y = random(OLED_HEIGHT);
        obdSetPixel(&obd, x, y, 1, 1);
      }
      break;
    case 2:
      for (x=0; x<OLED_WIDTH-1; x+=2)
        {
          obdDrawLine(&obd, x, 0, OLED_WIDTH-x, OLED_HEIGHT-1, 1, 1);
        }
        for (y=0; y<OLED_HEIGHT-1; y+=2)
        {
          obdDrawLine(&obd, OLED_WIDTH-1,y, 0,OLED_HEIGHT-1-y, 1, 1);
        }
     break;
         case 3:
        for (y=0; y<OLED_HEIGHT-1; y++)
        {
          obdDrawLine(&obd, 0, y, OLED_WIDTH-1,y, 1, 1);
          delay(10);
        }
     break;
     default:
        for (x=0; x<OLED_WIDTH-1; x+=2)
        {
          obdDrawLine(&obd, x, 0, x, OLED_HEIGHT-1, 1, 1);
          delay(20);
        }
        for (y=0; y<OLED_HEIGHT-1; y+=3)
        {
          obdDrawLine(&obd, 0, y, OLED_WIDTH-1,y, 1, 1);
          delay(15);
        }
  }
  
}

void loop() {
  // Won't need this for 80 years.
  uint8_t month = myRTC.getMonth(century);
  uint8_t day = myRTC.getDate();
  uint8_t day_of_week = myRTC.getDoW();
  uint8_t hour = myRTC.getHour(h12Flag, pmFlag);
  uint8_t minute = myRTC.getMinute();
  char clockhh[8];
  char clockmm[3];
  char day_number[3];

  char temperature[6];
  itoa(hour, clockhh, 10);
  itoa(minute, clockmm, 10);
  itoa(day, day_number, 10);

  itoa(myRTC.getTemperature(), temperature, 10);
  char celsius[4] = " C";
  strncat(temperature, celsius, 2);

  // Make HH & MM have an additional 0 if <10
   char minute_buffer[3];
   if (minute<10) {
      strlcpy(minute_buffer,    "0", sizeof(minute_buffer));
      strlcat(minute_buffer, clockmm, sizeof(minute_buffer));
   } else {
      strlcpy(minute_buffer, clockmm, sizeof(minute_buffer));
   } 

   
  // append ch to str
  char separator[2] = ":";
  strncat(clockhh, separator, 1);
  strncat(clockhh, minute_buffer, 2);

  String day_message = vector_find(day, month);
  Serial.printf("%s\n", day_message);
  
int i, x, y;
char szTemp[32];
unsigned long ms;

  obdFill(&obd, 0x0, 1);
  
  obdWriteString(&obd, 0,10,2, clockhh, FONT_16x32, 0, 1);
  //                      x  y  write "day month" 
  obdWriteString(&obd, 0, 10,30,(char *)day_number, FONT_8x8, 0, 1);
  obdWriteString(&obd, 0, 30,30,(char *)month_t[month], FONT_8x8, 0, 1);
  
  // Write special message if the day matches:
  obdWriteString(&obd, 0, 0,40,(char *)day_message.c_str(), FONT_8x8, 0, 1);
  
  delay(4000);
  
 // Pixel and line functions won't work without a back buffer
  animation_close(random(5));
  delay(400);

  obdWriteString(&obd, 0,10,3, "Temperatura:", FONT_8x8, 0, 1);
  obdWriteString(&obd, 0,30,17, temperature, FONT_16x32, 0, 1);
  delay(2000);

} /* loop() */
