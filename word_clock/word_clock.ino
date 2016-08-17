#include "Wire.h"
#define DS3231_I2C_ADDRESS 0x68

const int upButtonPin = 2;
const int downButtonPin = 3;

unsigned char red, blue, green;

const int ShiftPWM_latchPin=4;
const bool ShiftPWM_invertOutputs = false;
const bool ShiftPWM_balanceLoad = false;

#include <ShiftPWM.h>

typedef enum{
  off,
  on
} led_state_t;

typedef enum{
  bright,
  dim
} light_state_t;

struct lights_t{
  light_state_t light_state;
  byte action_needed;
};

typedef enum{
  oclock,
  five_past,
  ten_past,
  quarter_past,
  twenty_past,
  twenty_five_past,
  half_past,
  twenty_five_to,
  twenty_to,
  quarter_to,
  ten_to,
  five_to,
  manual_update
} time_update_t;

#define was_low_no_add_time       B00000001
#define was_low_add_time          B00000010
#define already_added_time        B00000100
#define already_dimmed_lights     B00001000

byte upButtonState = 0;
byte downButtonState = 0;
long buttonCompareTime= 0;
lights_t lights = {bright, 1};
time_update_t time_update = manual_update; 

void rgbLedRainbow(unsigned long cycleTime, int rainbowWidth);
void rgbLedRainbowLoveYou(unsigned long cycleTime, int rainbowWidth);

unsigned char maxBrightness = 255;
unsigned char dimBrightness = 5;
unsigned char pwmFrequency = 75;
unsigned int numRegisters = 6;
unsigned int numOutputs = numRegisters*8;
unsigned int numRGBLeds = numRegisters*8/3;
unsigned int fadingMode = 0; 
unsigned long startTime = 0; 

byte second, lastSecond, minute, hour, tempHour, dayOfWeek, dayOfMonth, month, year;

void setup(){
  while(!Serial){
    delay(100); 
  }
  Wire.begin();
  Serial.begin(9600);

  pinMode(upButtonPin, INPUT);
  pinMode(downButtonPin, INPUT);
  
  ShiftPWM.SetAmountOfRegisters(numRegisters);
  ShiftPWM.SetPinGrouping(1); 
  ShiftPWM.Start(pwmFrequency,maxBrightness);
  ShiftPWM.SetAll(0);
  
  // DS3231 seconds, minutes, hours, day, date, month, year
  setDS3231time(30,59,7,2,15,8,16);
  readDS3231time(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);
  lastSecond = second;
  if((hour >=8) && (hour <= 20))
  {
    lights.light_state = bright;
  }
  else lights.light_state = dim;

}

void loop()
{    
  if(lastSecond != second)
  {
    displayTime();
    lastSecond = second;
  }
  parse_time();
  buttonCompareTime = millis();
  while(digitalRead(upButtonPin) == LOW)
  {
    if((millis() - buttonCompareTime) >= 3000)
    {
      if(lights.action_needed == 1)
        if(lights.light_state == bright)
        {
          lights.light_state = dim;
          lights.action_needed = 0;
          upButtonState |= was_low_no_add_time;
          ShiftPWM.SetAll(0);      
        }
        else if(lights.light_state == dim)
        {
          lights.light_state = bright;
          lights.action_needed = 0;
          upButtonState |= was_low_no_add_time;
          ShiftPWM.SetAll(0); 
        }
        time_update = manual_update;
    }
    upButtonState |= was_low_add_time;
    if(digitalRead(downButtonPin) == LOW)
    {
      upButtonState |= was_low_no_add_time;
      ShiftPWM.SetAll(0);
      while(digitalRead(downButtonPin) == LOW)
      {
        if(digitalRead(upButtonPin) == LOW)
        {
          ShiftPWM.SetAll(0);
          set_light("i", on);
          rgbLedRainbowLoveYou(10000,numRGBLeds);
        }
        else
        {
          time_update = manual_update;
          parse_time();
        }
      }
      buttonCompareTime = millis();
      time_update = manual_update;
      parse_time();
    }
  }
  if(upButtonState == was_low_add_time)
    {
      upButtonState |= already_added_time;
      readDS3231time(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);
      minute += 5;
      if(minute >= 60)
      {
        minute -= 60;
        if(hour == 23) 
        {
          hour = 0;
          dayOfMonth += 1;
          if(dayOfWeek == 7) dayOfWeek = 1;
          else dayOfWeek += 1;
        }
        else hour += 1;
      }
      setDS3231time(second, minute, hour, dayOfWeek, dayOfMonth, month, year);
      parse_time();
    }

  while(digitalRead(downButtonPin) == LOW)
  {
    downButtonState |= was_low_add_time;
    if(digitalRead(upButtonPin) == LOW)
    {
      downButtonState |= was_low_no_add_time;
      ShiftPWM.SetAll(0);
      while(digitalRead(upButtonPin) == LOW)
      {
        if(digitalRead(downButtonPin) == LOW)
        {
          ShiftPWM.SetAll(0);
          set_light("i", on);
          set_light("love (red)", on);
          set_light("you (red)", on);
        }
        else
        {
          time_update = manual_update;
          parse_time();
        }
      }
      time_update = manual_update;
      parse_time();
    }
  }
  if(downButtonState == was_low_add_time)
    {
      downButtonState |= already_added_time;
      readDS3231time(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);
      if(minute <=4)
      {
        minute += 60;
        if(hour == 0)
        {
          hour = 23;
          dayOfMonth -= 1;
          if(dayOfWeek == 1) dayOfWeek = 7;
          else dayOfWeek -= 1;
        }
        else hour -= 1;
      }
      
      minute -= 5;
      setDS3231time(second, minute, hour, dayOfWeek, dayOfMonth, month, year);
      parse_time();
    }

    downButtonState = 0;
    upButtonState = 0;
    lights.action_needed = 1;

    if((month == 12) && (dayOfMonth == 10))
    {
      rgbLedRainbow(5000, numRGBLeds);
    }

    if((hour == 8) && (minute == 0) && (lights.light_state != bright))
    {
      Serial.println("flag1");
      time_update = manual_update;
      lights.light_state = bright;
    }

    if((hour == 20) && (minute == 0) && lights.light_state != dim)
    {
      Serial.println("flag2");
      time_update = manual_update;
      lights.light_state = dim;
    }
}

void rgbLedRainbow(unsigned long cycleTime, int rainbowWidth){
  // Displays a rainbow spread over a few LED's (numRGBLeds), which shifts in hue. 
  // The rainbow can be wider then the real number of LED's.
  unsigned long time = millis()-startTime;
  unsigned long colorShift = (360*time/cycleTime)%360; // this color shift is like the hue slider in Photoshop.

  for(unsigned int led=0;led<3;led++){ // loop over all LED's
    int hue = ((led)*360/(rainbowWidth-1)+colorShift)%360; // Set hue from 0 to 360 from first to last led and shift the hue
    hsvtorgb(&red, &green, &blue, hue, maxBrightness, maxBrightness);
    switch(led){
      case 0: ShiftPWM.SetOne(12, red); // write the HSV values, with saturation and value at maximum
              ShiftPWM.SetOne(13, green); // write the HSV values, with saturation and value at maximum
              ShiftPWM.SetOne(14, blue); // write the HSV values, with saturation and value at maximum
              break;
      case 1: ShiftPWM.SetOne(17, red); // write the HSV values, with saturation and value at maximum
              ShiftPWM.SetOne(18, green); // write the HSV values, with saturation and value at maximum
              ShiftPWM.SetOne(19, blue); // write the HSV values, with saturation and value at maximum
              ShiftPWM.SetOne(20, red); // write the HSV values, with saturation and value at maximum
              ShiftPWM.SetOne(21, green); // write the HSV values, with saturation and value at maximum
              ShiftPWM.SetOne(22, blue); // write the HSV values, with saturation and value at maximum
              break;
      case 2: ShiftPWM.SetOne(24, red); // write the HSV values, with saturation and value at maximum
              ShiftPWM.SetOne(25, green); // write the HSV values, with saturation and value at maximum
              ShiftPWM.SetOne(26, blue); // write the HSV values, with saturation and value at maximum
              break;
    }
  }
}

void rgbLedRainbowLoveYou(unsigned long cycleTime, int rainbowWidth){
  // Displays a rainbow spread over a few LED's (numRGBLeds), which shifts in hue. 
  // The rainbow can be wider then the real number of LED's.
  unsigned long time = millis()-startTime;
  unsigned long colorShift = (360*time/cycleTime)%360; // this color shift is like the hue slider in Photoshop.

  for(unsigned int led=0;led<2;led++){ // loop over all LED's
    int hue = ((led)*360/(rainbowWidth-1)+colorShift)%360; // Set hue from 0 to 360 from first to last led and shift the hue
    hsvtorgb(&red, &green, &blue, hue, maxBrightness, maxBrightness);
    switch(led){
      case 0: ShiftPWM.SetOne(37, red); // write the HSV values, with saturation and value at maximum
              ShiftPWM.SetOne(38, green); // write the HSV values, with saturation and value at maximum
              ShiftPWM.SetOne(39, blue); // write the HSV values, with saturation and value at maximum
              break;
      case 1: ShiftPWM.SetOne(40, red); // write the HSV values, with saturation and value at maximum
              ShiftPWM.SetOne(41, green); // write the HSV values, with saturation and value at maximum
              ShiftPWM.SetOne(42, blue); // write the HSV values, with saturation and value at maximum
              break;
    }
  }
}

byte decToBcd(byte val)
{
  return( (val/10*16) + (val%10) );
}

// Convert binary coded decimal to normal decimal numbers
byte bcdToDec(byte val)
{
  return( (val/16*10) + (val%16) );
}

void setDS3231time(byte second, byte minute, byte hour, byte dayOfWeek, byte
dayOfMonth, byte month, byte year)
{
  // sets time and date data to DS3231
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(0); // set next input to start at the seconds register
  Wire.write(decToBcd(second)); // set seconds
  Wire.write(decToBcd(minute)); // set minutes
  Wire.write(decToBcd(hour)); // set hours
  Wire.write(decToBcd(dayOfWeek)); // set day of week (1=Sunday, 7=Saturday)
  Wire.write(decToBcd(dayOfMonth)); // set date (1 to 31)
  Wire.write(decToBcd(month)); // set month
  Wire.write(decToBcd(year)); // set year (0 to 99)
  Wire.endTransmission();
}

void readDS3231time(byte *second, byte *minute, byte *hour, byte *dayOfWeek, byte *dayOfMonth, byte *month, byte *year)
{
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(0); // set DS3231 register pointer to 00h
  Wire.endTransmission();
  Wire.requestFrom(DS3231_I2C_ADDRESS, 7);
  // request seven bytes of data from DS3231 starting from register 00h
  *second = bcdToDec(Wire.read() & 0x7f);
  *minute = bcdToDec(Wire.read());
  *hour = bcdToDec(Wire.read() & 0x3f);
  *dayOfWeek = bcdToDec(Wire.read());
  *dayOfMonth = bcdToDec(Wire.read());
  *month = bcdToDec(Wire.read());
  *year = bcdToDec(Wire.read());
}

void displayTime()
{
  byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;
  readDS3231time(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);
  Serial.print(hour, DEC);
  Serial.print(":");
  if (minute<10) Serial.print("0");
  Serial.print(minute, DEC);
  Serial.print(":");
  if (second<10) Serial.print("0");
  Serial.print(second, DEC);
  Serial.print(" ");
  Serial.print(dayOfMonth, DEC);
  Serial.print("/");
  Serial.print(month, DEC);
  Serial.print("/");
  Serial.print(year, DEC);
  Serial.print(" Day of week: ");
  switch(dayOfWeek){
  case 1:
    Serial.println("Sunday");
    break;
  case 2:
    Serial.println("Monday");
    break;
  case 3:
    Serial.println("Tuesday");
    break;
  case 4:
    Serial.println("Wednesday");
    break;
  case 5:
    Serial.println("Thursday");
    break;
  case 6:
    Serial.println("Friday");
    break;
  case 7:
    Serial.println("Saturday");
    break;
  }
}

void set_light(char const* text, led_state_t state)
{
    if(state == on)
    {
      if(lights.light_state == bright)
      {
        if(text == "i") ShiftPWM.SetOne(0, maxBrightness);
        else if(text == "t") ShiftPWM.SetOne(1, maxBrightness);
        else if(text == "it")
        {
            ShiftPWM.SetOne(0, maxBrightness);
            ShiftPWM.SetOne(1, maxBrightness);      
        }
        else if(text == "is") ShiftPWM.SetOne(2, maxBrightness);
        else if(text == "ten") ShiftPWM.SetOne(3, maxBrightness);
        else if(text == "half") ShiftPWM.SetOne(4, maxBrightness);
        else if(text == "quarter") 
        {
            ShiftPWM.SetOne(5, maxBrightness);
            ShiftPWM.SetOne(6, maxBrightness);
        }
        else if(text == "twenty")
        {
            ShiftPWM.SetOne(7, maxBrightness);
            ShiftPWM.SetOne(8, maxBrightness);
        }
        else if(text == "five") ShiftPWM.SetOne(9, maxBrightness);
        else if(text == "minutes")
        {
            ShiftPWM.SetOne(10, maxBrightness);
            ShiftPWM.SetOne(11, maxBrightness);
        }
        else if(text == "happy (red)") ShiftPWM.SetOne(12, maxBrightness);
        else if(text == "happy (green)") ShiftPWM.SetOne(13, maxBrightness);
        else if(text == "happy (blue)") ShiftPWM.SetOne(14, maxBrightness);
        else if(text == "past") ShiftPWM.SetOne(15, maxBrightness);
        else if(text == "to") ShiftPWM.SetOne(16, maxBrightness);
        else if(text == "birthday (red)")
        {
            ShiftPWM.SetOne(17, maxBrightness);
            ShiftPWM.SetOne(20, maxBrightness);
        }
        else if(text == "birthday (green)") 
        {
            ShiftPWM.SetOne(18, maxBrightness);
            ShiftPWM.SetOne(21, maxBrightness);
        }
        else if(text == "birthday (blue)")
        {
            ShiftPWM.SetOne(19, maxBrightness);
            ShiftPWM.SetOne(22, maxBrightness);
        }
        else if(text == "three") ShiftPWM.SetOne(23, maxBrightness);
        else if(text == "lexie (red)") ShiftPWM.SetOne(24, maxBrightness);
        else if(text == "lexie (green)") ShiftPWM.SetOne(25, maxBrightness);
        else if(text == "lexie (blue)") ShiftPWM.SetOne(26, maxBrightness);
        else if(text == "eight") ShiftPWM.SetOne(27, maxBrightness);
        else if(text == "one") ShiftPWM.SetOne(28, maxBrightness);
        else if(text == "two") ShiftPWM.SetOne(29, maxBrightness);
        else if(text == "four") ShiftPWM.SetOne(30, maxBrightness);
        else if(text == "eleven")
        {
            ShiftPWM.SetOne(31, maxBrightness);
            ShiftPWM.SetOne(32, maxBrightness);
        }
        else if(text == "nine") ShiftPWM.SetOne(33, maxBrightness);
        else if(text == "seven") ShiftPWM.SetOne(34, maxBrightness);
        else if(text == "five (hour)") ShiftPWM.SetOne(35, maxBrightness);
        else if(text == "six") ShiftPWM.SetOne(36, maxBrightness);
        else if(text == "love (red)") ShiftPWM.SetOne(37, maxBrightness);
        else if(text == "love (green)") ShiftPWM.SetOne(38, maxBrightness);
        else if(text == "love (blue)") ShiftPWM.SetOne(39, maxBrightness);
        else if(text == "you (red)") ShiftPWM.SetOne(40, maxBrightness);
        else if(text == "you (green)") ShiftPWM.SetOne(41, maxBrightness);
        else if(text == "you (blue)") ShiftPWM.SetOne(42, maxBrightness);
        else if(text == "ten (hour)") ShiftPWM.SetOne(43, maxBrightness);
        else if(text == "twelve")
        {
            ShiftPWM.SetOne(44, maxBrightness);
            ShiftPWM.SetOne(45, maxBrightness);
        }
        else if(text == "oclock") 
        {
            ShiftPWM.SetOne(46, maxBrightness);
            ShiftPWM.SetOne(47, maxBrightness);
        } 
      } 
      else if(lights.light_state == dim)
      {
        if(text == "i") ShiftPWM.SetOne(0, dimBrightness);
        else if(text == "t") ShiftPWM.SetOne(1, dimBrightness);
        else if(text == "it")
        {
            ShiftPWM.SetOne(0, dimBrightness);
            ShiftPWM.SetOne(1, dimBrightness);      
        }
        else if(text == "is") ShiftPWM.SetOne(2, dimBrightness);
        else if(text == "ten") ShiftPWM.SetOne(3, dimBrightness);
        else if(text == "half") ShiftPWM.SetOne(4, dimBrightness);
        else if(text == "quarter") 
        {
            ShiftPWM.SetOne(5, dimBrightness);
            ShiftPWM.SetOne(6, dimBrightness);
        }
        else if(text == "twenty")
        {
            ShiftPWM.SetOne(7, dimBrightness);
            ShiftPWM.SetOne(8, dimBrightness);
        }
        else if(text == "five") ShiftPWM.SetOne(9, dimBrightness);
        else if(text == "minutes")
        {
            ShiftPWM.SetOne(10, dimBrightness);
            ShiftPWM.SetOne(11, dimBrightness);
        }
        else if(text == "happy (red)") ShiftPWM.SetOne(12, dimBrightness);
        else if(text == "happy (green)") ShiftPWM.SetOne(13, dimBrightness);
        else if(text == "happy (blue)") ShiftPWM.SetOne(14, dimBrightness);
        else if(text == "past") ShiftPWM.SetOne(15, dimBrightness);
        else if(text == "to") ShiftPWM.SetOne(16, dimBrightness);
        else if(text == "birthday (red)")
        {
            ShiftPWM.SetOne(17, dimBrightness);
            ShiftPWM.SetOne(20, dimBrightness);
        }
        else if(text == "birthday (green)") 
        {
            ShiftPWM.SetOne(18, dimBrightness);
            ShiftPWM.SetOne(21, dimBrightness);
        }
        else if(text == "birthday (blue)")
        {
            ShiftPWM.SetOne(19, dimBrightness);
            ShiftPWM.SetOne(22, dimBrightness);
        }
        else if(text == "three") ShiftPWM.SetOne(23, dimBrightness);
        else if(text == "lexie (red)") ShiftPWM.SetOne(24, dimBrightness);
        else if(text == "lexie (green)") ShiftPWM.SetOne(25, dimBrightness);
        else if(text == "lexie (blue)") ShiftPWM.SetOne(26, dimBrightness);
        else if(text == "eight") ShiftPWM.SetOne(27, dimBrightness);
        else if(text == "one") ShiftPWM.SetOne(28, dimBrightness);
        else if(text == "two") ShiftPWM.SetOne(29, dimBrightness);
        else if(text == "four") ShiftPWM.SetOne(30, dimBrightness);
        else if(text == "eleven")
        {
            ShiftPWM.SetOne(31, dimBrightness);
            ShiftPWM.SetOne(32, dimBrightness);
        }
        else if(text == "nine") ShiftPWM.SetOne(33, dimBrightness);
        else if(text == "seven") ShiftPWM.SetOne(34, dimBrightness);
        else if(text == "five (hour)") ShiftPWM.SetOne(35, dimBrightness);
        else if(text == "six") ShiftPWM.SetOne(36, dimBrightness);
        else if(text == "love (red)") ShiftPWM.SetOne(37, dimBrightness);
        else if(text == "love (green)") ShiftPWM.SetOne(38, dimBrightness);
        else if(text == "love (blue)") ShiftPWM.SetOne(39, dimBrightness);
        else if(text == "you (red)") ShiftPWM.SetOne(40, dimBrightness);
        else if(text == "you (green)") ShiftPWM.SetOne(41, dimBrightness);
        else if(text == "you (blue)") ShiftPWM.SetOne(42, dimBrightness);
        else if(text == "ten (hour)") ShiftPWM.SetOne(43, dimBrightness);
        else if(text == "twelve")
        {
            ShiftPWM.SetOne(44, dimBrightness);
            ShiftPWM.SetOne(45, dimBrightness);
        }
        else if(text == "oclock") 
        {
            ShiftPWM.SetOne(46, dimBrightness);
            ShiftPWM.SetOne(47, dimBrightness);
        } 
      } 
    }
    else if(state == off)
    {
        if(text == "i") ShiftPWM.SetOne(0, 0);
        else if(text == "t") ShiftPWM.SetOne(1, 0);
        else if(text == "it")
        {
            ShiftPWM.SetOne(0, 0);
            ShiftPWM.SetOne(1, 0);      
        }
        else if(text == "is") ShiftPWM.SetOne(2, 0);
        else if(text == "ten") ShiftPWM.SetOne(3, 0);
        else if(text == "half") ShiftPWM.SetOne(4, 0);
        else if(text == "quarter") 
        {
            ShiftPWM.SetOne(5, 0);
            ShiftPWM.SetOne(6, 0);
        }
        else if(text == "twenty")
        {
            ShiftPWM.SetOne(7, 0);
            ShiftPWM.SetOne(8, 0);
        }
        else if(text == "five") ShiftPWM.SetOne(9, 0);
        else if(text == "minutes")
        {
            ShiftPWM.SetOne(10, 0);
            ShiftPWM.SetOne(11, 0);
        }
        else if(text == "happy (red)") ShiftPWM.SetOne(12, 0);
        else if(text == "happy (green)") ShiftPWM.SetOne(13, 0);
        else if(text == "happy (blue)") ShiftPWM.SetOne(14, 0);
        else if(text == "past") ShiftPWM.SetOne(15, 0);
        else if(text == "to") ShiftPWM.SetOne(16, 0);
        else if(text == "birthday (red)")
        {
            ShiftPWM.SetOne(17, 0);
            ShiftPWM.SetOne(20, 0);
        }
        else if(text == "birthday (green)") 
        {
            ShiftPWM.SetOne(18, 0);
            ShiftPWM.SetOne(21, 0);
        }
        else if(text == "birthday (blue)")
        {
            ShiftPWM.SetOne(19, 0);
            ShiftPWM.SetOne(22, 0);
        }
        else if(text == "three") ShiftPWM.SetOne(23, 0);
        else if(text == "lexie (red)") ShiftPWM.SetOne(24, 0);
        else if(text == "lexie (green)") ShiftPWM.SetOne(25, 0);
        else if(text == "lexie (blue)") ShiftPWM.SetOne(26, 0);
        else if(text == "eight") ShiftPWM.SetOne(27, 0);
        else if(text == "one") ShiftPWM.SetOne(28, 0);
        else if(text == "two") ShiftPWM.SetOne(29, 0);
        else if(text == "four") ShiftPWM.SetOne(30, 0);
        else if(text == "eleven")
        {
            ShiftPWM.SetOne(31, 0);
            ShiftPWM.SetOne(32, 0);
        }
        else if(text == "nine") ShiftPWM.SetOne(33, 0);
        else if(text == "seven") ShiftPWM.SetOne(34, 0);
        else if(text == "five (hour)") ShiftPWM.SetOne(35, 0);
        else if(text == "six") ShiftPWM.SetOne(36, 0);
        else if(text == "love (red)") ShiftPWM.SetOne(37, 0);
        else if(text == "love (green)") ShiftPWM.SetOne(38, 0);
        else if(text == "love (blue)") ShiftPWM.SetOne(39, 0);
        else if(text == "you (red)") ShiftPWM.SetOne(40, 0);
        else if(text == "you (green)") ShiftPWM.SetOne(41, 0);
        else if(text == "you (blue)") ShiftPWM.SetOne(42, 0);
        else if(text == "ten (hour)") ShiftPWM.SetOne(43, 0);
        else if(text == "twelve")
        {
            ShiftPWM.SetOne(44, 0);
            ShiftPWM.SetOne(45, 0);
        }
        else if(text == "oclock") 
        {
            ShiftPWM.SetOne(46, 0);
            ShiftPWM.SetOne(47, 0);
        }  
    }
}

void parse_time()
{
    readDS3231time(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);
    tempHour = hour;
    if(minute >=0 && minute <= 2)
    {
      if(time_update == oclock){}
      else
      {
        time_update = oclock;
        ShiftPWM.SetAll(0);
        set_light("it", on);
        set_light("is", on);
        if(hour >= 12) hour -= 12;
        switch(hour)
        {
            case 0:
              set_light("twelve", on);
              break;
            case 1:
              set_light("one", on);
              break;
            case 2:
              set_light("two", on);
              break;
            case 3:
              set_light("three", on);
              break;
            case 4:
              set_light("four", on);
              break;
            case 5:
              set_light("five (hour)", on);
              break;
            case 6:
              set_light("six", on);
              break;
            case 7:
              set_light("seven", on);
              break;
            case 8:
              set_light("eight", on);
              break;
            case 9:
              set_light("nine", on);
              break;
            case 10:
              set_light("ten (hour)", on);
              break;
            case 11:
              set_light("eleven", on);
              break; 
        }
        set_light("oclock", on);
      }
    }
    else if(minute > 2 && minute <= 7)
    {
      if(time_update == five_past){}
      else
      {
        time_update = five_past;
        ShiftPWM.SetAll(0);
        set_light("it", on);
        set_light("is", on);
        set_light("five", on);
        set_light("minutes", on);
        set_light("past", on);
        if(hour >= 12) hour -= 12;
        switch(hour)
        {
            case 0:
              set_light("twelve", on);
              break;
            case 1:
              set_light("one", on);
              break;
            case 2:
              set_light("two", on);
              break;
            case 3:
              set_light("three", on);
              break;
            case 4:
              set_light("four", on);
              break;
            case 5:
              set_light("five (hour)", on);
              break;
            case 6:
              set_light("six", on);
              break;
            case 7:
              set_light("seven", on);
              break;
            case 8:
              set_light("eight", on);
              break;
            case 9:
              set_light("nine", on);
              break;
            case 10:
              set_light("ten (hour)", on);
              break;
            case 11:
              set_light("eleven", on);
              break;
        }
        set_light("oclock", on);
      }
    }
    else if(minute > 7 && minute <= 12)
    {
      if(time_update == ten_past){}
      else
      {
        time_update = ten_past;
        ShiftPWM.SetAll(0);
        set_light("it", on);
        set_light("is", on);
        set_light("ten", on);
        set_light("minutes", on);
        set_light("past", on);
        if(hour >= 12) hour -= 12;
        switch(hour)
        {
            case 0:
              set_light("twelve", on);
              break;
            case 1:
              set_light("one", on);
              break;
            case 2:
              set_light("two", on);
              break;
            case 3:
              set_light("three", on);
              break;
            case 4:
              set_light("four", on);
              break;
            case 5:
              set_light("five (hour)", on);
              break;
            case 6:
              set_light("six", on);
              break;
            case 7:
              set_light("seven", on);
              break;
            case 8:
              set_light("eight", on);
              break;
            case 9:
              set_light("nine", on);
              break;
            case 10:
              set_light("ten (hour)", on);
              break;
            case 11:
              set_light("eleven", on);
              break;
        }
        set_light("oclock", on);
      }
    }
    else if(minute > 12 && minute <= 17)
    {
      if(time_update == quarter_past){}
      else
      {
        time_update = quarter_past;
        ShiftPWM.SetAll(0);
        set_light("it", on);
        set_light("is", on);
        set_light("quarter", on);
        set_light("past", on);
        if(hour >= 12) hour -= 12;
        switch(hour)
        {
            case 0:
              set_light("twelve", on);
              break;
            case 1:
              set_light("one", on);
              break;
            case 2:
              set_light("two", on);
              break;
            case 3:
              set_light("three", on);
              break;
            case 4:
              set_light("four", on);
              break;
            case 5:
              set_light("five (hour)", on);
              break;
            case 6:
              set_light("six", on);
              break;
            case 7:
              set_light("seven", on);
              break;
            case 8:
              set_light("eight", on);
              break;
            case 9:
              set_light("nine", on);
              break;
            case 10:
              set_light("ten (hour)", on);
              break;
            case 11:
              set_light("eleven", on);
              break; 
        }
        set_light("oclock", on);
      }
    }
    else if(minute > 17 && minute <= 22)
    {
      if(time_update == twenty_past){}
      else
      {
        time_update = twenty_past;
        ShiftPWM.SetAll(0);
        set_light("it", on);
        set_light("is", on);
        set_light("twenty", on);
        set_light("minutes", on);
        set_light("past", on);
        if(hour >= 12) hour -= 12;
        switch(hour)
        {
            case 0:
              set_light("twelve", on);
              break;
            case 1:
              set_light("one", on);
              break;
            case 2:
              set_light("two", on);
              break;
            case 3:
              set_light("three", on);
              break;
            case 4:
              set_light("four", on);
              break;
            case 5:
              set_light("five (hour)", on);
              break;
            case 6:
              set_light("six", on);
              break;
            case 7:
              set_light("seven", on);
              break;
            case 8:
              set_light("eight", on);
              break;
            case 9:
              set_light("nine", on);
              break;
            case 10:
              set_light("ten (hour)", on);
              break;
            case 11:
              set_light("eleven", on);
              break;
        }
        set_light("oclock", on);
      }
    }
    else if(minute > 22 && minute <= 27)
    {
      if(time_update == twenty_five_past){}
      else
      {
        time_update = twenty_five_past;
        ShiftPWM.SetAll(0);
        set_light("it", on);
        set_light("is", on);
        set_light("twenty", on);
        set_light("five", on);
        set_light("minutes", on);
        set_light("past", on);
        if(hour >= 12) hour -= 12;
        switch(hour)
        {
            case 0:
              set_light("twelve", on);
              break;
            case 1:
              set_light("one", on);
              break;
            case 2:
              set_light("two", on);
              break;
            case 3:
              set_light("three", on);
              break;
            case 4:
              set_light("four", on);
              break;
            case 5:
              set_light("five (hour)", on);
              break;
            case 6:
              set_light("six", on);
              break;
            case 7:
              set_light("seven", on);
              break;
            case 8:
              set_light("eight", on);
              break;
            case 9:
              set_light("nine", on);
              break;
            case 10:
              set_light("ten (hour)", on);
              break;
            case 11:
              set_light("eleven", on);
              break; 
        }
        set_light("oclock", on);
      }
    }
    else if(minute > 27 && minute <= 32)
    {
      if(time_update == half_past){}
      else
      {
        time_update = half_past;
        ShiftPWM.SetAll(0);
        set_light("it", on);
        set_light("is", on);
        set_light("half", on);
        set_light("past", on);
        if(hour >= 12) hour -= 12;
        switch(hour)
        {
            case 0:
              set_light("twelve", on);
              break;
            case 1:
              set_light("one", on);
              break;
            case 2:
              set_light("two", on);
              break;
            case 3:
              set_light("three", on);
              break;
            case 4:
              set_light("four", on);
              break;
            case 5:
              set_light("five (hour)", on);
              break;
            case 6:
              set_light("six", on);
              break;
            case 7:
              set_light("seven", on);
              break;
            case 8:
              set_light("eight", on);
              break;
            case 9:
              set_light("nine", on);
              break;
            case 10:
              set_light("ten (hour)", on);
              break;
            case 11:
              set_light("eleven", on);
              break;
        }
        set_light("oclock", on);
      }
    }
    else if(minute > 32 && minute <= 37)
    {
      if(time_update == twenty_five_to){}
      else
      {
        time_update = twenty_five_to;
        ShiftPWM.SetAll(0);
        set_light("it", on);
        set_light("is", on);
        set_light("twenty", on);
        set_light("five", on);
        set_light("minutes", on);
        set_light("to", on);
        if(hour >= 12) hour -= 12;
        switch(hour)
        {
            case 0:
              set_light("one", on);
              break;
            case 1:
              set_light("two", on);
              break;
            case 2:
              set_light("three", on);
              break;
            case 3:
              set_light("four", on);
              break;
            case 4:
              set_light("five (hour)", on);
              break;
            case 5:
              set_light("six", on);
              break;
            case 6:
              set_light("seven", on);
              break;
            case 7:
              set_light("eight", on);
              break;
            case 8:
              set_light("nine", on);
              break;
            case 9:
              set_light("ten (hour)", on);
              break;
            case 10:
              set_light("eleven", on);
              break;
            case 11:
              set_light("twelve", on);
              break;
        }
        set_light("oclock", on);
      }
    }
    else if(minute > 37 && minute <= 42)
    {
      if(time_update == twenty_to){}
      else
      {
        time_update = twenty_to;
        ShiftPWM.SetAll(0);
        set_light("it", on);
        set_light("is", on);
        set_light("twenty", on);
        set_light("minutes", on);
        set_light("to", on);
        if(hour >= 12) hour -= 12;
        switch(hour)
        {
            case 0:
              set_light("one", on);
              break;
            case 1:
              set_light("two", on);
              break;
            case 2:
              set_light("three", on);
              break;
            case 3:
              set_light("four", on);
              break;
            case 4:
              set_light("five (hour)", on);
              break;
            case 5:
              set_light("six", on);
              break;
            case 6:
              set_light("seven", on);
              break;
            case 7:
              set_light("eight", on);
              break;
            case 8:
              set_light("nine", on);
              break;
            case 9:
              set_light("ten (hour)", on);
              break;
            case 10:
              set_light("eleven", on);
              break;
            case 11:
              set_light("twelve", on);
              break;
        }
        set_light("oclock", on);
      }
    }
    else if(minute > 42 && minute <= 47)
    {
      if(time_update == quarter_to){}
      else
      {
        time_update = quarter_to;
        ShiftPWM.SetAll(0);
        set_light("it", on);
        set_light("is", on);
        set_light("quarter", on);
        set_light("to", on);
        if(hour >= 12) hour -= 12;
        switch(hour)
        {
            case 0:
              set_light("one", on);
              break;
            case 1:
              set_light("two", on);
              break;
            case 2:
              set_light("three", on);
              break;
            case 3:
              set_light("four", on);
              break;
            case 4:
              set_light("five (hour)", on);
              break;
            case 5:
              set_light("six", on);
              break;
            case 6:
              set_light("seven", on);
              break;
            case 7:
              set_light("eight", on);
              break;
            case 8:
              set_light("nine", on);
              break;
            case 9:
              set_light("ten (hour)", on);
              break;
            case 10:
              set_light("eleven", on);
              break;
            case 11:
              set_light("twelve", on);
              break;
        }
        set_light("oclock", on);
      }
    }
    else if(minute > 47 && minute <= 52)
    {
      if(time_update == ten_to){}
      else
      {
        time_update = ten_to;
        ShiftPWM.SetAll(0);
        set_light("it", on);
        set_light("is", on);
        set_light("ten", on);
        set_light("minutes", on);
        set_light("to", on);
        if(hour >= 12) hour -= 12;
        switch(hour)
        {
            case 0:
              set_light("one", on);
              break;
            case 1:
              set_light("two", on);
              break;
            case 2:
              set_light("three", on);
              break;
            case 3:
              set_light("four", on);
              break;
            case 4:
              set_light("five (hour)", on);
              break;
            case 5:
              set_light("six", on);
              break;
            case 6:
              set_light("seven", on);
              break;
            case 7:
              set_light("eight", on);
              break;
            case 8:
              set_light("nine", on);
              break;
            case 9:
              set_light("ten (hour)", on);
              break;
            case 10:
              set_light("eleven", on);
              break;
            case 11:
              set_light("twelve", on);
              break;
        }
        set_light("oclock", on);
      }
    }
    else if(minute > 52 && minute <= 57)
    {
      if(time_update == five_to){}
      else
      {
        time_update = five_to;
        ShiftPWM.SetAll(0);
        set_light("it", on);
        set_light("is", on);
        set_light("five", on);
        set_light("minutes", on);
        set_light("to", on);
        if(hour >= 12) hour -= 12;
        switch(hour)
        {
            case 0:
              set_light("one", on);
              break;
            case 1:
              set_light("two", on);
              break;
            case 2:
              set_light("three", on);
              break;
            case 3:
              set_light("four", on);
              break;
            case 4:
              set_light("five (hour)", on);
              break;
            case 5:
              set_light("six", on);
              break;
            case 6:
              set_light("seven", on);
              break;
            case 7:
              set_light("eight", on);
              break;
            case 8:
              set_light("nine", on);
              break;
            case 9:
              set_light("ten (hour)", on);
              break;
            case 10:
              set_light("eleven", on);
              break;
            case 11:
              set_light("twelve", on);
              break;
        }
        set_light("oclock", on);
      }
    }
    else if(minute > 57)
    {
      if(time_update == oclock){}
      else
      {
        time_update = oclock;
        ShiftPWM.SetAll(0);
        set_light("it", on);
        set_light("is", on);
        if(hour >= 12) hour -= 12;
        switch(hour)
        {
            case 0:
              set_light("one", on);
              break;
            case 1:
              set_light("two", on);
              break;
            case 2:
              set_light("three", on);
              break;
            case 3:
              set_light("four", on);
              break;
            case 4:
              set_light("five (hour)", on);
              break;
            case 5:
              set_light("six", on);
              break;
            case 6:
              set_light("seven", on);
              break;
            case 7:
              set_light("eight", on);
              break;
            case 8:
              set_light("nine", on);
              break;
            case 9:
              set_light("ten (hour)", on);
              break;
            case 10:
              set_light("eleven", on);
              break;
            case 11:
              set_light("twelve", on);
              break;
        }
        set_light("oclock", on);
      }
    }
    hour = tempHour;
}

void hsvtorgb(unsigned char *r, unsigned char *g, unsigned char *b, unsigned char h, unsigned char s, unsigned char v)
{
    unsigned char region, fpart, p, q, t;
    
    if(s == 0) {
        /* color is grayscale */
        *r = *g = *b = v;
        return;
    }
    
    /* make hue 0-5 */
    region = h / 43;
    /* find remainder part, make it from 0-255 */
    fpart = (h - (region * 43)) * 6;
    
    /* calculate temp vars, doing integer multiplication */
    p = (v * (255 - s)) >> 8;
    q = (v * (255 - ((s * fpart) >> 8))) >> 8;
    t = (v * (255 - ((s * (255 - fpart)) >> 8))) >> 8;
        
    /* assign temp vars based on color cone region */
    switch(region) {
        case 0:
            *r = v; *g = t; *b = p; break;
        case 1:
            *r = q; *g = v; *b = p; break;
        case 2:
            *r = p; *g = v; *b = t; break;
        case 3:
            *r = p; *g = q; *b = v; break;
        case 4:
            *r = t; *g = p; *b = v; break;
        default:
            *r = v; *g = p; *b = q; break;
    }
    
    return;
}
