/*
Project: Quincy Controller
Author:Jay Herron 
Date: 2019-12-21
 
Use this sketch to read the temperature from 1-Wire devices
you have attached to your Particle device (core, p0, p1, photon, electron)

Temperature is read from: DS18S20, DS18B20, DS1822, DS2438

I/O setup:
These made it easy to just 'plug in' my 18B20

D3 - 1-wire ground, or just use regular pin and comment out below.
D4 - 1-wire signal, 2K-10K resistor to D5 (3v3)
D5 - 1-wire power, ditto ground comment.

A pull-up resistor is required on the signal line. The spec calls for a 4.7K.
I have used 1K-10K depending on the bus configuration and what I had out on the
bench. If you are powering the device, they all work. If you are using parasitic
power it gets more picky about the value.

*/

#include <stdlib.h> // Used for String to int/double conversions
#include "DS18.h"


static DS18 ds18 = DS18(D4); 

// Internal variables
const int lightPin = A0;
const int heatPin = A1;
const int dataInterval = 5; // in seconds
long dataReadTime;
boolean tempReadingStarted = false;

// Set default variables
const double tempSpDayDefault = 83; // 85 is ideal for highest temp at basking spot
const double tempSpNightDefault = 70; // Drop of 10-15 degrees is ideal. Temps in 60s are ok.
const int hourDayStartDefault = 8;
const int hourDayEndDefault = 20; // 8PM

// Set EEPROM memory addresses so that settings are persistent across restarts.
const int tempSpDayAddr = 0; // double size = 8 bytes
const int tempSpNightAddr = 8; // double size = 8 bytes
const int hourDayStartAddr = 16; // int size = 4 bytes
const int hourDayEndAddr = 20; // int size = 4 bytes

// Particle variables
double temp;
double tempSp;
double tempSpDay;
double tempSpNight;
double tempDeadband = 2; // This is the +/- on the setpoint control
int hourDayStart;
int hourDayEnd;
boolean lightStatus = false;
boolean heatStatus = false;

// Particle functions
int setTempSpDay(String command);
int setTempSpNight(String command);
int setHourDayStart(String command);
int setHourDayEnd(String command);

void setup() {
  Serial.begin(9600);
  
  // Set up relay pins
  pinMode(lightPin, OUTPUT);
  pinMode(heatPin, OUTPUT);

  Time.zone(-6); // Mountain Daylight timezone

  // Read EEPROM values and set to default if needed
  // THIS DOESN'T WORK BECAUSE EEPROM VALUES ARE INTERPRETED AS UNSIGNED INTEGERS... See: https://forum.arduino.cc/index.php?topic=41497.0
  // However, once set then they read off fine, so it's not really important after I set them once...
  EEPROM.get(tempSpDayAddr, tempSpDay);
  if(tempSpDay == 0xFFFFFFFF) tempSpDay = tempSpDayDefault;
  EEPROM.get(tempSpNightAddr, tempSpNight);
  if(tempSpNight == 0xFFFFFFFF) tempSpNight = tempSpNightDefault;
  EEPROM.get(hourDayStartAddr, hourDayStart);
  if(hourDayStart == 0xFFFF) hourDayStart = hourDayStartDefault;
  EEPROM.get(hourDayEndAddr, hourDayEnd);
  if(hourDayEnd == 0xFFFF) hourDayEnd = hourDayEndDefault;
  

  // Declare particle variables
  Particle.variable("temp", temp);
  Particle.variable("tempSp", tempSp);
  Particle.variable("tempSpDay", tempSpDay);
  Particle.variable("tempSpNight", tempSpNight);
  Particle.variable("hourDayStart", hourDayStart);
  Particle.variable("hourDayEnd", hourDayEnd);
  Particle.variable("lightStatus", lightStatus);
  Particle.variable("heatStatus", heatStatus);

  // Declare functions
  Particle.function("setTempSpDay", setTempSpDay);
  Particle.function("setTempSpNight", setTempSpNight);
  Particle.function("setHourDayStart", setHourDayStart);
  Particle.function("setHourDayEnd", setHourDayEnd);
  
  dataReadTime = Time.now();
}

void loop(void) {
  if(hourDayStart <= Time.hour() && Time.hour() < hourDayEnd){ // Daytime
    if(!lightStatus) { // Turn on light
      digitalWrite(lightPin, HIGH);
      lightStatus = true;
    }
    if(tempSp != tempSpDay) // Set tempSp to tempSpDay
      tempSp = tempSpDay;
  }
  else { // Nighttime
    if(lightStatus) { // Turn off light
      digitalWrite(lightPin, LOW);
      lightStatus = false;
    }
    // Set tempSp to tempSpNight
    if(tempSp != tempSpNight) // Set tempSp to tempSpDay
      tempSp = tempSpNight;
  }


  // Only read data on correct intervals
  if(Time.now() - dataReadTime > dataInterval) {
    // Read temp sensor
    if(ds18.read()){
      temp = ds18.fahrenheit();
      //Serial.print("Temperature: "); Serial.println(temperature, 2);

      // Control heat to maintain temp at tempSp
      if(temp > (tempSp + tempDeadband) && heatStatus) { // Too high, turn off heat.
        digitalWrite(heatPin, LOW);
        heatStatus = false;
      }
      else if(temp < (tempSp - tempDeadband) && !heatStatus) { // Too low, turn on heat.
        digitalWrite(heatPin, HIGH);
        heatStatus = true;
      }

      dataReadTime = Time.now();
    }
  }
}


// PARTICLE FUNCTIONS

// Sets the tempSpDay variable
int setTempSpDay(String command)
{
  double newSp = atof(command); // Converts a Str to a double
  if(newSp == 0.0) return -1; // atof returns 0.0 if its not a valid conversion. We shouldn't ever want a temp of 0 anyway...
  else {
    tempSpDay = newSp;
    EEPROM.put(tempSpDayAddr, tempSpDay);
    return 1;
  }
}

// Sets the tempSpNight variable
int setTempSpNight(String command)
{
  double newSp = atof(command);
  if(newSp == 0.0) return -1;
  else {
    tempSpNight = newSp;
    EEPROM.put(tempSpNightAddr, tempSpNight);
    return 1;
  }
}

// Sets the hourDayStart variable
int setHourDayStart(String command)
{
  int newHour = atoi(command); // Converts a Str to an int
  if(newHour < 0 || newHour > 23 || newHour > hourDayEnd) return -1; // Don't allow settings that aren't 0-23 or are greater than hourDayEnd
  else {
    hourDayStart = newHour;
    EEPROM.put(hourDayStartAddr, hourDayStart);
    return 1;
  }
}

// Sets the hourDayEnd variable
int setHourDayEnd(String command)
{
  int newHour = atoi(command); // Converts a Str to an int
  if(newHour < 0 || newHour > 23 || newHour < hourDayStart) return -1; // Don't allow settings that aren't 0-23 or are less than hourDayStart
  else {
    hourDayEnd = newHour;
    EEPROM.put(hourDayEndAddr, hourDayEnd);
    return 1;
  }
}