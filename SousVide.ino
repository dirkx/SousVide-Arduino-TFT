/* Copyright 2011, 2021 Dirk-Willem van Gulik, alle rights reserved.
 *  
 *  Licensed under the Apache Software Licenses version 2.0 or newer.
 */
#include <Adafruit_GFX.h>    // Core graphics library

// Fancier Blue boards with thje M3 holes from Adafruit / kiwielectronics
// #include <Adafruit_TFTLCD.h>

// Red boards with "white price" sticker on the back from unknown
// provenance and odd white icons on the silkscreen.
//
#include "MCUFRIEND_kbv.h"
#include <TouchScreen.h>
#include <PID_v1.h>


// When using the BREAKOUT BOARD only, use these 8 data lines to the LCD:
// For the Arduino Uno, Duemilanove, Diecimila, etc.:
//   D0 connects to digital pin 8  (Notice these are
//   D1 connects to digital pin 9   NOT in order!)
//   D2 connects to digital pin 2
//   D3 connects to digital pin 3
//   D4 connects to digital pin 4
//   D5 connects to digital pin 5
//   D6 connects to digital pin 6
//   D7 connects to digital pin 7

// For the Arduino Mega, use digital pins 22 through 29
// (on the 2-row header at the end of the board).
//   D0 connects to digital pin 22
//   D1 connects to digital pin 23
//   D2 connects to digital pin 24
//   D3 connects to digital pin 25
//   D4 connects to digital pin 26
//   D5 connects to digital pin 27
//   D6 connects to digital pin 28
//   D7 connects to digital pin 29

// For the Arduino Due, use digital pins 33 through 40
// (on the 2-row header at the end of the board).
//   D0 connects to digital pin 33
//   D1 connects to digital pin 34
//   D2 connects to digital pin 35
//   D3 connects to digital pin 36
//   D4 connects to digital pin 37
//   D5 connects to digital pin 38
//   D6 connects to digital pin 39
//   D7 connects to digital pin 40


#define RELAY_1PERC_ON (100 /* milliseconds */)
#define RELAY 1 /* GPIO pin */
#define RELAY_ON HIGH

# // LCD
#define LCD_CS A3
#define LCD_CD A2 /* aka as RS */
#define LCD_WR A1
#define LCD_RD A0
#define LCD_RESET A4


// Sensor to use DS18B20 or MAX6675
//
// #define MAX6675
#define DS18B20
// #define MOCKSENSOR // will also mock the relay / not really switch it on.

#define LED (13)

unsigned long tempSampleTimeInMillies;

#ifdef DS18B20
// Temperatue sensor and relay
#include <OneWire.h>
#include <DallasTemperature.h>
#define ONE_WIRE_BUS 0
#define RESOLUTION 12

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

void setup_temp() {
  sensors.begin();
  DeviceAddress tempDeviceAddress;
  sensors.getAddress(tempDeviceAddress, 0);
  sensors.setResolution(tempDeviceAddress, RESOLUTION);

  sensors.setWaitForConversion(false);
  sensors.requestTemperatures(); // que one up.
  tempSampleTimeInMillies = 750 / (1 << (12 - RESOLUTION));
}

float get_temp() {
  float t = sensors.getTempCByIndex(0);
  sensors.requestTemperatures(); // que the next one up (to be feched in tempSampleTimeInMillies).
  return t;
}
#endif

#ifdef MAX6675
#include <Thermocouple.h>
#include <MAX6675_Thermocouple.h>
#include <AverageThermocouple.h>

#define SCK_PIN 3
#define CS_PIN 4
#define SO_PIN 5

#define READINGS_NUMBER 10
#define DELAY_TIME 10
Thermocouple* thermocouple = NULL;

void setup_temp() {
  Thermocouple* originThermocouple = new MAX6675_Thermocouple(SCK_PIN, CS_PIN, SO_PIN);
  thermocouple = new AverageThermocouple(
    originThermocouple,
    READINGS_NUMBER,
    DELAY_TIME
  );
  tempSampleTimeInMillies = DELAY_TIME * READINGS_NUMBER;
}

float get_temp() {
  return thermocouple->readCelsius();

}
#endif

#ifdef MOCKSENSOR
void setup_temp() {
  tempSampleTimeInMillies = 500;
}

float get_temp() {
  return 30.0 + 10 * sin(millis() / 5000.);
}
#endif

// Resistors touch screen.

#define YP A1  // must be an analog pin, use "An" notation!
#define XM A2  // must be an analog pin, use "An" notation!
#define YM 7   // can be a digital pin
#define XP 6   // can be a digital pin

#define PENRADIUS 3

#define TS_MINX 150
#define TS_MINY 120
#define TS_MAXX 920
#define TS_MAXY 940

// For better pressure precision, we need to know the resistance
// between X+ and X- Use any multimeter to read it
// For the one we're using, its 300 ohms across the X plate
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);


// Assign human-readable names to some common 16-bit color values:
#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

#define RGB(r, g, b) (((r&0xF8)<<8)|((g&0xFC)<<3)|(b>>3))

#define GREY      RGB(127, 127, 127)
#define DARKGREY  RGB(64, 64, 64)
#define TURQUOISE RGB(0, 128, 128)
#define PINK      RGB(255, 128, 192)
#define OLIVE     RGB(128, 128, 0)
#define PURPLE    RGB(128, 0, 128)
#define AZURE     RGB(0, 128, 255)
#define ORANGE    RGB(255,128,64)

// LCD orientation: always landscape, 1=USB upper left / 3=USB lower right
//
#define LCDROTATION 1

#define SET_SIZE  10
#define SET_X     SET_SIZE + 2 * SET_SIZE + 8
#define SET_Y     tft.height() / 2

#define ACT_SIZE    SET_SIZE
#define ACT_X       SET_X
#define ACT_Y       30

float   setTemp = 37.00;

// Adafruit_TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET);
MCUFRIEND_kbv tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET);

int GRAPH_X, GRAPH_W, GRAPH_Y, GRAPH_H, PWR_X, PWR_Y, PWR_H, PWR_W, DX;

const float MIN_T = 10.;
const float MAX_T = 50.;

const int N = 128;

float   temp[N], setTempHistory[N];

// double kp=2,ki=0.5,kd=2;

// 5 minuten 15 -> 37graden; overshoot to 60; 4 min.
// Dan 8-10 minuten flat. Daling 0.2/10 seconden. Bij 50C 0.2/15 seconden.
//
// 0 and 100% change rates.
//
#define DOWNRATE (0.2/15)    // Daalt ron de 0.1-0.2 per 10-15 seconden.
#define UPRATE ((60-16)/600) // van 15->16 graden in ongeveer 10 minuten.

// double consKp = 10, consKi = 0.05, consKd = 0;
double consKp = 5, consKi = 0.01, consKd = 3.0;
double Setpoint, Input, Output;

PID myPID(&Input, &Output, &Setpoint, consKp, consKi, consKd, DIRECT);

class Log : public Print {
  public:
    Log() {
      Serial.begin(115200);
    }
    virtual size_t write(uint8_t c);
  private:
};

size_t Log::write(uint8_t c) {
  return Serial.write(c);
}

Log Log;


void setup(void) {
  Log.println("\n\nStarted " __FILE__ "\n" __DATE__ " " __TIME__);
  tft.reset();

  uint16_t identifier = tft.readID();
  if (!identifier)
    Log.println(F("No LCD driver found"));
  else {
    Log.print(F("LCD driver chip: 0x"));
    Log.println(identifier, HEX);
  }

  tft.begin(identifier);
  tft.setRotation(LCDROTATION);

  // We cannot '#define' or const-optimize these; as the tft size
  // and orientation is only known post setup.
  //
  GRAPH_X = 120;
  GRAPH_W = (tft.width() - GRAPH_X - 16);

  GRAPH_Y = 30;
  GRAPH_H  = (tft.height() - GRAPH_Y - 32);

  PWR_X = SET_SIZE;
  PWR_W = (60);
  PWR_H = (6);
  PWR_Y = (tft.height() - 12 - PWR_H);

  DX = GRAPH_W / N + 1;

  tft.fillScreen(BLACK);
  tft.drawRect(1, 1, tft.width() - 2, tft.height() - 2, WHITE);


  tft.fillTriangle(SET_X - 2 * SET_SIZE, SET_Y + 2 * SET_SIZE,
                   SET_X + 2 * SET_SIZE, SET_Y + 2 * SET_SIZE,
                   SET_X, SET_Y + 4.2 * SET_SIZE,
                   WHITE);
  tft.fillTriangle(SET_X - 2 * SET_SIZE, SET_Y - 2 * SET_SIZE,
                   SET_X + 2 * SET_SIZE, SET_Y - 2 * SET_SIZE,
                   SET_X, SET_Y - 4.2 * SET_SIZE,
                   WHITE);
  updateSetTemp(setTemp);
  updateTemp(13.2);


  tft.drawLine(GRAPH_X - 2, GRAPH_Y - 4,
               GRAPH_X - 2, GRAPH_Y + GRAPH_H + 8,
               WHITE);
  tft.drawLine(GRAPH_X - 4,             GRAPH_Y + GRAPH_H  + 2,
               GRAPH_X + GRAPH_W + 4, + GRAPH_Y + GRAPH_H  + 2,
               WHITE);

  char buff[10];

  tft.setTextColor(WHITE, BLACK);
  tft.setTextSize(1);

  tft.setCursor(GRAPH_X - 30, GRAPH_Y - 4 + GRAPH_H);
  snprintf(buff, sizeof(buff), "%02d.%01d", (int)MIN_T, ((int)(10 * MIN_T)) % 10);
  tft.print(buff);

  tft.setCursor(GRAPH_X - 30, GRAPH_Y - 4);
  snprintf(buff, sizeof(buff), "%02d.%01d", (int)MAX_T, ((int)(10 * MAX_T)) % 10);
  tft.print(buff);

  tft.drawRect(PWR_X - 1, PWR_Y - 1,
               PWR_W + 2, PWR_H + 2,
               WHITE);

  myPID.SetMode(AUTOMATIC);
  myPID.SetOutputLimits(0, 100);

  setup_temp();

  pinMode(RELAY, OUTPUT);
  digitalWrite(RELAY, !RELAY_ON);

  pinMode(LED, OUTPUT);
}

void updateSetTemp(float f) {
  char buff[10];

  tft.setCursor(SET_X - 9 * SET_SIZE / 4 , SET_Y - SET_SIZE / 2 - 2);
  tft.setTextColor(WHITE, BLACK);
  tft.setTextSize(SET_SIZE / 4);
  snprintf(buff, sizeof(buff), "%02d.%01d", (int)f, ((int)(10 * fabs(f))) % 10);
  tft.print(buff);
}

void updateTemp(float f) {
  char buff[10];

  tft.setCursor(ACT_X - 9 * ACT_SIZE / 4 , ACT_Y - ACT_SIZE / 2 - 2);
  tft.setTextColor(WHITE, BLACK);
  tft.setTextSize(ACT_SIZE / 4);
  snprintf(buff, sizeof(buff), "%02d.%01dC", (int)f, ((int)(10 * fabs(f))) % 10);
  tft.print(buff);
}

#define MINPRESSURE 10
#define MAXPRESSURE 1000


void loop()
{
  digitalWrite(LED, HIGH);
  TSPoint q = ts.getPoint();
  TSPoint p = q;
  digitalWrite(LED, LOW);

  // if sharing pins, you'll need to fix the directions of the touchscreen pins
  //pinMode(XP, OUTPUT);
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);
  //pinMode(YM, OUTPUT);

  static unsigned long pressStarted = 0;
  unsigned long now = millis();

  unsigned long thisPress = 0;
  if (p.z > MINPRESSURE && p.z < MAXPRESSURE) {
    p.x = map(q.x, TS_MINX, TS_MAXX, 0, tft.width());
    p.y = map(q.y, TS_MINY, TS_MAXY, tft.height(), 0);

    if ((p.x > SET_X -  3 * SET_SIZE) &&  (p.x < SET_X +  3 * SET_SIZE)) {
      if ((p.y < SET_Y -  3 * SET_SIZE) && (p.y > SET_Y -  7 * SET_SIZE)) {

        if (now - pressStarted > 300)
          setTemp -= 0.1;

        if (pressStarted && now - pressStarted > 3000)
          setTemp -= 1;

        thisPress = now;
      } else if ((p.y > SET_Y +  3 * SET_SIZE) && (p.y < SET_Y +  7 * SET_SIZE)) {
        if (now - pressStarted > 300)
          setTemp += 0.1;
        if (pressStarted && now - pressStarted > 3000)
          setTemp += 1;

        thisPress = now;
      }
    }

    if (pressStarted == 0)
      pressStarted = thisPress;

    if (thisPress == 0) {
      pressStarted = 0;
    } else {
      if (setTemp > MAX_T)
        setTemp = MAX_T;
      if (setTemp < MIN_T)
        setTemp = MIN_T;
      updateSetTemp(setTemp);
    }


#if 0
    if ((thisPress == 0) && ((p.y - PENRADIUS) > 0) && ((p.y + PENRADIUS) < tft.height())) {
      tft.fillCircle(p.x, p.y, PENRADIUS, RED);
    }
#endif
  }

  static unsigned long lastUpdate = 0;
  if (now - lastUpdate > tempSampleTimeInMillies) {
    float t = get_temp();
    lastUpdate = millis();

    temp[N - 1] = t;

    for (int i = 0; i < N - 1; i++) {
      temp[i] = temp[i + 1];
      setTempHistory[i] = setTempHistory[i + 1];
    }

    temp[N - 1] =  temp[N - 2];

    setTempHistory[N - 1] = setTemp;
    updateTemp( temp[N - 1]);

    for (int i = 0; i < N; i++) {
      float t = temp[i];

      if (t < MIN_T) t = MIN_T;
      if (t > MAX_T) t = MAX_T;

      t = (t - MIN_T) / (MAX_T - MIN_T);

      int x = GRAPH_X + GRAPH_W * i / N;
      int y = GRAPH_Y + GRAPH_H * (1. - t);

      t = (setTempHistory[i] - MIN_T) / (MAX_T - MIN_T);
      int sety = GRAPH_Y + GRAPH_H * (1. - t);

      tft.drawFastVLine(x, GRAPH_Y , GRAPH_H + 1, BLACK);
      tft.drawFastVLine(x + 1, GRAPH_Y , GRAPH_H + 1, BLACK);

      if (setTempHistory[i])
        tft.fillRect(x - 2, sety, 4, 2, YELLOW);
      if (temp[i])
        tft.fillRect(x - 2, y, 4, 2, GREEN);
    }
  }


  static unsigned long lastPid = 0;
  if (now - lastPid > 500) {
    Input = temp[N - 1];
    Setpoint = setTemp;
    myPID.Compute();


    tft.fillRect(PWR_X, PWR_Y,
                 PWR_W, PWR_H,
                 BLACK);

    tft.fillRect(PWR_X, PWR_Y,
                 PWR_W * Output / 100. , PWR_H,
                 GREEN);

    tft.fillCircle(PWR_X + PWR_W + 8, PWR_Y - 1 + PWR_H / 2, 3, digitalRead(RELAY) ? MAGENTA : BLACK);
    tft.drawCircle(PWR_X + PWR_W + 8, PWR_Y - 1 + PWR_H / 2, 5, WHITE);

    tft.setCursor(PWR_X + PWR_W / 2 - 25, PWR_Y - 26);
    tft.setTextColor(WHITE, BLACK);
    tft.setTextSize(2);
    char buff[10];

    snprintf(buff, sizeof(buff), "%3d%%  ", (int)Output);
    tft.print(buff);

    lastPid = now;
  }

#if 0
  Log.print("V ");
  Log.print(((now / 500) % 100) / 5);
  Log.print(" -- ");
  Log.println(Output / 5);
#endif

  // If RELAY_1PERC_ON == 500 milli seconds then: switch at least 0.5
  // second; for up to 0.5 x 100 = 50 seconds cycles. Linear (so 10%
  // is 5 second on; 45 seconds off).
  //
  static bool lastState = false;
  if (((int)(now / RELAY_1PERC_ON) % (100)) < (Output)) {
    if (!lastState)
      Log.println("On");
    lastState = true;
#ifndef MOCKSENSOR
    digitalWrite(RELAY, RELAY_ON);
#endif

  } else {
    if (lastState)
      Log.println("off");
    lastState = false;
#ifndef MOCKSENSOR
    digitalWrite(RELAY, !RELAY_ON);
#endif
  }
}
