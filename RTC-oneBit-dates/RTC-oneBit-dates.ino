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
// Deepsleep library
#include <Adafruit_SleepyDog.h>
// Fonts
#include "Ubuntu_M24pt8b.h"
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
    //Serial.println("vector __throw_length_error");
  }
}
vector<Day_alert> date_vector;

DS3231 myRTC;

// Weekdays and months translatables. [0] is empty since day & month start on 1.
char weekday_t[][12] = { "", "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };

char month_t[][12] = { "", "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};

//#define ENABLE_SERIAL_DEBUG
#define RTC_ENABLE_PIN 10

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
ONE_BIT_DISPLAY display;

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
  #ifdef ENABLE_SERIAL_DEBUG
  Serial.begin(115200);
  while ( !Serial ) delay(10);   // for nrf52840 with native usb
  #endif
  
  Day_alert dayv;
  
  // Make a list of some important dates
  dayv.day   = 17;
  dayv.month = 6;
  dayv.note = "Feliz cumple Martin";
  vector_add(dayv);
  dayv.day   = 19;
  dayv.month = 6;
  dayv.note = "Feliz cumple Helena";
  vector_add(dayv);
  dayv.day   = 12;
  dayv.month = 7;
  dayv.note = "Feliz cumple JAVI";
  vector_add(dayv);
  dayv.day   = 18;
  dayv.month = 7;
  dayv.note = "Feliz cumple Carlos Fasani";
  vector_add(dayv);
  dayv.day   = 18;
  dayv.month = 1;
  dayv.note = "Feliz cumple Anabelli";
  vector_add(dayv);
  
  dayv.day   = 6;
  dayv.month = 2;
  dayv.note = "Feliz cumple MAMA";
  vector_add(dayv);
  dayv.day   = 4;
  dayv.month = 4;
  dayv.note = "Feliz cumple Paulina";
  vector_add(dayv);

  dayv.day   = 14;
  dayv.month = 9;
  dayv.note = "Feliz cumple Nel";
  vector_add(dayv);

  // - - - Fechas historicas
    dayv.day   = 1;
  dayv.month = 1;
  dayv.note = "A recuperarse de la puta resaca";
  vector_add(dayv);
    dayv.day   = 2;
  dayv.month = 9;
  dayv.note = "1945 - Fin de la 2da guerra mundial";
  vector_add(dayv);
  
  // - - - Stars, musicians, etc. 
    dayv.day   = 11;
  dayv.month = 8;
  dayv.note = "1959-Nace Gustavo Ceratti";
  vector_add(dayv);
    dayv.day   = 23;
  dayv.month = 10;
  dayv.note = "1951-Nace Charly Garcia";
  vector_add(dayv);
    dayv.day = 17;
  dayv.month = 5;
  dayv.note = "1953-Nace Luca Prodan en Roma";
  vector_add(dayv);
    dayv.day = 23;
  dayv.month = 1;
  dayv.note = "1950-Nace Luis Alberto Spinetta";
    dayv.day = 10;
  dayv.month = 3;
  dayv.note = "1950-Nace Norberto Napolitano (Pappo)";
  vector_add(dayv);
      dayv.day = 13;
  dayv.month = 3;
  dayv.note = "1963-Nace Fito Paez en Rosario";
  vector_add(dayv);
    dayv.day = 24;
  dayv.month = 7;
  dayv.note = "1964-Nace Vicentico (Fabulosos Cadillacs)";
  vector_add(dayv);
  
  // Internacionales
    dayv.day = 26;
  dayv.month = 7;
  dayv.note = "1943-Nace Mick Jagger en Dartford, UK";
  vector_add(dayv);
  
    dayv.day = 6;
  dayv.month = 2;
  dayv.note = "1945-Nace Robert Nesta Marley en Jamaica";
  vector_add(dayv);
    dayv.day = 6;
  dayv.month = 3;
  dayv.note = "1946-Nace David Gilmour en Cambridge";
  vector_add(dayv);
    dayv.day = 9;
  dayv.month = 10;
  dayv.note = "1940-Nace John Lennon en Liverpool";
  vector_add(dayv);
    dayv.day = 6;
  dayv.month = 1;
  dayv.note = "1946-Nace Syd Barret, fundador Pink Floyd";
  vector_add(dayv);
    dayv.day = 6;
  dayv.month = 9;
  dayv.note = "1943-Nace Roger Waters,fundador de Pink Floyd";
  vector_add(dayv);
    dayv.day = 6;
  dayv.month = 10;
  dayv.note = "1966-Nace Adolfo Fito Cabrales (Fito&Fitipaldis)";
  vector_add(dayv);
  
    dayv.day = 1; //11
  dayv.month = 7; //12
  dayv.note = "1890-Nace Carlos Gardel en Touluse, el Zorzal";
  vector_add(dayv);
  // -- END of dates
  pinMode(RTC_ENABLE_PIN, OUTPUT);    // sets the digital pin 10 as output (Feeds RTC)
  
} /* setup() */

bool century = false;
bool h12Flag;
bool pmFlag;

void animation_close(uint8_t number, uint8_t color = 1) {
  //Serial.printf("anim %d\n", number);
  uint16_t x = 0;
  uint16_t y = 0;

  switch (number) {
    case 0: // Close to the right
          for (x=0; x<OLED_WIDTH-1; x+=2)
        {
          display.drawLine(x, 0, x, OLED_HEIGHT-1, color);
          delay(6);
        }
    case 1: // Random pixels
      x = random(1000);
      for (uint16_t i=0; i<3000+x; i++)
      {
        x = random(OLED_WIDTH);
        y = random(OLED_HEIGHT);
        display.drawPixel(x, y, color);
        delay(1);
      }
      break;
    case 2: // Grid
      for (x=0; x<OLED_WIDTH-1; x+=2)
        {
          display.drawLine(x, 0, OLED_WIDTH-x, OLED_HEIGHT-1, color);
          delay(2);
        }
        for (y=0; y<OLED_HEIGHT-1; y+=2)
        {
          display.drawLine(OLED_WIDTH-1,y, 0,OLED_HEIGHT-1-y, color);
          delay(1);
        }
     break;
     case 3: // Close down window
        for (y=0; y<OLED_HEIGHT-1; y++)
        {
          display.drawLine(0, y, OLED_WIDTH-1,y, color);
          delay(10);
        }
     break;
     case 4:  // Circles
      for (uint16_t i=0; i<50; i++)
      {
        uint8_t r = random(20);
        x = r+random(OLED_WIDTH-r);
        y = r+random(OLED_HEIGHT-r);
        
        display.drawCircle(x, y, r, color);
        delay(1);
      }
      break;
     case 5: // Rainforest
      for (uint16_t xi=0; xi<OLED_WIDTH; xi++)
      {
        for (uint16_t d=1; d<random(10)+1; d++)
        {
          uint8_t drop = random(10);
          uint8_t yi = random(OLED_HEIGHT-drop);
          display.drawLine(xi, yi, xi, yi+drop, color);
          delay(4);
        }
      }
      break;
           case 6: // Flashes!
      for (uint16_t xi=0; xi<12; xi++)
      {
       display.fillScreen(OLED_WHITE);
       delay(5);
       display.fillScreen(OLED_BLACK);
       delay(15);
      }
      break;
     default: // Close up window
        for (y=OLED_HEIGHT; y>0; y--)
        {
          display.drawLine(0, y, OLED_WIDTH-1,y, color);
          delay(10);
        }
     break;
  }
  
}

uint8_t total_random_slides = 7;

void loop() {
  digitalWrite(RTC_ENABLE_PIN, HIGH);
  // Wait some millis so it does wake up
  //delay(100);
  // Start display
  display.I2Cbegin(MY_OLED);
  //
  // Here we're asking the library to allocate the backing buffer
  // If successful, the library will change the render flag to "RAM only" so that
  // all drawing occurs only to the internal buffer, and display() must be called
  // to see the changes. Without a backing buffer, the API will try to draw all
  // output directly to the display instead.
  //
  if (!display.allocBuffer()) {
    display.print("Alloc failed");
  }
  display.setTextWrap(true);
  display.fillScreen(OLED_BLACK);
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
  char separator[4] = ":";
  strncat(clockhh, separator, 3);
  strncat(clockhh, minute_buffer, 2);
  
  // Fills all with 0x0 (Black)
  display.fillScreen(OLED_BLACK);
  //display.setTextColor(1, OLED_BLACK);
  display.setFreeFont(&Ubuntu_M24pt8b);
  display.setCursor(0,33);
  display.print(clockhh);

  display.setFont(FONT_8x8);
  display.setCursor(10,44);
  display.print(day_number);
  display.setCursor(30,44);
  display.print(month_t[month]);
  display.display();
  delay(5000);

  String day_message = vector_find(day, month);
  if (day_message != "") {
    // Implemented a new method setTextWrap in the C++ api for obdSetTextWrap(&obd, true);
    animation_close(random(total_random_slides));
    display.fillRect(0,20,OLED_WIDTH,33, 0);
    display.setCursor(0,30);
    display.print(day_message.c_str());
    display.display();
    delay(4000);
  }

  display.fillScreen(OLED_BLACK);
  char t[20] = "Temperatura:";
  display.setCursor(20,0);
  display.print(t);
  display.setFreeFont(&Ubuntu_M24pt8b);
  display.setCursor(16,46);
  display.print(temperature);
  display.display();

  delay(3000);
  animation_close(random(total_random_slides-1), 0);
  
  display.fillScreen(OLED_BLACK);
  display.display();
  
  // Switch off RTC
  digitalWrite(RTC_ENABLE_PIN, LOW);

  int sleepMS = Watchdog.sleep(60000);
  #ifdef ENABLE_SERIAL_DEBUG
  Serial.printf("Go to sleep %d\n\n", sleepMS);
  #endif
} /* loop() */
