#include <Arduino.h>
#include <SPI.h>
#include <LCD.h>
#include <SD.h>

#define DEBUG

#include "XPT2046.h"
#include "IK2.h"

#define MAX_FILES_COUNT 11

const int arm1Pin = 10;
const int arm2Pin = 11;

const int sdCsPin = 5;


const float arm1ZeroAngle = 95;
const float arm2ZeroAngle = 82+60;

void testIK();
void getBmpFileList(String* destList, int& count, int maxCount);
void displayList(String* list, int count);

void setup() {

#ifdef DEBUG
  Serial.begin(9600);
#endif

//  testIK();

  String bmpFiles[MAX_FILES_COUNT];
  int bmpCount = 0;
  getBmpFileList(bmpFiles, bmpCount, MAX_FILES_COUNT);
  displayList(bmpFiles, bmpCount);

  while (true) {}
}

void getBmpFileList(String* destList, int& count, int maxCount) {
  count = 0;

  __XPT2046_CS_DISABLE();

  pinMode(sdCsPin, OUTPUT);
  digitalWrite(sdCsPin, HIGH);
  Sd2Card card;
  card.init(SPI_FULL_SPEED, sdCsPin); 
  if(!SD.begin(sdCsPin))  { 
    #ifdef DEBUG
      Serial.println("SD init failed!");
    #endif
    return;
  }

  #ifdef DEBUG
    Serial.println("SD card initialized. Searching for .BMP files...");
  #endif
  File root = SD.open("/");
  if (!root) {
    #ifdef DEBUG
      Serial.println("Failed to open root directory.");
    #endif
    return;
  }

  while (true) {
    File entry = root.openNextFile();
    if (!entry) break;

    if (!entry.isDirectory()) {
      String filename = entry.name();
      filename.toUpperCase(); // Ensure case-insensitive matching
      if (filename.endsWith(".BMP")) {
        #ifdef DEBUG
          Serial.println(filename);
        #endif
        destList[count++] = filename;
        if (count >= maxCount) break;
      }
    }
    entry.close();
  }
  root.close();
}

void displayList(String* list, int count) {
    SPI.setDataMode(SPI_MODE3);
    SPI.setBitOrder(MSBFIRST);
    SPI.setClockDivider(SPI_CLOCK_DIV4);
    SPI.begin();

    Tft.lcd_init();
    Tft.setRotation(Rotation_270_D);
    Tft.lcd_clear_screen(BLACK);

    for (int i = 0; i < count; i++) {
        String filename = list[i];
        int x = 20;
        int y = 10 + i * 21;

        if (i % 2 == 0) {
            Tft.lcd_display_string(x, y, (const uint8_t *)filename.c_str(), FONT_1608, WHITE);
        } else {
            Tft.lcd_display_string(x, y, (const uint8_t *)filename.c_str(), FONT_1608, YELLOW);
        }
    }
  }

void testIK() {

  Servo servoArm1;
  Servo servoArm2;

  servoArm1.attach(arm1Pin, 550, 2500);
  servoArm2.attach(arm2Pin, 550, 2450);

  IK2DOF ik2dof(
      70,  // arm1 length
      70,  // arm2 length
      arm1ZeroAngle,
      arm2ZeroAngle,
      false,  // arm1 not inverted
      false,  // arm2 not inverted
      servoArm1,
      servoArm2
  );

  //  servoArm1.write(arm1ZeroAngle);
//  servoArm2.write(arm2ZeroAngle);

//  ik2dof.write(40, 60);
//  ik2dof.write(75, 60);


  float angle = 0;
  float cx = 70;
  float cy = 70;
  float r = 40;

  while (true) {

    float x = cx + r * cos(radians(angle));
    float y = cy + r * sin(radians(angle));

    ik2dof.write(x, y);

    delay(10);

    angle += 1;
    if (angle > 360) {
      angle = 0;
    }
  }
}
