#include "Wire.h"
#define DS3231_I2C_ADDRESS 0x68

const int topButtonPin = 2;
const int bottomButtonPin = 3;

unsigned char red, blue, green;

const int ShiftPWM_latchPin=4;
const bool ShiftPWM_invertOutputs = false;
const bool ShiftPWM_balanceLoad = false;

#include <ShiftPWM.h>

typedef struct RgbColor
{
    unsigned char r;
    unsigned char g;
    unsigned char b;
} RgbColor;

typedef struct HsvColor
{
    unsigned char h;
    unsigned char s;
    unsigned char v;
} HsvColor;

typedef enum{
  off = 0,
  dim = 5,
  bright = 255
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

#define was_low_do_not_change       B00000001
#define was_low_change_time         B00000010
#define already_changed_time        B00000100

byte topButtonState = 0;
byte bottomButtonState = 0;
long buttonCompareTime= 0;
lights_t lights = {bright, 1};
time_update_t time_update = manual_update; 
bool birthday = false;

RgbColor rgb = {0, 0, 0};
HsvColor hsv = {0, 0, 0};

void rgbLedRainbow(unsigned long cycleTime, int rainbowWidth);
void rgbLedRainbowLoveYou(unsigned long cycleTime, int rainbowWidth);
void checkStartBrightness(byte hour);
void displayTimeCheck();
void handleTopButton();
void handleBottomButton();
void resetButtonStates();
void addFive();
void subtractFive();
void checkBirthday();
void checkChangeBrightness();
void light_time_to_hour(byte hour);
void light_time_past_hour(byte hour);
RgbColor HsvToRgb2(HsvColor hsv);

unsigned char pwmFrequency = 70;
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

  pinMode(topButtonPin, INPUT);
  pinMode(bottomButtonPin, INPUT);
  
  ShiftPWM.SetAmountOfRegisters(numRegisters);
  ShiftPWM.SetPinGrouping(1); 
  ShiftPWM.Start(pwmFrequency,bright);
  ShiftPWM.SetAll(0);
  
  // DS3231 seconds, minutes, hours, day, date, month, year
  //setDS3231time(40,34,3,4,17,8,16);
  readDS3231time(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);
  lastSecond = second;
  checkStartBrightness(hour);
}

void loop()
{    
  displayTimeCheck();
  
  parse_time();
  
  handleTopButton();

  handleBottomButton();

  resetButtonStates();

  checkBirthday();

  checkChangeBrightness();
}

void rgbLedRainbow(unsigned long cycleTime, int rainbowWidth){
  // Displays a rainbow spread over a few LED's (numRGBLeds), which shifts in hue. 
  // The rainbow can be wider then the real number of LED's.
  unsigned long time = millis()-startTime;
  unsigned long colorShift = (256*time/cycleTime)%256; // this color shift is like the hue slider in Photoshop.

  for(unsigned int led=0;led<3;led++){ // loop over all LED's
    int hue = ((led)*256/(rainbowWidth-1)+colorShift)%256; // Set hue from 0 to 360 from first to last led and shift the hue
    hsv.h = hue;
    hsv.s = bright;
    hsv.v = bright;
    rgb = HsvToRgb2(hsv);
    switch(led){
      case 0: ShiftPWM.SetOne(12, rgb.r); // write the HSV values, with saturation and value at maximum
              ShiftPWM.SetOne(13, rgb.g); // write the HSV values, with saturation and value at maximum
              ShiftPWM.SetOne(14, rgb.b); // write the HSV values, with saturation and value at maximum
              break;
      case 1: ShiftPWM.SetOne(17, rgb.r); // write the HSV values, with saturation and value at maximum
              ShiftPWM.SetOne(18, rgb.g); // write the HSV values, with saturation and value at maximum
              ShiftPWM.SetOne(19, rgb.b); // write the HSV values, with saturation and value at maximum
              ShiftPWM.SetOne(20, rgb.r); // write the HSV values, with saturation and value at maximum
              ShiftPWM.SetOne(21, rgb.g); // write the HSV values, with saturation and value at maximum
              ShiftPWM.SetOne(22, rgb.b); // write the HSV values, with saturation and value at maximum
              break;
      case 2: ShiftPWM.SetOne(24, rgb.r); // write the HSV values, with saturation and value at maximum
              ShiftPWM.SetOne(25, rgb.g); // write the HSV values, with saturation and value at maximum
              ShiftPWM.SetOne(26, rgb.b); // write the HSV values, with saturation and value at maximum
              break;
    }
  }
}

void rgbLedRainbowLoveYou(unsigned long cycleTime, int rainbowWidth){
  // Displays a rainbow spread over a few LED's (numRGBLeds), which shifts in hue. 
  // The rainbow can be wider then the real number of LED's.
  unsigned long time = millis()-startTime;
  unsigned long colorShift = (256*time/cycleTime)%256; // this color shift is like the hue slider in Photoshop.

  for(unsigned int led=0;led<2;led++){ // loop over all LED's
    int hue = ((led)*256/(rainbowWidth-1)+colorShift)%256; // Set hue from 0 to 360 from first to last led and shift the hue
    hsv.h = hue;
    hsv.s = bright;
    hsv.v = bright;
    rgb = HsvToRgb2(hsv);
    switch(led){
      case 0: ShiftPWM.SetOne(37, rgb.r); // write the HSV values, with saturation and value at maximum
              ShiftPWM.SetOne(38, rgb.g); // write the HSV values, with saturation and value at maximum
              ShiftPWM.SetOne(39, rgb.b); // write the HSV values, with saturation and value at maximum
              break;
      case 1: ShiftPWM.SetOne(40, rgb.r); // write the HSV values, with saturation and value at maximum
              ShiftPWM.SetOne(41, rgb.g); // write the HSV values, with saturation and value at maximum
              ShiftPWM.SetOne(42, rgb.b); // write the HSV values, with saturation and value at maximum
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

void checkStartBrightness(byte hour)
{
  if((hour >=8) && (hour < 20)) lights.light_state = bright;
  else lights.light_state = dim;
}

void displayTimeCheck()
{
  if(lastSecond != second)
  {
    displayTime();
    lastSecond = second;
  }
}

void handleTopButton()
{
  buttonCompareTime = millis();
  while(digitalRead(topButtonPin) == LOW)
  {
    if((millis() - buttonCompareTime) >= 3000) // check if top button has been held for over 3 seconds
    {
      if(lights.action_needed == 1) // check if action can be taken (only == 1 for first time entering). change brightness
      {
        if(lights.light_state == bright) lights.light_state = dim;
        else lights.light_state = bright;
        lights.action_needed = 0;
        topButtonState |= was_low_do_not_change;
        ShiftPWM.SetAll(0);      
        time_update = manual_update;
      }
    }
    
    topButtonState |= was_low_change_time;
    if(digitalRead(bottomButtonPin) == LOW) // check if bottom button is being pushed
    {
      topButtonState |= was_low_do_not_change; // bottom button was pushed, disregard adding time
      ShiftPWM.SetAll(0);
      while(digitalRead(bottomButtonPin) == LOW) // as long as bottom button is pushed
      {
        if(digitalRead(topButtonPin) == LOW) // make sure top button is still pressed, and do led animation
        {
          ShiftPWM.SetAll(0);
          set_light("i", bright);
          rgbLedRainbowLoveYou(4000, numRGBLeds);
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
  if(topButtonState == was_low_change_time) // if top button was pushed alone for long enough
  {
    topButtonState |= already_changed_time; // set flag so you know time has been added
    addFive();
    parse_time();
  }
}

void handleBottomButton()
{
  while(digitalRead(bottomButtonPin) == LOW) // as long as bottom button is pushed
  {
    bottomButtonState |= was_low_change_time; // prepare to subtract time
    if(digitalRead(topButtonPin) == LOW) // if top button is pushed with bottom button, animation will happen
    {
      bottomButtonState |= was_low_do_not_change; // disregard time being subtracted
      ShiftPWM.SetAll(0);
      while(digitalRead(topButtonPin) == LOW) // as long as top button is pushed
      {
        if(digitalRead(bottomButtonPin) == LOW) // make sure bottom button is still pressed, and do led animation
        {
          ShiftPWM.SetAll(0);
          set_light("i", bright);
          set_light("love (red)", bright);
          set_light("you (red)", bright);
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
  if(bottomButtonState == was_low_change_time) // if bottom button was pushed alone
    {
      bottomButtonState |= already_changed_time; // set flag so you know time has been subtracted
      subtractFive();
      parse_time();
    }
}

void resetButtonStates()
{
  bottomButtonState = 0;
  topButtonState = 0;
  lights.action_needed = 1;
}

void addFive()
{
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
}

void subtractFive()
{
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
}

void checkBirthday()
{
  if((month == 12) && (dayOfMonth == 10)) 
  {
    rgbLedRainbow(4000, numRGBLeds);
    birthday = true;
  }
  else if((birthday == true) && ((month != 12) || (dayOfMonth != 10)))
  {
    birthday = false;
    time_update = manual_update;
  }
}

void checkChangeBrightness()
{
  if((hour == 8) && (minute == 0) && (lights.light_state != bright))
  {
    time_update = manual_update;
    lights.light_state = bright;
  }
  else if((hour == 20) && (minute == 0) && (lights.light_state != dim))
  {
    time_update = manual_update;
    lights.light_state = dim;
  }
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

void set_light(char const* text, light_state_t state)
{
        if(text == "i") ShiftPWM.SetOne(0, state);
        else if(text == "t") ShiftPWM.SetOne(1, state);
        else if(text == "it")
        {
            ShiftPWM.SetOne(0, state);
            ShiftPWM.SetOne(1, state);      
        }
        else if(text == "is") ShiftPWM.SetOne(2, state);
        else if(text == "ten") ShiftPWM.SetOne(3, state);
        else if(text == "half") ShiftPWM.SetOne(4, state);
        else if(text == "quarter") 
        {
            ShiftPWM.SetOne(5, state);
            ShiftPWM.SetOne(6, state);
        }
        else if(text == "twenty")
        {
            ShiftPWM.SetOne(7, state);
            ShiftPWM.SetOne(8, state);
        }
        else if(text == "five") ShiftPWM.SetOne(9, state);
        else if(text == "minutes")
        {
            ShiftPWM.SetOne(10, state);
            ShiftPWM.SetOne(11, state);
        }
        else if(text == "happy (red)") ShiftPWM.SetOne(12, state);
        else if(text == "happy (green)") ShiftPWM.SetOne(13, state);
        else if(text == "happy (blue)") ShiftPWM.SetOne(14, state);
        else if(text == "past") ShiftPWM.SetOne(15, state);
        else if(text == "to") ShiftPWM.SetOne(16, state);
        else if(text == "birthday (red)")
        {
            ShiftPWM.SetOne(17, state);
            ShiftPWM.SetOne(20, state);
        }
        else if(text == "birthday (green)") 
        {
            ShiftPWM.SetOne(18, state);
            ShiftPWM.SetOne(21, state);
        }
        else if(text == "birthday (blue)")
        {
            ShiftPWM.SetOne(19, state);
            ShiftPWM.SetOne(22, state);
        }
        else if(text == "three") ShiftPWM.SetOne(23, state);
        else if(text == "lexie (red)") ShiftPWM.SetOne(24, state);
        else if(text == "lexie (green)") ShiftPWM.SetOne(25, state);
        else if(text == "lexie (blue)") ShiftPWM.SetOne(26, state);
        else if(text == "eight") ShiftPWM.SetOne(27, state);
        else if(text == "one") ShiftPWM.SetOne(28, state);
        else if(text == "two") ShiftPWM.SetOne(29, state);
        else if(text == "four") ShiftPWM.SetOne(30, state);
        else if(text == "eleven")
        {
            ShiftPWM.SetOne(31, state);
            ShiftPWM.SetOne(32, state);
        }
        else if(text == "nine") ShiftPWM.SetOne(33, state);
        else if(text == "seven") ShiftPWM.SetOne(34, state);
        else if(text == "five (hour)") ShiftPWM.SetOne(35, state);
        else if(text == "six") ShiftPWM.SetOne(36, state);
        else if(text == "love (red)") ShiftPWM.SetOne(37, state);
        else if(text == "love (green)") ShiftPWM.SetOne(38, state);
        else if(text == "love (blue)") ShiftPWM.SetOne(39, state);
        else if(text == "you (red)") ShiftPWM.SetOne(40, state);
        else if(text == "you (green)") ShiftPWM.SetOne(41, state);
        else if(text == "you (blue)") ShiftPWM.SetOne(42, state);
        else if(text == "ten (hour)") ShiftPWM.SetOne(43, state);
        else if(text == "twelve")
        {
            ShiftPWM.SetOne(44, state);
            ShiftPWM.SetOne(45, state);
        }
        else if(text == "oclock") 
        {
            ShiftPWM.SetOne(46, state);
            ShiftPWM.SetOne(47, state);
        } 
} 

void light_time_to_hour(byte hour)
{
    hour %= 12;
    switch(hour)
    {
        case 0:
          set_light("one", lights.light_state);
          break;
        case 1:
          set_light("two", lights.light_state);
          break;
        case 2:
          set_light("three", lights.light_state);
          break;
        case 3:
          set_light("four", lights.light_state);
          break;
        case 4:
          set_light("five (hour)", lights.light_state);
          break;
        case 5:
          set_light("six", lights.light_state);
          break;
        case 6:
          set_light("seven", lights.light_state);
          break;
        case 7:
          set_light("eight", lights.light_state);
          break;
        case 8:
          set_light("nine", lights.light_state);
          break;
        case 9:
          set_light("ten (hour)", lights.light_state);
          break;
        case 10:
          set_light("eleven", lights.light_state);
          break;
        case 11:
          set_light("twelve", lights.light_state);
          break;
    }
}

void light_time_past_hour(byte hour)
{
    hour %= 12;
    switch(hour)
    {
        case 0:
          set_light("twelve", lights.light_state);
          break;
        case 1:
          set_light("one", lights.light_state);
          break;
        case 2:
          set_light("two", lights.light_state);
          break;
        case 3:
          set_light("three", lights.light_state);
          break;
        case 4:
          set_light("four", lights.light_state);
          break;
        case 5:
          set_light("five (hour)", lights.light_state);
          break;
        case 6:
          set_light("six", lights.light_state);
          break;
        case 7:
          set_light("seven", lights.light_state);
          break;
        case 8:
          set_light("eight", lights.light_state);
          break;
        case 9:
          set_light("nine", lights.light_state);
          break;
        case 10:
          set_light("ten (hour)", lights.light_state);
          break;
        case 11:
          set_light("eleven", lights.light_state);
          break; 
    }
}

void parse_time()
{
    readDS3231time(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);
    if(minute >=0 && minute <= 2)
    {
      if(time_update == oclock){}
      else
      {
        time_update = oclock;
        ShiftPWM.SetAll(0);
        set_light("it", lights.light_state);
        set_light("is", lights.light_state);
        light_time_past_hour(hour);
        set_light("oclock", lights.light_state);
      }
    }
    else if(minute > 2 && minute <= 7)
    {
      if(time_update == five_past){}
      else
      {
        time_update = five_past;
        ShiftPWM.SetAll(0);
        set_light("it", lights.light_state);
        set_light("is", lights.light_state);
        set_light("five", lights.light_state);
        set_light("minutes", lights.light_state);
        set_light("past", lights.light_state);
        light_time_past_hour(hour);
        set_light("oclock", lights.light_state);
      }
    }
    else if(minute > 7 && minute <= 12)
    {
      if(time_update == ten_past){}
      else
      {
        time_update = ten_past;
        ShiftPWM.SetAll(0);
        set_light("it", lights.light_state);
        set_light("is", lights.light_state);
        set_light("ten", lights.light_state);
        set_light("minutes", lights.light_state);
        set_light("past", lights.light_state);
        light_time_past_hour(hour);
        set_light("oclock", lights.light_state);
      }
    }
    else if(minute > 12 && minute <= 17)
    {
      if(time_update == quarter_past){}
      else
      {
        time_update = quarter_past;
        ShiftPWM.SetAll(0);
        set_light("it", lights.light_state);
        set_light("is", lights.light_state);
        set_light("quarter", lights.light_state);
        set_light("past", lights.light_state);
        light_time_past_hour(hour);
        set_light("oclock", lights.light_state);
      }
    }
    else if(minute > 17 && minute <= 22)
    {
      if(time_update == twenty_past){}
      else
      {
        time_update = twenty_past;
        ShiftPWM.SetAll(0);
        set_light("it", lights.light_state);
        set_light("is", lights.light_state);
        set_light("twenty", lights.light_state);
        set_light("minutes", lights.light_state);
        set_light("past", lights.light_state);
        light_time_past_hour(hour);
        set_light("oclock", lights.light_state);
      }
    }
    else if(minute > 22 && minute <= 27)
    {
      if(time_update == twenty_five_past){}
      else
      {
        time_update = twenty_five_past;
        ShiftPWM.SetAll(0);
        set_light("it", lights.light_state);
        set_light("is", lights.light_state);
        set_light("twenty", lights.light_state);
        set_light("five", lights.light_state);
        set_light("minutes", lights.light_state);
        set_light("past", lights.light_state);
        light_time_past_hour(hour);
        set_light("oclock", lights.light_state);
      }
    }
    else if(minute > 27 && minute <= 32)
    {
      if(time_update == half_past){}
      else
      {
        time_update = half_past;
        ShiftPWM.SetAll(0);
        set_light("it", lights.light_state);
        set_light("is", lights.light_state);
        set_light("half", lights.light_state);
        set_light("past", lights.light_state);
        light_time_past_hour(hour);
        set_light("oclock", lights.light_state);
      }
    }
    else if(minute > 32 && minute <= 37)
    {
      if(time_update == twenty_five_to){}
      else
      {
        time_update = twenty_five_to;
        ShiftPWM.SetAll(0);
        set_light("it", lights.light_state);
        set_light("is", lights.light_state);
        set_light("twenty", lights.light_state);
        set_light("five", lights.light_state);
        set_light("minutes", lights.light_state);
        set_light("to", lights.light_state);
        light_time_to_hour(hour);
        set_light("oclock", lights.light_state);
      }
    }
    else if(minute > 37 && minute <= 42)
    {
      if(time_update == twenty_to){}
      else
      {
        time_update = twenty_to;
        ShiftPWM.SetAll(0);
        set_light("it", lights.light_state);
        set_light("is", lights.light_state);
        set_light("twenty", lights.light_state);
        set_light("minutes", lights.light_state);
        set_light("to", lights.light_state);
        light_time_to_hour(hour);
        set_light("oclock", lights.light_state);
      }
    }
    else if(minute > 42 && minute <= 47)
    {
      if(time_update == quarter_to){}
      else
      {
        time_update = quarter_to;
        ShiftPWM.SetAll(0);
        set_light("it", lights.light_state);
        set_light("is", lights.light_state);
        set_light("quarter", lights.light_state);
        set_light("to", lights.light_state);
        light_time_to_hour(hour);
        set_light("oclock", lights.light_state);
      }
    }
    else if(minute > 47 && minute <= 52)
    {
      if(time_update == ten_to){}
      else
      {
        time_update = ten_to;
        ShiftPWM.SetAll(0);
        set_light("it", lights.light_state);
        set_light("is", lights.light_state);
        set_light("ten", lights.light_state);
        set_light("minutes", lights.light_state);
        set_light("to", lights.light_state);
        light_time_to_hour(hour);
        set_light("oclock", lights.light_state);
      }
    }
    else if(minute > 52 && minute <= 57)
    {
      if(time_update == five_to){}
      else
      {
        time_update = five_to;
        ShiftPWM.SetAll(0);
        set_light("it", lights.light_state);
        set_light("is", lights.light_state);
        set_light("five", lights.light_state);
        set_light("minutes", lights.light_state);
        set_light("to", lights.light_state);
        light_time_to_hour(hour);
        set_light("oclock", lights.light_state);
      }
    }
    else if(minute > 57)
    {
      if(time_update == oclock){}
      else
      {
        time_update = oclock;
        ShiftPWM.SetAll(0);
        set_light("it", lights.light_state);
        set_light("is", lights.light_state);
        light_time_to_hour(hour);
        set_light("oclock", lights.light_state);
      }
    }
}

RgbColor HsvToRgb2(HsvColor hsv)
{
    RgbColor rgb;
    unsigned char region, p, q, t;
    unsigned int h, s, v, remainder;

    if (hsv.s == 0)
    {
        rgb.r = hsv.v;
        rgb.g = hsv.v;
        rgb.b = hsv.v;
        return rgb;
    }

    // converting to 16 bit to prevent overflow
    h = hsv.h;
    s = hsv.s;
    v = hsv.v;

    region = h / 43;
    remainder = (h - (region * 43)) * 6; 

    p = (v * (255 - s)) >> 8;
    q = (v * (255 - ((s * remainder) >> 8))) >> 8;
    t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8;

    switch (region)
    {
        case 0:
            rgb.r = v;
            rgb.g = t;
            rgb.b = p;
            break;
        case 1:
            rgb.r = q;
            rgb.g = v;
            rgb.b = p;
            break;
        case 2:
            rgb.r = p;
            rgb.g = v;
            rgb.b = t;
            break;
        case 3:
            rgb.r = p;
            rgb.g = q;
            rgb.b = v;
            break;
        case 4:
            rgb.r = t;
            rgb.g = p;
            rgb.b = v;
            break;
        default:
            rgb.r = v;
            rgb.g = p;
            rgb.b = q;
            break;
    }

    return rgb;
}
