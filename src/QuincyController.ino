/*
Project: Quincy Controller
Author:Jay Herron 
Date: 2019-12-21

See readme for details

*/

#include <stdlib.h> // Used for String to int/double conversions
#include "adafruit-sht31.h"

static Adafruit_SHT31 sht31 = Adafruit_SHT31();

// Internal variables
const int lightOnPin = D8;
const int lightOffPin = D7;
const int heatOnPin = D6;
const int heatOffPin = D5;
long dataFetchedAt;
const int dataFetchInterval = 5; // in seconds
long timeSyncedAt;
const int timeSyncInterval = 24 * 60 * 60; // 1 day in seconds
boolean tempReadingStarted = false;

// Set default adjustible variables
const double tempSpDayDefault = 83; // 85 is ideal for highest temp at basking spot
const double tempSpNightDefault = 70; // Drop of 10-15 degrees is ideal. Temps in 60s are ok.
const int hourDayStartDefault = 8;
const int hourDayEndDefault = 20; // 8PM

// Set EEPROM memory addresses so that settings are persistent across restarts.
const int tempSpDayAddr = 0; // double size = 8 bytes
const int tempSpNightAddr = 8; // double size = 8 bytes
const int hourDayStartAddr = 16; // int size = 4 bytes
const int hourDayEndAddr = 20; // int size = 4 bytes

// Particle variables/functions
double temp;
double humidity;
double tempSp;
double tempSpDay;
double tempSpNight;
double tempDeadband = 2; // This is the +/- on the setpoint control
int hourDayStart;
int hourDayEnd;
int lightStatus = 0; // We do this as an int b/c particle variables can only be int, double, or String
int heatStatus = 0;
int setTempSpDay(String command);
int setTempSpNight(String command);
int setHourDayStart(String command);
int setHourDayEnd(String command);

void setup() {
  Serial.begin(9600);
  
  pinMode(lightOnPin, OUTPUT);
  pinMode(lightOffPin, OUTPUT);
  pinMode(heatOnPin, OUTPUT);
  pinMode(heatOffPin, OUTPUT);

  if (!sht31.begin(0x44)) {
    Serial.println("Couldn't find SHT31");
  }

  Time.zone(-6); // Mountain Daylight timezone

  // Read stored values and set to default if needed
  // This doesn't work because EEPROM values are interpreted as unsigned integers... See: https://forum.arduino.cc/index.php?topic=41497.0
  // However, once set then they read off fine, so it's not really important after I set them once...
  EEPROM.get(tempSpDayAddr, tempSpDay);
  if (tempSpDay == 0xFFFFFFFF) {
    tempSpDay = tempSpDayDefault;
  }
  EEPROM.get(tempSpNightAddr, tempSpNight);
  if (tempSpNight == 0xFFFFFFFF) {
    tempSpNight = tempSpNightDefault;
  }
  EEPROM.get(hourDayStartAddr, hourDayStart);
  if (hourDayStart == 0xFFFF) {
    hourDayStart = hourDayStartDefault;
  }
  EEPROM.get(hourDayEndAddr, hourDayEnd);
  if (hourDayEnd == 0xFFFF) {
    hourDayEnd = hourDayEndDefault;
  }

  Particle.variable("temp", temp);
  Particle.variable("humidity", humidity);
  Particle.variable("tempSp", tempSp);
  Particle.variable("tempSpDay", tempSpDay);
  Particle.variable("tempSpNight", tempSpNight);
  Particle.variable("hourDayStart", hourDayStart);
  Particle.variable("hourDayEnd", hourDayEnd);
  Particle.variable("lightStatus", lightStatus);
  Particle.variable("heatStatus", heatStatus);
  Particle.function("setTempSpDay", setTempSpDay);
  Particle.function("setTempSpNight", setTempSpNight);
  Particle.function("setHourDayStart", setHourDayStart);
  Particle.function("setHourDayEnd", setHourDayEnd);
  
  // Set heat and light off, since we don't know what their last latching position is
  turnLightOff();
  turnHeatOff();

  dataFetchedAt = Time.now();
  timeSyncedAt = Time.now();
}

void loop(void) {
  if (isDaytime()) {
    tempSp = tempSpDay;
    if (lightStatus == 0) {
      turnLightOn();
    }
  } else {
    tempSp = tempSpNight;
    if (lightStatus == 1) {
      turnLightOff();
    }
  }

  if (Time.now() > dataFetchedAt + dataFetchInterval) {
    temp = sht31.readTemperature()*9/5 + 32; // In Fahrenheit
    humidity = sht31.readHumidity();

    if (temp < lowDeadbandEdge() && heatStatus == 0) {
      turnHeatOn();
    } else if (temp > lowDeadbandEdge() && heatStatus == 1) {
      turnHeatOff();
    }

    dataFetchedAt = Time.now();
  }

  if (Time.now() > timeSyncedAt + timeSyncInterval) {
    Particle.syncTime();
    timeSyncedAt = Time.now();
  }
}


// PARTICLE FUNCTIONS

// Sets the tempSpDay variable
int setTempSpDay(String command) {
  double newSp = atof(command); // Converts a Str to a double
  if (newSp == 0.0) {
    return -1; // atof returns 0.0 if its not a valid conversion. We shouldn't ever want a temp of 0 anyway...
  } else {
    tempSpDay = newSp;
    EEPROM.put(tempSpDayAddr, tempSpDay);
    return 1;
  }
}

// Sets the tempSpNight variable
int setTempSpNight(String command) {
  double newSp = atof(command);
  if (newSp == 0.0) {
    return -1;
  } else {
    tempSpNight = newSp;
    EEPROM.put(tempSpNightAddr, tempSpNight);
    return 1;
  }
}

// Sets the hourDayStart variable
int setHourDayStart(String command) {
  int newHour = atoi(command); // Converts a Str to an int
  if (newHour < 0 || newHour > 23 || newHour > hourDayEnd) {
    return -1; // Don't allow settings that aren't 0-23 or are greater than hourDayEnd
  } else {
    hourDayStart = newHour;
    EEPROM.put(hourDayStartAddr, hourDayStart);
    return 1;
  }
}

// Sets the hourDayEnd variable
int setHourDayEnd(String command) {
  int newHour = atoi(command); // Converts a Str to an int
  if (newHour < 0 || newHour > 23 || newHour < hourDayStart) {
    return -1; // Don't allow settings that aren't 0-23 or are less than hourDayStart
  } else {
    hourDayEnd = newHour;
    EEPROM.put(hourDayEndAddr, hourDayEnd);
    return 1;
  }
}


// HELPER FUNCTIONS
bool isDaytime() {
  return hourDayStart <= Time.hour() && Time.hour() < hourDayEnd;
}

double lowDeadbandEdge() {
  return tempSp - tempDeadband;
}

double highDeadbandEdge() {
  return tempSp + tempDeadband;
}

void turnLightOn() {
  relaySet(lightOnPin);
  lightStatus = 1;
}

void turnLightOff() {
  relaySet(lightOffPin);
  lightStatus = 0;
}

void turnHeatOn() {
  relaySet(heatOnPin);
  heatStatus = 1;
}

void turnHeatOff() {
  relaySet(heatOffPin);
  heatStatus = 0;
}

void relaySet(uint16_t pin) {
  digitalWrite(pin, HIGH);
  delay(200); // in milliseconds. Apparently, only 10ms is required, but I don't like living on the edge.
  digitalWrite(pin, LOW);
}