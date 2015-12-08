/*
 * ShiftPWM non-blocking RGB fades example, (c) Elco Jacobs, updated August 2012.
 *
 * This example for ShiftPWM shows how to control your LED's in a non-blocking way: no delay loops.
 * This example receives a number from the serial port to set the fading mode. Instead you can also read buttons or sensors.
 * It uses the millis() function to create fades. The block fades example might be easier to understand, so start there.
 *
 * Please go to www.elcojacobs.com/shiftpwm for documentation, fuction reference and schematics.
 * If you want to use ShiftPWM with LED strips or high power LED's, visit the shop for boards.
 */

// ShiftPWM uses timer1 by default. To use a different timer, before '#include <ShiftPWM.h>', add
// #define SHIFTPWM_USE_TIMER2  // for Arduino Uno and earlier (Atmega328)
// #define SHIFTPWM_USE_TIMER3  // for Arduino Micro/Leonardo (Atmega32u4)

// Clock and data pins are pins from the hardware SPI, you cannot choose them yourself.
// Data pin is MOSI (Uno and earlier: 11, Leonardo: ICSP 4, Mega: 51, Teensy 2.0: 2, Teensy 2.0++: 22) 
// Clock pin is SCK (Uno and earlier: 13, Leonardo: ICSP 3, Mega: 52, Teensy 2.0: 1, Teensy 2.0++: 21)

#include "Wire.h"
#define DS3231_I2C_ADDRESS 0x68
// Convert normal decimal numbers to binary coded decimal

const int upButtonPin = 2;
const int downButtonPin = 3;

unsigned char red, blue, green;


// You can choose the latch pin yourself.
const int ShiftPWM_latchPin=4;

// ** uncomment this part to NOT use the SPI port and change the pin numbers. This is 2.5x slower **
// #define SHIFTPWM_NOSPI
// const int ShiftPWM_dataPin = 6;
// const int ShiftPWM_clockPin = 4;

// If your LED's turn on if the pin is low, set this to true, otherwise set it to false.
const bool ShiftPWM_invertOutputs = false;

// You can enable the option below to shift the PWM phase of each shift register by 8 compared to the previous.
// This will slightly increase the interrupt load, but will prevent all PWM signals from becoming high at the same time.
// This will be a bit easier on your power supply, because the current peaks are distributed.
const bool ShiftPWM_balanceLoad = false;

#include <ShiftPWM.h>   // include ShiftPWM.h after setting the pins!

typedef enum{
  off,
  on
} led_state_t;

//typedef enum{
//  was_low_no_add_time,
//  was_low_add_time,
//  already_added_time,
//  waiting
//} button_state_t;

byte upButtonState = 0;
byte downButtonState = 0;
byte was_low_no_add_time = B00000001;
byte was_low_add_time = B00000010;
byte already_added_time = B00000100;
byte waiting = B00001000;
//button_state_t upButtonState;
//button_state_t downButtonState;

// Function prototypes (telling the compiler these functions exist).
void oneByOne(void);
void inOutTwoLeds(void);
void inOutAll(void);
void alternatingColors(void);
void hueShiftAll(void);
void randomColors(void);
void fakeVuMeter(void);
void rgbLedRainbow(unsigned long cycleTime, int rainbowWidth);
void printInstructions(void);

// Here you set the number of brightness levels, the update frequency and the number of shift registers.
// These values affect the load of ShiftPWM.
// Choose them wisely and use the PrintInterruptLoad() function to verify your load.
unsigned char maxBrightness = 255;
unsigned char pwmFrequency = 75;
unsigned int numRegisters = 6;
unsigned int numOutputs = numRegisters*8;
unsigned int numRGBLeds = numRegisters*8/3;
unsigned int fadingMode = 0; //start with all LED's off.

unsigned long startTime = 0; // start time for the chosen fading mode

byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;

void setup(){
  while(!Serial){
    delay(100); 
  }
  Wire.begin();
  Serial.begin(9600);

  // Sets the number of 8-bit registers that are used.
  ShiftPWM.SetAmountOfRegisters(numRegisters);

  // SetPinGrouping allows flexibility in LED setup. 
  // If your LED's are connected like this: RRRRGGGGBBBBRRRRGGGGBBBB, use SetPinGrouping(4).
  ShiftPWM.SetPinGrouping(1); //This is the default, but I added here to demonstrate how to use the funtion
  
  ShiftPWM.Start(pwmFrequency,maxBrightness);
  printInstructions();
  // set the initial time here:
  // DS3231 seconds, minutes, hours, day, date, month, year
  //setDS3231time(10,05,21,4,2,12,15);
  ShiftPWM.SetAll(0);
  pinMode(upButtonPin, INPUT);
  pinMode(downButtonPin, INPUT);

//  attachInterrupt(digitalPinToInterrupt(2), upPress, LOW);
//  attachInterrupt(digitalPinToInterrupt(3), downPress, CHANGE);
}

void loop()
{    
  displayTime(); // display the real-time clock data on the Serial Monitor,
  parse_time();
  while(digitalRead(upButtonPin) == LOW)
  {
    upButtonState |= was_low_add_time;
    if(digitalRead(downButtonPin) == LOW)
    {
      upButtonState |= was_low_no_add_time;
      ShiftPWM.SetAll(0);
      while(digitalRead(downButtonPin) == LOW)
      {
        //set_light("i", on);
//        set_light("love (red)", on);
//        set_light("you (red)", on);
      set_light("i", on);
      rgbLedRainbowLoveYou(10000,numRGBLeds);

      }
    }
  }
  if(upButtonState == was_low_add_time)
    {
      upButtonState |= already_added_time;
      readDS3231time(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);
      minute += 5;
      if(minute >= 60) minute -= 60;
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
        //set_light("i", on);
//        set_light("love (red)", on);
//        set_light("you (red)", on);
      }
    }
  }
  if(downButtonState == was_low_add_time)
    {
      downButtonState |= already_added_time;
      readDS3231time(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);
      if(minute <=4) minute += 60;
      minute -= 5;
      setDS3231time(second, minute, hour, dayOfWeek, dayOfMonth, month, year);
      parse_time();
    }

    downButtonState = 0;
    upButtonState = 0;

    if((month == 12) && (dayOfMonth == 8))
    {
      rgbLedRainbow(5000, numRGBLeds);
    }
    
    //delay(1000);
  
//  if(Serial.available()){
//    if(Serial.peek() == 'l'){
//      // Print information about the interrupt frequency, duration and load on your program
//      ShiftPWM.PrintInterruptLoad();
//    }
//    else if(Serial.peek() == 'm'){
//      // Print instructions again
//      printInstructions();
//    }
//    else{
//      fadingMode = Serial.parseInt(); // read a number from the serial port to set the mode
//      Serial.print("Mode set to "); 
//      Serial.print(fadingMode); 
//      Serial.print(": ");
//      startTime = millis();
//      switch(fadingMode){
//      case 0:
//        Serial.println("All LED's off");
//        break;
//      case 1:
//        Serial.println("Fade in and out one by one");
//        break;
//      case 2:
//        Serial.println("Fade in and out all LED's");
//        break;
//      case 3:
//        Serial.println("Fade in and out 2 LED's in parallel");
//        break;
//      case 4:
//        Serial.println("Alternating LED's in 6 different colors");
//        break;
//      case 5:
//        Serial.println("Hue shift all LED's");
//        break;
//      case 6:
//        Serial.println("Setting random LED's to random color");
//        break;
//      case 7:
//        Serial.println("Fake a VU meter");
//        break;
//      case 8:
//        Serial.println("Display a color shifting rainbow as wide as the LED's");
//        break;         
//      case 9:
//        Serial.println("Display a color shifting rainbow wider than the LED's");
//        break;     
//      default:
//        Serial.println("Unknown mode!");
//        break;
//      }
//    }
//    while (Serial.read() >= 0){
//      ; // flush remaining characters
//    }
//  }
//  switch(fadingMode){
//  case 0:
//    // Turn all LED's off.
//    ShiftPWM.SetAll(0);
//    break;
//  case 1:
//    oneByOne();
//    break;
//  case 2:
//    inOutAll();
//    break;
//  case 3:
//    inOutTwoLeds();
//    break;
//  case 4:
//    alternatingColors();
//    break;
//  case 5:
//    hueShiftAll();
//    break;
//  case 6:
//    randomColors();
//    break;
//  case 7:
//    fakeVuMeter();
//    break;
//  case 8:
//    rgbLedRainbow(3000,numRGBLeds);
//    break;
//  case 9:
//    rgbLedRainbow(10000,5*numRGBLeds);    
//    break;   
//  default:
//    Serial.println("Unknown Mode!");
//    delay(1000);
//    break;
//  }
}

void oneByOne(void){ // Fade in and fade out all outputs one at a time
  unsigned char brightness;
  unsigned long fadeTime = 500;
  unsigned long loopTime = numOutputs*fadeTime*2;
  unsigned long time = millis()-startTime;
  unsigned long timer = time%loopTime;
  unsigned long currentStep = timer%(fadeTime*2);

  int activeLED = timer/(fadeTime*2);

  if(currentStep <= fadeTime ){
    brightness = currentStep*maxBrightness/fadeTime; ///fading in
  }
  else{
    brightness = maxBrightness-(currentStep-fadeTime)*maxBrightness/fadeTime; ///fading out;
  }
  ShiftPWM.SetAll(0);
  ShiftPWM.SetOne(activeLED, brightness);
}

void inOutTwoLeds(void){ // Fade in and out 2 outputs at a time
  unsigned long fadeTime = 500;
  unsigned long loopTime = numOutputs*fadeTime;
  unsigned long time = millis()-startTime;
  unsigned long timer = time%loopTime;
  unsigned long currentStep = timer%fadeTime;

  int activeLED = timer/fadeTime;
  unsigned char brightness = currentStep*maxBrightness/fadeTime;

  ShiftPWM.SetAll(0);
  ShiftPWM.SetOne((activeLED+1)%numOutputs,brightness);
  ShiftPWM.SetOne(activeLED,maxBrightness-brightness);
}

void inOutAll(void){  // Fade in all outputs
  unsigned char brightness;
  unsigned long fadeTime = 2000;
  unsigned long time = millis()-startTime;
  unsigned long currentStep = time%(fadeTime*2);

  if(currentStep <= fadeTime ){
    brightness = currentStep*maxBrightness/fadeTime; ///fading in
  }
  else{
    brightness = maxBrightness-(currentStep-fadeTime)*maxBrightness/fadeTime; ///fading out;
  }
  ShiftPWM.SetAll(brightness);
}

void alternatingColors(void){ // Alternate LED's in 6 different colors
  unsigned long holdTime = 2000;
  unsigned long time = millis()-startTime;
  unsigned long shift = (time/holdTime)%6;
  for(unsigned int led=0; led<numRGBLeds; led++){
    switch((led+shift)%6){
    case 0:
      ShiftPWM.SetRGB(led,255,0,0);    // red
      break;
    case 1:
      ShiftPWM.SetRGB(led,0,255,0);    // green
      break;
    case 2:
      ShiftPWM.SetRGB(led,0,0,255);    // blue
      break;
    case 3:
      ShiftPWM.SetRGB(led,255,128,0);  // orange
      break;
    case 4:
      ShiftPWM.SetRGB(led,0,255,255);  // turqoise
      break;
    case 5:
      ShiftPWM.SetRGB(led,255,0,255);  // purple
      break;
    }
  }
}

void hueShiftAll(void){  // Hue shift all LED's
  unsigned long cycleTime = 10000;
  unsigned long time = millis()-startTime;
  unsigned long hue = (360*time/cycleTime)%360;
  ShiftPWM.SetAllHSV(hue, 255, 255); 
}

void randomColors(void){  // Update random LED to random color. Funky!
  unsigned long updateDelay = 100;
  static unsigned long previousUpdateTime;
  if(millis()-previousUpdateTime > updateDelay){
    previousUpdateTime = millis();
    ShiftPWM.SetHSV(random(numRGBLeds),random(360),255,255);
  }
}

void fakeVuMeter(void){ // imitate a VU meter
  static unsigned int peak = 0;
  static unsigned int prevPeak = 0;
  static unsigned long currentLevel = 0;
  static unsigned long fadeStartTime = startTime;
  
  unsigned long fadeTime = (currentLevel*2);// go slower near the top

  unsigned long time = millis()-fadeStartTime;
  currentLevel = time%(fadeTime);

  if(currentLevel==peak){
    // get a new peak value
    prevPeak = peak;
    while(abs(peak-prevPeak)<5){
      peak =  random(numRGBLeds); // pick a new peak value that differs at least 5 from previous peak
    }
  }

  if(millis() - fadeStartTime > fadeTime){
    fadeStartTime = millis();
    if(currentLevel<peak){ //fading in
      currentLevel++;
    }
    else{ //fading out
      currentLevel--;
    }
  }
  // animate to new top
  for(unsigned int led=0;led<numRGBLeds;led++){
    if(led<currentLevel){
      int hue = (numRGBLeds-1-led)*120/numRGBLeds; // From green to red
      ShiftPWM.SetHSV(led,hue,255,255); 
    }
    else if(led==currentLevel){
      int hue = (numRGBLeds-1-led)*120/numRGBLeds; // From green to red
      int value;
      if(currentLevel<peak){ //fading in        
        value = time*255/fadeTime;
      }
      else{ //fading out
        value = 255-time*255/fadeTime;
      }
      ShiftPWM.SetHSV(led,hue,255,value);       
    }
    else{
      ShiftPWM.SetRGB(led,0,0,0);
    }
  }
}

void rgbLedRainbow(unsigned long cycleTime, int rainbowWidth){
  // Displays a rainbow spread over a few LED's (numRGBLeds), which shifts in hue. 
  // The rainbow can be wider then the real number of LED's.
  unsigned long time = millis()-startTime;
  unsigned long colorShift = (360*time/cycleTime)%360; // this color shift is like the hue slider in Photoshop.

  for(unsigned int led=0;led<3;led++){ // loop over all LED's
    int hue = ((led)*360/(rainbowWidth-1)+colorShift)%360; // Set hue from 0 to 360 from first to last led and shift the hue
    hsvtorgb(&red, &green, &blue, hue, 255, 255);
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
    hsvtorgb(&red, &green, &blue, hue, 255, 255);
    switch(led){
      case 0: ShiftPWM.SetOne(37, red); // write the HSV values, with saturation and value at maximum
              ShiftPWM.SetOne(38, green); // write the HSV values, with saturation and value at maximum
              ShiftPWM.SetOne(39, blue); // write the HSV values, with saturation and value at maximum
              break;
      case 1: ShiftPWM.SetOne(40, red); // write the HSV values, with saturation and value at maximum
              ShiftPWM.SetOne(41, green); // write the HSV values, with saturation and value at maximum
              ShiftPWM.SetOne(42, blue); // write the HSV values, with saturation and value at maximum
              break;
////      case 3: ShiftPWM.SetOne(17, hue); // write the HSV values, with saturation and value at maximum
////              ShiftPWM.SetOne(20, hue); // write the HSV values, with saturation and value at maximum
////              break;
////      case 4: ShiftPWM.SetOne(18, hue); // write the HSV values, with saturation and value at maximum
////              ShiftPWM.SetOne(21, hue); // write the HSV values, with saturation and value at maximum
////              break;
////      case 5: ShiftPWM.SetOne(19, hue); // write the HSV values, with saturation and value at maximum
////              ShiftPWM.SetOne(22, hue); // write the HSV values, with saturation and value at maximum
////              break;
////      case 6: ShiftPWM.SetOne(24, hue); // write the HSV values, with saturation and value at maximum
////              break;
////      case 7: ShiftPWM.SetOne(25, hue); // write the HSV values, with saturation and value at maximum
////              break;
////      case 8: ShiftPWM.SetOne(26, hue); // write the HSV values, with saturation and value at maximum
////              break;
    }
  }
}

void printInstructions(void){
  Serial.println("---- ShiftPWM Non-blocking fades demo ----");
  Serial.println("");
  
  Serial.println("Type 'l' to see the load of the ShiftPWM interrupt (the % of CPU time the AVR is busy with ShiftPWM)");
  Serial.println("");
  Serial.println("Type any of these numbers to set the demo to this mode:");
  Serial.println("  0. All LED's off");
  Serial.println("  1. Fade in and out one by one");
  Serial.println("  2. Fade in and out all LED's");
  Serial.println("  3. Fade in and out 2 LED's in parallel");
  Serial.println("  4. Alternating LED's in 6 different colors");
  Serial.println("  5. Hue shift all LED's");
  Serial.println("  6. Setting random LED's to random color");
  Serial.println("  7. Fake a VU meter");
  Serial.println("  8. Display a color shifting rainbow as wide as the LED's");
  Serial.println("  9. Display a color shifting rainbow wider than the LED's");  
  Serial.println("");
  Serial.println("Type 'm' to see this info again");  
  Serial.println("");
  Serial.println("----");
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

void readDS3231time(byte *second,
byte *minute,
byte *hour,
byte *dayOfWeek,
byte *dayOfMonth,
byte *month,
byte *year)
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
  // retrieve data from DS3231
  readDS3231time(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month,
  &year);
  // send it to the serial monitor
  Serial.print(hour, DEC);
  // convert the byte variable to a decimal number when displayed
  Serial.print(":");
  if (minute<10)
  {
    Serial.print("0");
  }
  Serial.print(minute, DEC);
  Serial.print(":");
  if (second<10)
  {
    Serial.print("0");
  }
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
        if(text == "i") ShiftPWM.SetOne(0, 255);
        else if(text == "t") ShiftPWM.SetOne(1, 255);
        else if(text == "it")
        {
            ShiftPWM.SetOne(0, 255);
            ShiftPWM.SetOne(1, 255);      
        }
        else if(text == "is") ShiftPWM.SetOne(2, 255);
        else if(text == "ten") ShiftPWM.SetOne(3, 255);
        else if(text == "half") ShiftPWM.SetOne(4, 255);
        else if(text == "quarter") 
        {
            ShiftPWM.SetOne(5, 255);
            ShiftPWM.SetOne(6, 255);
        }
        else if(text == "twenty")
        {
            ShiftPWM.SetOne(7, 255);
            ShiftPWM.SetOne(8, 255);
        }
        else if(text == "five") ShiftPWM.SetOne(9, 255);
        else if(text == "minutes")
        {
            ShiftPWM.SetOne(10, 255);
            ShiftPWM.SetOne(11, 255);
        }
        else if(text == "happy (red)") ShiftPWM.SetOne(12, 255);
        else if(text == "happy (green)") ShiftPWM.SetOne(13, 255);
        else if(text == "happy (blue)") ShiftPWM.SetOne(14, 255);
        else if(text == "past") ShiftPWM.SetOne(15, 255);
        else if(text == "to") ShiftPWM.SetOne(16, 255);
        else if(text == "birthday (red)")
        {
            ShiftPWM.SetOne(17, 255);
            ShiftPWM.SetOne(20, 255);
        }
        else if(text == "birthday (green)") 
        {
            ShiftPWM.SetOne(18, 255);
            ShiftPWM.SetOne(21, 255);
        }
        else if(text == "birthday (blue)")
        {
            ShiftPWM.SetOne(19, 255);
            ShiftPWM.SetOne(22, 255);
        }
        else if(text == "three") ShiftPWM.SetOne(23, 255);
        else if(text == "lexie (red)") ShiftPWM.SetOne(24, 255);
        else if(text == "lexie (green)") ShiftPWM.SetOne(25, 255);
        else if(text == "lexie (blue)") ShiftPWM.SetOne(26, 255);
        else if(text == "eight") ShiftPWM.SetOne(27, 255);
        else if(text == "one") ShiftPWM.SetOne(28, 255);
        else if(text == "two") ShiftPWM.SetOne(29, 255);
        else if(text == "four") ShiftPWM.SetOne(30, 255);
        else if(text == "eleven")
        {
            ShiftPWM.SetOne(31, 255);
            ShiftPWM.SetOne(32, 255);
        }
        else if(text == "nine") ShiftPWM.SetOne(33, 255);
        else if(text == "seven") ShiftPWM.SetOne(34, 255);
        else if(text == "five") ShiftPWM.SetOne(35, 255);
        else if(text == "six") ShiftPWM.SetOne(36, 255);
        else if(text == "love (red)") ShiftPWM.SetOne(37, 255);
        else if(text == "love (green)") ShiftPWM.SetOne(38, 255);
        else if(text == "love (blue)") ShiftPWM.SetOne(39, 255);
        else if(text == "you (red)") ShiftPWM.SetOne(40, 255);
        else if(text == "you (green)") ShiftPWM.SetOne(41, 255);
        else if(text == "you (blue)") ShiftPWM.SetOne(42, 255);
        else if(text == "ten") ShiftPWM.SetOne(43, 255);
        else if(text == "twelve")
        {
            ShiftPWM.SetOne(44, 255);
            ShiftPWM.SetOne(45, 255);
        }
        else if(text == "oclock") 
        {
            ShiftPWM.SetOne(46, 255);
            ShiftPWM.SetOne(47, 255);
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
        else if(text == "five") ShiftPWM.SetOne(35, 0);
        else if(text == "six") ShiftPWM.SetOne(36, 0);
        else if(text == "love (red)") ShiftPWM.SetOne(37, 0);
        else if(text == "love (green)") ShiftPWM.SetOne(38, 0);
        else if(text == "love (blue)") ShiftPWM.SetOne(39, 0);
        else if(text == "you (red)") ShiftPWM.SetOne(40, 0);
        else if(text == "you (green)") ShiftPWM.SetOne(41, 0);
        else if(text == "you (blue)") ShiftPWM.SetOne(42, 0);
        else if(text == "ten") ShiftPWM.SetOne(43, 0);
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
    ShiftPWM.SetAll(0);
    set_light("it", on);
    set_light("is", on);
    if(minute >=0 && minute <= 2)
    {
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
              set_light("five", on);
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
              set_light("ten", on);
              break;
            case 11:
              set_light("eleven", on);
              break; 
        }
        set_light("oclock", on);
    }
    else if(minute > 2 && minute <= 7)
    {
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
              set_light("five", on);
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
              set_light("ten", on);
              break;
            case 11:
              set_light("eleven", on);
              break;
        }
        set_light("oclock", on);
    }
    else if(minute > 7 && minute <= 12)
    {
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
              set_light("five", on);
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
              set_light("ten", on);
              break;
            case 11:
              set_light("eleven", on);
              break;
        }
        set_light("oclock", on);
    }
    else if(minute > 12 && minute <= 17)
    {
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
              set_light("five", on);
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
              set_light("ten", on);
              break;
            case 11:
              set_light("eleven", on);
              break; 
        }
        set_light("oclock", on);
    }
    else if(minute > 17 && minute <= 22)
    {
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
              set_light("five", on);
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
              set_light("ten", on);
              break;
            case 11:
              set_light("eleven", on);
              break;
        }
        set_light("oclock", on);
    }
    else if(minute > 22 && minute <= 27)
    {
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
              set_light("five", on);
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
              set_light("ten", on);
              break;
            case 11:
              set_light("eleven", on);
              break; 
        }
        set_light("oclock", on);
    }
    else if(minute > 27 && minute <= 32)
    {
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
              set_light("five", on);
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
              set_light("ten", on);
              break;
            case 11:
              set_light("eleven", on);
              break;
        }
        set_light("oclock", on);
    }
    else if(minute > 32 && minute <= 37)
    {
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
              set_light("five", on);
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
              set_light("ten", on);
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
    else if(minute > 37 && minute <= 42)
    {
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
              set_light("five", on);
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
              set_light("ten", on);
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
    else if(minute > 42 && minute <= 47)
    {
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
              set_light("five", on);
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
              set_light("ten", on);
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
    else if(minute > 47 && minute <= 52)
    {
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
              set_light("five", on);
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
              set_light("ten", on);
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
    else if(minute > 52 && minute <= 57)
    {
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
              set_light("five", on);
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
              set_light("ten", on);
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
    else if(minute > 57)
    {
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
              set_light("five", on);
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
              set_light("ten", on);
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

void upPress(void)
{
//    while(upButtonPin == HIGH)
//    {
//      if(downButtonPin == HIGH)
//      {
//        ShiftPWM.SetAll(0);
//        set_light("love (red)", on);
//        set_light("you (red)", on);
//        upButtonState = love_you;
//        while(downButtonPin == HIGH) {}
//      }
//    }
//    if((upButtonPin == LOW) && (upButtonState = add_time))
//    {
//      readDS3231time(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);
//      minute += 5;
//      if(minute >= 60) minute -= 60;
//      setDS3231time(second, minute, hour, dayOfWeek, dayOfMonth, month, year);
//      parse_time();
//    }
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
