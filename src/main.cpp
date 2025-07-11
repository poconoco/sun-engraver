#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <EEPROM.h>

#define DEBUG
//#define CALIBRATE_TOUCH

#include "Touch.h"
#include "LCD.h"
#include "XPT2046.h"
#include "IK2.h"
#include "SunBmp.h"

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#define MAX_FILES_COUNT 11

const int arm1Pin = 10;
const int arm2Pin = 11;

const int sdCsPin = 5;

const float arm1ZeroAngle = 95;
const float arm2ZeroAngle = 82+60;

const uint16_t fileRowHeight = 21; // Height of each file row in pixels
const uint16_t fileListX = 20; // X position of the file list
const uint16_t fileListY = 10; // Y position of the first file row


void testIK();
void getBmpFileList(String* destList, int& count, int maxCount);
void initializeDisplay();
void calibrateTouch();
void displayList(String* list, int count);
int getSelectedIndex(String* list, int count);

int selectFile(String* list, int count);

void setup() {

#ifdef DEBUG
  Serial.begin(9600);
#endif

//  testIK();

  String bmpFiles[MAX_FILES_COUNT];
  int bmpCount = 0;
  getBmpFileList(bmpFiles, bmpCount, MAX_FILES_COUNT);
  initializeDisplay();
  selectFile(bmpFiles, bmpCount);

  while (true) {
    delay(1000);
  }
}

int selectFile(String* list, int count) {
  while (true) {
    displayList(list, count);
    const int idx = getSelectedIndex(list, count);
    const String filename = list[idx];

    Tft.lcd_clear_screen(BLACK);

    File bmpFile = SD.open(filename);

    if (! bmpFile) {
        bmpFile.close();
        Tft.lcd_display_string(20, 50, (const uint8_t*)"Failed to open file!", FONT_1608, RED);
        delay(2000);
        continue;
    }

    if(! bmpReadHeader(bmpFile)) {
      bmpFile.close();
      Tft.lcd_display_string(20, 50, (const uint8_t*)"Bad header,", FONT_1608, RED);
      Tft.lcd_display_string(20, 70, (const uint8_t*)"bmp must be " STR(SUN_BMP_WIDTH) "x" STR(SUN_BMP_HEIGHT) ",", FONT_1608, RED);
      Tft.lcd_display_string(20, 90, (const uint8_t*)"not be compressed and be 24bit", FONT_1608, RED);
      delay(2000);
      continue;
    }

    displayPreview(bmpFile);
    bmpFile.close();

    return idx;
  }

  return -1; // Should never reach here
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
      if (filename.endsWith(".BMP")) {
        #ifdef DEBUG
          Serial.println(filename);
        #endif
        destList[count++] = filename;

        if (count >= maxCount) 
          break;
      }
    }
    entry.close();
  }
  root.close();
}

void initializeDisplay() {
  SPI.setDataMode(SPI_MODE3);
  SPI.setBitOrder(MSBFIRST);
  SPI.setClockDivider(SPI_CLOCK_DIV4);
  SPI.begin();

  Tft.lcd_init();
  Tft.setRotation(Rotation_270_D);
}

void displayList(String* list, int count) {
  Tft.lcd_clear_screen(BLACK);

  for (int i = 0; i < count; i++) {
      String filename = list[i];
      int x = fileListX;
      int y = fileListY + i * fileRowHeight;

      if (i % 2 == 0) {
          Tft.lcd_display_string(x, y, (const uint8_t *)filename.c_str(), FONT_1608, WHITE);
      } else {
          Tft.lcd_display_string(x, y, (const uint8_t *)filename.c_str(), FONT_1608, YELLOW);
      }
  }
}

void calibrateTouch() {
    Tp.tp_init();
    const int baseAddress = 0x00;
    int pointer = 0;

    #ifdef CALIBRATE_TOUCH
      #ifdef DEBUG
        Serial.println("Starting calibration...");
      #endif

      Tp.tp_adjust();
      Tp.tp_dialog();

      // Store calibration in the EEPROM
      tp_dev_t &touchParams = Tp.get_touch_params();
      EEPROM.put(baseAddress + pointer, touchParams.iXoff); pointer += sizeof(touchParams.iXoff);
      EEPROM.put(baseAddress + pointer, touchParams.iYoff); pointer += sizeof(touchParams.iYoff);
      EEPROM.put(baseAddress + pointer, touchParams.fXfac); pointer += sizeof(touchParams.fXfac);
      EEPROM.put(baseAddress + pointer, touchParams.fYfac); pointer += sizeof(touchParams.fYfac);
      #ifdef DEBUG
        Serial.println("Touch calibration saved to EEPROM: ");
        Serial.print("  - iXoff: "); Serial.println(touchParams.iXoff);
        Serial.print("  - iYoff: "); Serial.println(touchParams.iYoff);
        Serial.print("  - fXfac: "); Serial.println(touchParams.fXfac);
        Serial.print("  - fYfac: "); Serial.println(touchParams.fYfac);
        Serial.println();
      #endif
    #else

      tp_dev_t &touchParams = Tp.get_touch_params();

      EEPROM.get(baseAddress + pointer, touchParams.iXoff); pointer += sizeof(touchParams.iXoff);
      EEPROM.get(baseAddress + pointer, touchParams.iYoff); pointer += sizeof(touchParams.iYoff);
      EEPROM.get(baseAddress + pointer, touchParams.fXfac); pointer += sizeof(touchParams.fXfac);
      EEPROM.get(baseAddress + pointer, touchParams.fYfac); pointer += sizeof(touchParams.fYfac);

      #ifdef DEBUG
        Serial.println("Touch calibration loaded from EEPROM");
        Serial.print("  - iXoff: "); Serial.println(touchParams.iXoff);
        Serial.print("  - iYoff: "); Serial.println(touchParams.iYoff);
        Serial.print("  - fXfac: "); Serial.println(touchParams.fXfac);
        Serial.print("  - fYfac: "); Serial.println(touchParams.fYfac);
      #endif
    #endif
}

int getSelectedIndex(String* list, int count) {
  calibrateTouch();

  uint16_t x, y;
  while (true) {
    if (Tp.is_pressed(x, y)) {
      #ifdef DEBUG
        Serial.print("Touched at: ");
        Serial.print(x);
        Serial.print(", ");
        Serial.println(y);
      #endif

      // Check if the touch is within the bounds of the displayed list
      if (x >= fileListX && x <= Tft.LCD_WIDTH - 20 && y >= fileListY && y <= fileListY + count * fileRowHeight) {
        int index = (y - fileListY) / fileRowHeight;
        if (index >= 0 && index < count) {
          // Highlight the selected file
          Tft.lcd_display_string(fileListX, fileListY + index * fileRowHeight, (const uint8_t *)list[index].c_str(), FONT_1608, GREEN);
          delay(100);
          return index; // Return the selected index
        }
      }

      delay(50);
    }
  }

  return -1; // Should never get here
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
