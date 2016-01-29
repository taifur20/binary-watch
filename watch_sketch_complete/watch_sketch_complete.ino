 #include <avr/interrupt.h>
 #include <avr/sleep.h>
 #include <FrequencyTimer2.h>
 #include "Wire.h"
 #define DS3231_I2C_ADDRESS 0x68
 
 #include <Button.h>        //https://github.com/JChristensen/Button

 #define BUTTON_PIN_1 2       //Connect a tactile button switch (or something similar)
 #define BUTTON_PIN_2 9                             //from Arduino pin 2 to ground.
 #define PULLUP true        //To keep things simple, we use the Arduino's internal pullup resistor.
 #define INVERT true        //Since the pullup resistor will keep the pin high unless the
                           //switch is closed, this is negative logic, i.e. a high state
                           //means the button is NOT pressed. (Assuming a normally open switch.)
 #define DEBOUNCE_MS 20     //A debounce time of 20 milliseconds usually works well for tactile button switches.


 #define LONG_PRESS 1000    //We define a "long press" to be 1000 milliseconds.


 Button myBtn1(BUTTON_PIN_1, PULLUP, INVERT, DEBOUNCE_MS);    //Declare the button
 Button myBtn2(BUTTON_PIN_2, PULLUP, INVERT, DEBOUNCE_MS); 

// Convert normal decimal numbers to binary coded decimal
byte decToBcd(byte val)
{
  return( (val/10*16) + (val%10) );
}
// Convert binary coded decimal to normal decimal numbers
byte bcdToDec(byte val)
{
  return( (val/16*10) + (val%16) );
}


byte second_unit, second_tens, minute_unit, minute_tens, hour_unit, hour_tens, 
    date_unit, date_tens, month_unit, month_tens, year_unit, year_tens, year_remain;
    
byte _second = 0, _minute = 10, _hour = 12, _day = 1, _date = 1, _month = 1, _year = 15;

#define msec  1

byte tMSB, tLSB;  // for temperature
float temp3231;
long lastPressTime = millis();  // for tracking last key press

#define space { \
    {0, 0, 0, 0, 0, 0, 0, 0},  \
    {0, 0, 0, 0, 0, 0, 0, 0}, \
    {0, 0, 0, 0, 0, 0, 0, 0}, \
    {0, 0, 0, 0, 0, 0, 0, 0}, \
    {0, 0, 0, 0, 0, 0, 0, 0}, \
    {0, 0, 0, 0, 0, 0, 0, 0}, \
    {0, 0, 0, 0, 0, 0, 0, 0}, \
    {0, 0, 0, 0, 0, 0, 0, 0} \
}

#define slash { \
    {0, 0, 0, 0, 0, 0, 0, 0},  \
    {0, 0, 0, 0, 0, 1, 0, 0}, \
    {0, 0, 0, 0, 1, 0, 0, 0}, \
    {0, 0, 0, 1, 0, 0, 0, 0}, \
    {0, 0, 1, 0, 0, 0, 0, 0}, \
    {0, 1, 0, 0, 0, 0, 0, 0}, \
    {0, 0, 0, 0, 0, 0, 0, 0}, \
    {0, 0, 0, 0, 0, 0, 0, 0} \
}

#define A { \
    {0, 0, 1, 1, 1, 0, 0, 0},  \
    {0, 1, 0, 0, 0, 1, 0, 0}, \
    {0, 1, 0, 0, 0, 1, 0, 0}, \
    {0, 1, 0, 0, 0, 1, 0, 0}, \
    {0, 1, 1, 1, 1, 1, 0, 0}, \
    {0, 1, 0, 0, 0, 1, 0, 0}, \
    {0, 1, 0, 0, 0, 1, 0, 0}, \
    {0, 0, 0, 0, 0, 0, 0, 0} \
}

#define M { \
    {0, 1, 0, 0, 0, 1, 0, 0},  \
    {0, 1, 1, 0, 1, 1, 0, 0}, \
    {0, 1, 0, 1, 0, 1, 0, 0}, \
    {0, 1, 0, 1, 0, 1, 0, 0}, \
    {0, 1, 0, 0, 0, 1, 0, 0}, \
    {0, 1, 0, 0, 0, 1, 0, 0}, \
    {0, 1, 0, 0, 0, 1, 0, 0}, \
    {0, 0, 0, 0, 0, 0, 0, 0} \
}

#define P { \
    {0, 1, 1, 1, 1, 0, 0, 0},  \
    {0, 1, 0, 0, 0, 1, 0, 0}, \
    {0, 1, 0, 0, 0, 1, 0, 0}, \
    {0, 1, 1, 1, 1, 0, 0, 0}, \
    {0, 1, 0, 0, 0, 0, 0, 0}, \
    {0, 1, 0, 0, 0, 0, 0, 0}, \
    {0, 1, 0, 0, 0, 0, 0, 0}, \
    {0, 0, 0, 0, 0, 0, 0, 0} \
}


#define zero { \
    {0, 0, 1, 1, 1, 0, 0, 0}, \
    {0, 1, 0, 0, 0, 1, 0, 0}, \
    {0, 1, 0, 0, 1, 1, 0, 0}, \
    {0, 1, 0, 1, 0, 1, 0, 0}, \
    {0, 1, 1, 0, 0, 1, 0, 0}, \
    {0, 1, 0, 0, 0, 1, 0, 0}, \
    {0, 0, 1, 1, 1, 0, 0, 0}, \
    {0, 0, 0, 0, 0, 0, 0, 0}  \
}

#define one { \
    {0, 0, 0, 1, 0, 0, 0, 0}, \
    {0, 0, 1, 1, 0, 0, 0, 0}, \
    {0, 0, 0, 1, 0, 0, 0, 0}, \
    {0, 0, 0, 1, 0, 0, 0, 0}, \
    {0, 0, 0, 1, 0, 0, 0, 0}, \
    {0, 0, 0, 1, 0, 0, 0, 0}, \
    {0, 0, 1, 1, 1, 0, 0, 0}, \
    {0, 0, 0, 0, 0, 0, 0, 0}  \
}

#define two { \
    {0, 0, 1, 1, 1, 0, 0, 0}, \
    {0, 1, 0, 0, 0, 1, 0, 0}, \
    {0, 0, 0, 0, 0, 1, 0, 0}, \
    {0, 0, 0, 0, 1, 0, 0, 0}, \
    {0, 0, 0, 1, 0, 0, 0, 0}, \
    {0, 0, 1, 0, 0, 0, 0, 0}, \
    {0, 1, 1, 1, 1, 1, 0, 0}, \
    {0, 0, 0, 0, 0, 0, 0, 0}  \
}

#define three { \
    {0, 1, 1, 1, 1, 1, 0, 0}, \
    {0, 0, 0, 0, 1, 0, 0, 0}, \
    {0, 0, 0, 1, 0, 0, 0, 0}, \
    {0, 0, 0, 0, 1, 0, 0, 0}, \
    {0, 0, 0, 0, 0, 1, 0, 0}, \
    {0, 1, 0, 0, 0, 1, 0, 0}, \
    {0, 0, 1, 1, 1, 0, 0, 0}, \
    {0, 0, 0, 0, 0, 0, 0, 0}  \
}

#define four { \
    {0, 0, 0, 0, 1, 0, 0, 0}, \
    {0, 0, 0, 1, 1, 0, 0, 0}, \
    {0, 0, 1, 0, 1, 0, 0, 0}, \
    {0, 1, 0, 0, 1, 0, 0, 0}, \
    {0, 1, 1, 1, 1, 1, 0, 0}, \
    {0, 0, 0, 0, 1, 0, 0, 0}, \
    {0, 0, 0, 0, 1, 0, 0, 0}, \
    {0, 0, 0, 0, 0, 0, 0, 0}  \
}

#define five { \
    {0, 1, 1, 1, 1, 1, 0, 0}, \
    {0, 1, 0, 0, 0, 0, 0, 0}, \
    {0, 1, 1, 1, 1, 0, 0, 0}, \
    {0, 0, 0, 0, 0, 1, 0, 0}, \
    {0, 0, 0, 0, 0, 1, 0, 0}, \
    {0, 1, 0, 0, 0, 1, 0, 0}, \
    {0, 0, 1, 1, 1, 0, 0, 0}, \
    {0, 0, 0, 0, 0, 0, 0, 0}  \
}

#define six { \
    {0, 0, 0, 1, 1, 0, 0, 0}, \
    {0, 0, 1, 0, 0, 0, 0, 0}, \
    {0, 1, 0, 0, 0, 0, 0, 0}, \
    {0, 1, 1, 1, 1, 0, 0, 0}, \
    {0, 1, 0, 0, 0, 1, 0, 0}, \
    {0, 1, 0, 0, 0, 1, 0, 0}, \
    {0, 0, 1, 1, 1, 0, 0, 0}, \
    {0, 0, 0, 0, 0, 0, 0, 0}  \
}

#define seven { \
    {0, 1, 1, 1, 1, 1, 0, 0}, \
    {0, 0, 0, 0, 0, 1, 0, 0}, \
    {0, 0, 0, 0, 1, 0, 0, 0}, \
    {0, 0, 0, 1, 0, 0, 0, 0}, \
    {0, 0, 1, 0, 0, 0, 0, 0}, \
    {0, 0, 1, 0, 0, 0, 0, 0}, \
    {0, 0, 1, 0, 0, 0, 0, 0}, \
    {0, 0, 0, 0, 0, 0, 0, 0}  \
}

#define eight { \
    {0, 0, 1, 1, 1, 0, 0, 0}, \
    {0, 1, 0, 0, 0, 1, 0, 0}, \
    {0, 1, 0, 0, 0, 1, 0, 0}, \
    {0, 0, 1, 1, 1, 0, 0, 0}, \
    {0, 1, 0, 0, 0, 1, 0, 0}, \
    {0, 1, 0, 0, 0, 1, 0, 0}, \
    {0, 0, 1, 1, 1, 0, 0, 0}, \
    {0, 0, 0, 0, 0, 0, 0, 0}  \
}

#define nine { \
    {0, 0, 1, 1, 1, 0, 0, 0}, \
    {0, 1, 0, 0, 0, 1, 0, 0}, \
    {0, 1, 0, 0, 0, 1, 0, 0}, \
    {0, 0, 1, 1, 1, 1, 0, 0}, \
    {0, 0, 0, 0, 0, 1, 0, 0}, \
    {0, 0, 0, 0, 1, 0, 0, 0}, \
    {0, 0, 1, 1, 0, 0, 0, 0}, \
    {0, 0, 0, 0, 0, 0, 0, 0}  \
}

#define colon { \
    {0, 0, 0, 0, 0, 0, 0, 0}, \
    {0, 0, 1, 1, 0, 0, 0, 0}, \
    {0, 0, 1, 1, 0, 0, 0, 0}, \
    {0, 0, 0, 0, 0, 0, 0, 0}, \
    {0, 0, 1, 1, 0, 0, 0, 0}, \
    {0, 0, 1, 1, 0, 0, 0, 0}, \
    {0, 0, 0, 0, 0, 0, 0, 0}, \
    {0, 0, 0, 0, 0, 0, 0, 0}  \
}

#define DEGC { \
    {1, 1, 0, 0, 1, 1, 1, 0}, \
    {1, 1, 0, 1, 0, 0, 0, 1}, \
    {0, 0, 0, 1, 0, 0, 0, 0}, \
    {0, 0, 0, 1, 0, 0, 0, 0}, \
    {0, 0, 0, 1, 0, 0, 0, 0}, \
    {0, 0, 0, 1, 0, 0, 0, 1}, \
    {0, 0, 0, 0, 1, 1, 1, 0}, \
    {0, 0, 0, 0, 0, 0, 0, 0}  \
}

#define T { \
    {0, 1, 1, 1, 1, 1, 0, 0}, \
    {0, 0, 0, 1, 0, 0, 0, 0}, \
    {0, 0, 0, 1, 0, 0, 0, 0}, \
    {0, 0, 0, 1, 0, 0, 0, 0}, \
    {0, 0, 0, 1, 0, 0, 0, 0}, \
    {0, 0, 0, 1, 0, 0, 0, 0}, \
    {0, 0, 0, 1, 0, 0, 0, 0}, \
    {0, 0, 0, 0, 0, 0, 0, 0}  \
}

#define smallm { \
    {0, 0, 0, 0, 0, 0, 0, 0}, \
    {0, 0, 0, 0, 0, 0, 0, 0}, \
    {0, 1, 1, 0, 1, 0, 0, 0}, \
    {0, 1, 0, 1, 0, 1, 0, 0}, \
    {0, 1, 0, 1, 0, 1, 0, 0}, \
    {0, 1, 0, 0, 0, 1, 0, 0}, \
    {0, 1, 0, 0, 0, 1, 0, 0}, \
    {0, 0, 0, 0, 0, 0, 0, 0}  \
}

#define smallp { \
    {0, 0, 0, 0, 0, 0, 0, 0}, \
    {0, 0, 0, 0, 0, 0, 0, 0}, \
    {0, 1, 1, 1, 1, 0, 0, 0}, \
    {0, 1, 0, 0, 0, 1, 0, 0}, \
    {0, 1, 1, 1, 1, 0, 0, 0}, \
    {0, 1, 0, 0, 0, 0, 0, 0}, \
    {0, 1, 0, 0, 0, 0, 0, 0}, \
    {0, 0, 0, 0, 0, 0, 0, 0}  \
}


byte col = 0;
byte leds[8][8];

byte rows[8] = {0, 3, 6, 12, 7, 14, 15, 4};
byte cols[8] = {8, 1, 10, 5, 17, 11, 16, 13};

byte colPin[8] = {4, 15, 14, 7, 12, 6, 3, 0}; //-ve pin
byte rowPin[8] = {8, 1, 10, 5, 17, 11, 16, 13}; //+ve

const byte numPatterns = 14;
byte patterns[numPatterns][8][8] = {

  zero, one, two, three, four, five, six, seven, eight, nine, colon, space, slash, DEGC

};
int pattern = 0;

//long timeSpand;

byte buttonStateOne = 0, buttonLongStateOne = 0, buttonStateTwo = 0, buttonLongStateTwo = 0;

byte clockMode = 0;
byte displayOnOff = 1;


void setup() {
 Wire.begin();

  // sets the pins as output
 for (byte i = 3; i <= 8; i++) {
   
    pinMode(i, OUTPUT);
  }
 for (byte i = 10; i <= 17; i++) {
   
    pinMode(i, OUTPUT);
  }
  
 pinMode(0, OUTPUT);
 pinMode(1, OUTPUT);
 
 
    // set up cols and rows
 for (byte i = 1; i <= 8; i++) {
    digitalWrite(colPin[i - 1], HIGH);
  }

 for (byte i = 1; i <= 8; i++) {
    digitalWrite(rowPin[i - 1], LOW);
  }

  clearLeds();


  // Turn off toggling of pin 11

  FrequencyTimer2::disable();

  // Set refresh rate (interrupt timeout period)

  FrequencyTimer2::setPeriod(2000);

  // Set interrupt routine to be called

  FrequencyTimer2::setOnOverflow(display);


  setPattern(pattern);
  
  attachInterrupt(0,wakeUpNow, LOW);

}

void loop() {
    calculateDateTime();
    button();
    if(clockMode == 1 && buttonLongStateTwo == 0){
       digitalClock();
       clockMode = 0;
    }
    else if(buttonLongStateTwo == 0 && clockMode == 0)
      binaryClock();
     
    else
      editDisplay();
      
  if(millis()-lastPressTime > 60000){
     sleepNow();
  }
        
}

void clearLeds() {
  // Clear display array
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      leds[i][j] = 0;
    }
  }
}


void setPattern(int pattern) {
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      leds[i][j] = patterns[pattern][i][j];
    }
  }
}


void slidePattern(int pattern, int del) {
  for (int l = 0; l < 8; l++) {
    for (int i = 0; i < 7; i++) {
      for (int j = 0; j < 8; j++) {
        leds[j][i] = leds[j][i+1];
         }
    }
    for (int j = 0; j < 8; j++) {
      leds[j][7] = patterns[pattern][j][0 + l];
    }
    delay(del);
  }
}

// Interrupt routine

void display() {
  digitalWrite(cols[col], LOW);  // Turn whole previous column off
  col++;
  
  //digitalWrite(rows[row], LOW);  // Turn whole previous column off
  //row++;
  
  if (col == 8) {
    col = 0;
  }

  for (int row = 0; row < 8; row++) {
    if (leds[col][7 - row] == 1) {
      digitalWrite(rows[row], LOW);  // Turn on this led
    }
    else {
      digitalWrite(rows[row], HIGH); // Turn off this led
    }
  }
  digitalWrite(cols[col], HIGH); // Turn whole column on at once (for equal lighting times)
}


void setDS3231time(byte second, byte minute, byte hour, byte dayOfWeek, byte
dayOfMonth, byte month, byte year)
{
  // sets time and date data to DS3231
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(0); // set next input to start at the seconds register
  Wire.write(decToBcd(second)); // set seconds
  Wire.write(decToBcd(minute)); // set minutes
  Wire.write(decToBcd(hour | 0x40)); // set hours
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
  *hour = bcdToDec(Wire.read() & 0x1f);
  *dayOfWeek = bcdToDec(Wire.read());
  *dayOfMonth = bcdToDec(Wire.read());
  *month = bcdToDec(Wire.read());
  *year = bcdToDec(Wire.read());
}

void calculateDateTime(){
  byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;
  // retrieve data from DS3231
  readDS3231time(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month,
  &year);
  
  if(hour > 12){
    hour = hour - 24;   
  }
  
  second_unit = second % 10;
  second_tens = second / 10;
  
  minute_unit = minute % 10;
  minute_tens = minute / 10;
  
  hour_unit = hour % 10;
  hour_tens = hour / 10;
  
  date_unit = dayOfMonth % 10;
  date_tens = dayOfMonth / 10;
  
  month_unit = month % 10;
  month_tens = month / 10;
  
  year_unit = year % 10;
  year_tens = year / 10;
}

void digitalClock(){
  FrequencyTimer2::setOnOverflow(display);
  slidePattern(hour_tens, 80);
  slidePattern(hour_unit, 80);
  slidePattern(10, 80);
  slidePattern(minute_tens, 80);
  slidePattern(minute_unit, 80);
  slidePattern(10, 80);
  slidePattern(second_tens, 80);
  slidePattern(second_unit, 80);
  //slidePattern(11, 80);
  slidePattern(11, 80);
  
  slidePattern(date_tens, 80);
  slidePattern(date_unit, 80);
  slidePattern(12, 80);
  slidePattern(month_tens, 80);
  slidePattern(month_unit, 80);
  slidePattern(12, 80);
  slidePattern(2, 80);
  slidePattern(0, 80);
  slidePattern(year_tens, 80);
  slidePattern(year_unit, 80);

  slidePattern(11, 80);
  int tempC = get3231Temp();
  int tempC_unit = tempC % 10;
  int tempC_tens = tempC / 10;
  
  slidePattern(tempC_tens, 80);
  slidePattern(tempC_unit, 80);
  slidePattern(13, 80);
  
  slidePattern(11, 80);
  slidePattern(11, 80);
}

void time_zero(int pos){
  delay(msec);  
}

void time_one(int pos){
  digitalWrite(colPin[pos], LOW);
  digitalWrite(rowPin[7], HIGH);
  delay(msec);
  digitalWrite(colPin[pos], HIGH);
  digitalWrite(rowPin[7], LOW);
}

void time_two(int pos){
  digitalWrite(colPin[pos], LOW);
  digitalWrite(rowPin[6], HIGH);
  delay(msec);
  digitalWrite(colPin[pos], HIGH);
  digitalWrite(rowPin[6], LOW);
}

void time_three(int pos){
  digitalWrite(colPin[pos], LOW);
  digitalWrite(rowPin[6], HIGH);
  digitalWrite(rowPin[7], HIGH);
  delay(msec);
  digitalWrite(colPin[pos], HIGH);
  digitalWrite(rowPin[6], LOW);
  digitalWrite(rowPin[7], LOW);
}

void time_four(int pos){
  digitalWrite(colPin[pos], LOW);
  digitalWrite(rowPin[5], HIGH);
  delay(msec);
  digitalWrite(colPin[pos], HIGH);
  digitalWrite(rowPin[5], LOW);
}

void time_five(int pos){
  digitalWrite(colPin[pos], LOW);
  digitalWrite(rowPin[5], HIGH);
  digitalWrite(rowPin[7], HIGH);
  delay(msec);
  digitalWrite(colPin[pos], HIGH);
  digitalWrite(rowPin[5], LOW);
  digitalWrite(rowPin[7], LOW);
}

void time_six(int pos){
  digitalWrite(colPin[pos], LOW);
  digitalWrite(rowPin[5], HIGH);
  digitalWrite(rowPin[6], HIGH);
  delay(msec);
  digitalWrite(colPin[pos], HIGH);
  digitalWrite(rowPin[5], LOW);
  digitalWrite(rowPin[6], LOW);
}

void time_seven(int pos){
  digitalWrite(colPin[pos], LOW);
  digitalWrite(rowPin[5], HIGH);
  digitalWrite(rowPin[6], HIGH);
  digitalWrite(rowPin[7], HIGH);
  delay(msec);
  digitalWrite(colPin[pos], HIGH);
  digitalWrite(rowPin[5], LOW);
  digitalWrite(rowPin[6], LOW);
  digitalWrite(rowPin[7], LOW);
}

void time_eight(int pos){
  digitalWrite(colPin[pos], LOW);
  digitalWrite(rowPin[4], HIGH);
  delay(msec);
  digitalWrite(colPin[pos], HIGH);
  digitalWrite(rowPin[4], LOW);
}

void time_nine(int pos){
  digitalWrite(colPin[pos], LOW);
  digitalWrite(rowPin[4], HIGH);
  digitalWrite(rowPin[7], HIGH);
  delay(msec);
  digitalWrite(colPin[pos], HIGH);
  digitalWrite(rowPin[4], LOW);
  digitalWrite(rowPin[7], LOW);
}

void date_zero(int pos){
  delay(msec);  
}

void date_one(int pos){
  digitalWrite(colPin[pos], LOW);
  digitalWrite(rowPin[7-4], HIGH);
  delay(msec);
  digitalWrite(colPin[pos], HIGH);
  digitalWrite(rowPin[7-4], LOW);
}

void date_two(int pos){
  digitalWrite(colPin[pos], LOW);
  digitalWrite(rowPin[6-4], HIGH);
  delay(msec);
  digitalWrite(colPin[pos], HIGH);
  digitalWrite(rowPin[6-4], LOW);
}

void date_three(int pos){
  digitalWrite(colPin[pos], LOW);
  digitalWrite(rowPin[6-4], HIGH);
  digitalWrite(rowPin[7-4], HIGH);
  delay(msec);
  digitalWrite(colPin[pos], HIGH);
  digitalWrite(rowPin[6-4], LOW);
  digitalWrite(rowPin[7-4], LOW);
}

void date_four(int pos){
  digitalWrite(colPin[pos], LOW);
  digitalWrite(rowPin[5-4], HIGH);
  delay(msec);
  digitalWrite(colPin[pos], HIGH);
  digitalWrite(rowPin[5-4], LOW);
}

void date_five(int pos){
  digitalWrite(colPin[pos], LOW);
  digitalWrite(rowPin[5-4], HIGH);
  digitalWrite(rowPin[7-4], HIGH);
  delay(msec);
  digitalWrite(colPin[pos], HIGH);
  digitalWrite(rowPin[5-4], LOW);
  digitalWrite(rowPin[7-4], LOW);
}

void date_six(int pos){
  digitalWrite(colPin[pos], LOW);
  digitalWrite(rowPin[5-4], HIGH);
  digitalWrite(rowPin[6-4], HIGH);
  delay(msec);
  digitalWrite(colPin[pos], HIGH);
  digitalWrite(rowPin[5-4], LOW);
  digitalWrite(rowPin[6-4], LOW);
}

void date_seven(int pos){
  digitalWrite(colPin[pos], LOW);
  digitalWrite(rowPin[5-4], HIGH);
  digitalWrite(rowPin[6-4], HIGH);
  digitalWrite(rowPin[7-4], HIGH);
  delay(msec);
  digitalWrite(colPin[pos], HIGH);
  digitalWrite(rowPin[5-4], LOW);
  digitalWrite(rowPin[6-4], LOW);
  digitalWrite(rowPin[7-4], LOW);
}

void date_eight(int pos){
  digitalWrite(colPin[pos], LOW);
  digitalWrite(rowPin[4-4], HIGH);
  delay(msec);
  digitalWrite(colPin[pos], HIGH);
  digitalWrite(rowPin[4-4], LOW);
}

void date_nine(int pos){
  digitalWrite(colPin[pos], LOW);
  digitalWrite(rowPin[4-4], HIGH);
  digitalWrite(rowPin[7-4], HIGH);
  delay(msec);
  digitalWrite(colPin[pos], HIGH);
  digitalWrite(rowPin[4-4], LOW);
  digitalWrite(rowPin[7-4], LOW);
}


void show(int digit, int pos){
  switch(digit){
     case 0:
     time_zero(pos);
     break;
     
     case 1:
     time_one(pos);
     break;
     
     case 2:
     time_two(pos);
     break;
     
     case 3:
     time_three(pos);
     break;
     
     case 4:
     time_four(pos);
     break;
     
     case 5:
     time_five(pos);
     break;
     
     case 6:
     time_six(pos);
     break;
     
     case 7:
     time_seven(pos);
     break;
     
     case 8:
     time_eight(pos);
     break;
     
     case 9:
     time_nine(pos);
     break;  
  }

}

void date_show(int digit, int pos){
  switch(digit){
     case 0:
     date_zero(pos);
     break;
     
     case 1:
     date_one(pos);
     break;
     
     case 2:
     date_two(pos);
     break;
     
     case 3:
     date_three(pos);
     break;
     
     case 4:
     date_four(pos);
     break;
     
     case 5:
     date_five(pos);
     break;
     
     case 6:
     date_six(pos);
     break;
     
     case 7:
     date_seven(pos);
     break;
     
     case 8:
     date_eight(pos);
     break;
     
     case 9:
     date_nine(pos);
     break;  
  }

}

void binaryClock(){
  FrequencyTimer2::setOnOverflow(0);
  show(second_unit, 7);
  show(second_tens, 6);
  
  show(minute_unit, 4);
  show(minute_tens, 3);
  
  show(hour_unit, 1);
  show(hour_tens, 0); 
 
  date_show(date_unit, 7);
  date_show(date_tens, 6);
 
  date_show(month_unit, 4);
  date_show(month_tens, 3);
 
  date_show(year_unit, 1);
  date_show(year_tens, 0); 
}

void showMinute(){
  int _minute_unit = _minute % 10;
  int _minute_tens = _minute / 10;
  show(_minute_unit, 4);
  show(_minute_tens, 3);
}

void showHour(){
  int _hour_unit = _hour % 10;
  int _hour_tens = _hour / 10;
  show(_hour_unit, 1);
  show(_hour_tens, 0); 
}
void showDate(){
  int _date_unit = _date % 10;
  int _date_tens = _date / 10;
  date_show(_date_unit, 7);
  date_show(_date_tens, 6);
}

void showMonth(){
  int _month_unit = _month % 10;
  int _month_tens = _month / 10;
  date_show(_month_unit, 4);
  date_show(_month_tens, 3);
}
void showYear(){
  int _year_unit = _year % 10;
  int _year_tens = _year / 10;
  date_show(_year_unit, 1);
  date_show(_year_tens, 0); 
}

 void button(){
    myBtn1.read();                //Read the button
    myBtn2.read();                //Read the button
                 
    if (myBtn1.wasReleased()){
         lastPressTime = millis();
         if(buttonLongStateTwo == 0){           
           clockMode = 1;
           //timeSpand = millis();
           }
         else if((buttonLongStateTwo == 1) && (buttonStateTwo == 1))   
            {
              showMinute();
              _minute++;
              delay(20);
              if(_minute > 59)
              _minute = 0;
            //Serial.println(minute);
            }
         else if((buttonLongStateTwo == 1) && (buttonStateTwo == 2))   
            {
              showHour();
              _hour++;
              delay(20);
              if(_hour > 12)
              _hour = 1;
            //Serial.println(hour);
            }
         else if((buttonLongStateTwo == 1) && (buttonStateTwo == 3))   
            {
              showDate();
              _date++;
              delay(20);
              if(_date > 31)
              _date = 1;
            //Serial.println(date);
            }
         else if((buttonLongStateTwo == 1) && (buttonStateTwo == 4))   
            {
              showMonth();
              _month++;
              delay(20);
              if(_month > 12)
              _month = 1;
           // Serial.println(month);
            }
         else if((buttonLongStateTwo == 1) && (buttonStateTwo == 5))   
            {
              showYear();
              _year++;
              if(_year > 99)
              _year = 15;
              delay(20);
             //Serial.println(year);
            }
        
        }
         
    else if (myBtn1.pressedFor(LONG_PRESS)){
         lastPressTime = millis();
         setDS3231time(_second,_minute,_hour,1,_date,_month,_year);
         
         //Serial.println("date set");
         buttonLongStateTwo = 0;
         //Serial.println(buttonLongStateTwo);
         delay(100);
       }
         
   else if (myBtn2.wasReleased()){
        lastPressTime = millis();
        if(buttonLongStateTwo == 0){
     
          }
         else if(buttonLongStateTwo == 1){ 
            buttonStateTwo++;
            if(buttonStateTwo > 5)
             buttonStateTwo = 1;
         }
          if((buttonStateTwo == 1)&&(buttonLongStateTwo == 1)){
            showMinute();
            delay(20);
            //Serial.println("display minute");
            }
          if((buttonStateTwo == 2)&&(buttonLongStateTwo == 1)){
            //Serial.println("display hour");
            showHour();
            delay(20);
            }
          if((buttonStateTwo == 3)&&(buttonLongStateTwo == 1)){
            //Serial.println("dispaly date");
            showDate();
            delay(20);
          }
          if((buttonStateTwo == 4)&&(buttonLongStateTwo == 1)){
            //Serial.println("display month");
            showMonth();
            delay(20);
          }
          if((buttonStateTwo == 5)&&(buttonLongStateTwo == 1)){
            //Serial.println("display year");
            showYear(); 
            delay(20);   
          }
        
       }
    else if (myBtn2.pressedFor(LONG_PRESS)){
         lastPressTime = millis();
         readDS3231time(&_second, &_minute, &_hour, &_day, &_date, &_month,
  &_year);
         
         buttonLongStateTwo = 1;
         //Serial.println("editing mode");
         delay(10);
   }


}


void editDisplay(){
         if((buttonStateTwo == 1)&&(buttonLongStateTwo == 1)){
            showMinute();
            
            //Serial.println("display minute");
            }
        if((buttonStateTwo == 2)&&(buttonLongStateTwo == 1)){
            //Serial.println("display hour");
            showHour();
          
            }
        if((buttonStateTwo == 3)&&(buttonLongStateTwo == 1)){
            //Serial.println("dispaly date");
            showDate();
           
          }
        if((buttonStateTwo == 4)&&(buttonLongStateTwo == 1)){
            //Serial.println("display month");
            showMonth();
            
          }
        if((buttonStateTwo == 5)&&(buttonLongStateTwo == 1)){
            //Serial.println("display year");
            showYear(); 
          
          }
        
  }
  
  
int get3231Temp()
 {
  //temp registers (11h-12h) get updated automatically every 64s
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(0x11);
  Wire.endTransmission();
  Wire.requestFrom(DS3231_I2C_ADDRESS, 2);
 
  if(Wire.available()) {
    tMSB = Wire.read(); //2's complement int portion
    tLSB = Wire.read(); //fraction portion
   
    temp3231 = (tMSB & B01111111); //do 2's math on Tmsb
    
  }
  else {
    //oh noes, no data!
  }
   
  return temp3231;
}

void sleepNow()         // here we put the arduino to sleep
{
      
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);   // sleep mode is set here

    sleep_enable();              // enables the sleep bit in the mcucr register
                                 // so sleep is possible. just a safety pin 

    attachInterrupt(0,wakeUpNow, LOW); // use interrupt 0 (pin 2) and run function
                                       // wakeUpNow when pin 2 gets LOW 
                            
    sleep_mode();                // here the device is actually put to sleep!!
                                 // 


    sleep_disable();             // first thing after waking from sleep:
                                 // disable sleep...
    lastPressTime = millis();
    detachInterrupt(0);          // disables interrupt 0 on pin 2 so the 
                                 // wakeUpNow code will not be executed 
                                 // during normal running time.
                                 // wat 2 sec. so humans can notice the
                                 // interrupt. 
                                 // LED to show the interrupt is handled
 
}

void wakeUpNow()        // here the interrupt is handled after wakeup
{
  //execute code here after wake-up before returning to the loop() function
  // timers and code using timers (serial.print and more...) will not work here.
 
}
 
