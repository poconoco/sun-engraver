#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <EEPROM.h>

//#define DEBUG
//#define CALIBRATE_TOUCH

#include "Touch.h"
#include "LCD.h"
#include "XPT2046.h"
#include "IK2.h"
#include "SunBmp.h"
#include "Button.h"

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#define MAX_FILES_COUNT 11

const int arm1Pin = 6;
const int arm2Pin = 8;

const int sdCsPin = 5;

const float arm1ZeroAngle = 95;
const float arm2ZeroAngle = 82+60;

const uint16_t fileRowHeight = 21; // Height of each file row in pixels
const uint16_t fileListX = 20; // X position of the file list
const uint16_t fileListY = 10; // Y position of the first file row

// Should be square (PX_MAX - PX_MIN == PY_MAX - PY_MIN)
const float lensXmin = 50;
const float lensXmax = 110;
const float lensYmin = -20;
const float lensYmax = 40;

void getBmpFileList(String*, int&, int);
void initializeDisplay();
void calibrateTouch();
void displayList(String*, int);
int getSelectedIndex(String*, int);
int selectFile(String*, int);
bool confirmSelection(String *, int);
int selectSpeed();
bool prepFocusLens();
bool focusLens(IK2DOF&);
bool doPrint(IK2DOF&, int);

void setup() {

#ifdef DEBUG
  Serial.begin(9600);
#endif

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

  String bmpFiles[MAX_FILES_COUNT];
  int bmpCount = 0;
  getBmpFileList(bmpFiles, bmpCount, MAX_FILES_COUNT);
  initializeDisplay();

  int selectedIndex = -1;
  int selectedSpeed = -1;  // From 1 to 10
  while (true) {
    selectedIndex = selectFile(bmpFiles, bmpCount);
    while (confirmSelection(bmpFiles, selectedIndex)) {
      while (true) {
        selectedSpeed = selectSpeed();
        if (selectedSpeed > 0) {
          while (prepFocusLens()) {
            while (focusLens(ik2dof)) {
              if (!doPrint(ik2dof, selectedSpeed))
                break;
            }
          }
        } else {
          break;
        }
      }
    }
  }
}

int selectFile(String* list, int count) {
  while (true) {
    displayList(list, count);
    const int idx = getSelectedIndex(list, count);
    return idx;
  }

  return -1; // Should never reach here
}

bool confirmSelection(String *list, int idx) {

  const String filename = list[idx];

  Tft.lcd_clear_screen(BLACK);

  File bmpFile = SD.open(filename);

  if (! bmpFile) {
      bmpFile.close();
      Tft.lcd_display_string(20, 50, (const uint8_t*)"Failed to open file!", FONT_1608, RED);
      delay(2000);
      return false;
  }

  if(! bmpReadHeader(bmpFile)) {
    bmpFile.close();
    Tft.lcd_display_string(20, 50, (const uint8_t*)"Bad header,", FONT_1608, RED);
    Tft.lcd_display_string(20, 70, (const uint8_t*)"bmp must be " STR(SUN_BMP_WIDTH) "x" STR(SUN_BMP_HEIGHT) ",", FONT_1608, RED);
    Tft.lcd_display_string(20, 90, (const uint8_t*)"not be compressed and be 24bit", FONT_1608, RED);
    delay(2000);
    return false;
  }

  displayPreview(bmpFile);
  bmpFile.close();

  Button confirmButton(Tft.LCD_WIDTH - 100, Tft.LCD_HEIGHT - 35, 80, 30, "OK >");
  Button backButton(20, Tft.LCD_HEIGHT - 35, 80, 30, "< Back");

  while (true) {
    if (confirmButton.isClicked()) {
      return true;
    }

    if (backButton.isClicked()) {
      return false;
    }
  }
}

int selectSpeed() {
  Tft.lcd_clear_screen(BLACK);
  Tft.lcd_display_string(100, 50, (const uint8_t*)"Set speed 1..10:", FONT_1608, WHITE);

  Button confirmButton(Tft.LCD_WIDTH - 100, Tft.LCD_HEIGHT - 35, 80, 30, "OK >");
  Button backButton(20, Tft.LCD_HEIGHT - 35, 80, 30, "< Back");

  const int speedX = Tft.LCD_WIDTH / 2 - 30 / 2;
  const int speedY = Tft.LCD_HEIGHT / 2 - 20;

  Button dec(speedX - 50, speedY, 30, 30, "-");
  Button inc(speedX + 50, speedY, 30, 30, "+");
  Tft.lcd_draw_rect(speedX, speedY, 30, 30, WHITE);
  Tft.lcd_fill_rect(speedX+1, speedY+1, 30-2, 30-2, GRAY);

  int speed = 5;
  bool redraw = true;
  while (true) {

    if (redraw) {
      Tft.lcd_fill_rect(speedX + 7, speedY + 7, 16, 16, GRAY);
      Tft.lcd_display_string(speedX + (speed < 10 ? 11 : 7), speedY + 7, (const uint8_t*)String(speed).c_str(), FONT_1608, WHITE);
      redraw = false;
    }

    if (dec.isClicked()) {
      if (speed > 1) {
        speed--;
        redraw = true;
      }
    }

    if (inc.isClicked()) {
      if (speed < 10) {
        speed++;
        redraw = true;
      }
    }

    if (confirmButton.isClicked()) {
      return speed;
    }

    if (backButton.isClicked()) {
      return -1;
    }
  }
}

bool prepFocusLens() {
  Tft.lcd_clear_screen(BLACK);
  Tft.lcd_display_string(100, 50, (const uint8_t*)"Cover lens with", FONT_1608, WHITE);
  Tft.lcd_display_string(100, 70, (const uint8_t*)"focusing cap", FONT_1608, WHITE);

  Button confirmButton(Tft.LCD_WIDTH - 100, Tft.LCD_HEIGHT - 35, 80, 30, "OK >");
  Button backButton(20, Tft.LCD_HEIGHT - 35, 80, 30, "< Back");

  while (true) {
    if (confirmButton.isClicked()) {
      return true;
    }

    if (backButton.isClicked()) {
      return false;
    }
  }
}

bool focusLens(IK2DOF& ik2dof) {
  Tft.lcd_clear_screen(BLACK);
  Tft.lcd_display_string(100, 50, (const uint8_t*)"Focus lens...", FONT_1608, WHITE);

  Button confirmButton(Tft.LCD_WIDTH - 100, Tft.LCD_HEIGHT - 35, 80, 30, "OK >");
  Button backButton(20, Tft.LCD_HEIGHT - 35, 80, 30, "< Back");

  while (true) {
    const float maxDelta = lensXmax - lensXmin;
    // Move along four sides of square
    for (int direction = 0; direction < 4; direction++) {
      for (float i = 0; i < maxDelta; i++) {
        float x;
        float y;
        switch (direction) {
          case 0:
            x = lensXmin + i;
            y = lensYmin;
            break;
          case 1:
            x = lensXmax;
            y = lensYmin + i;
            break;
          case 2:
            x = lensXmax - i;
            y = lensYmax;
            break;
          case 3:
            x = lensXmin;
            y = lensYmax - i;
            break;
        }

        ik2dof.write(x, y);
        if (confirmButton.isClicked()) {
          return true;
        }

        if (backButton.isClicked()) {
          return false;
        }
        delay(10);
      }
    }
  }
}

bool doPrint(IK2DOF& ik2dof, int speed) {
  Tft.lcd_clear_screen(BLACK);
  Tft.lcd_display_string(100, 50, (const uint8_t*)"Printing...", FONT_1608, WHITE);

  Button confirmButton(Tft.LCD_WIDTH - 100, Tft.LCD_HEIGHT - 35, 80, 30, "OK >");
  Button backButton(20, Tft.LCD_HEIGHT - 35, 80, 30, "< Back");

  while (true) {
    if (confirmButton.isClicked()) {
      return true;
    }

    if (backButton.isClicked()) {
      return false;
    }
  }  
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
