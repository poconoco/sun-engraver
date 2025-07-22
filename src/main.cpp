#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <EEPROM.h>

// Uncommenting DEBUG requiers disabling manually some features, otehrwise 
// you'll get memory overflow
//#define DEBUG

//#define CALIBRATE_TOUCH
//#define CALIBRATE_SERVO_ANGLES
//#define SKIP_BURNING
 
#include "Touch.h"
#include "LCD.h"
#include "XPT2046.h"
#include "IK2.h"
#include "SunBmp.h"
#include "Button.h"
#include "FloatServo.h"
#include "BitSet.h"
#include "config.h"

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

void getBmpFileList(String*, int&);
void initializeDisplay();
void calibrateTouch();
void displayList(String*, int);
int getSelectedIndex(String*, int);
int selectFile(String*, int);
bool confirmSelection(String *, int);
bool selectSpeed(int8_t &resultSpeed);
bool selectSunDirection(int &resultDirection, int8_t &resultMonth);
bool prepFocusLens();
bool focusLens(IK2DOF&);
bool doBurn(IK2DOF&, int, float, int8_t month, String);
void drawQuadPoint(uint16_t, uint16_t, uint16_t);
void drawArrow(uint16_t cx, uint16_t cy, uint16_t length, float angleDeg, uint16_t color);
void offsetXY(float& x, float& y, float angleDeg, float distance);
float getSunSpeed(int8_t month);
void offsetBySunMovement(float &x, float &y, float sunDirection, float sunSpeed, float timeDeltaSec);
void draw_middle_label(const char *label, int y, uint16_t color);

void setup() {

  #ifdef DEBUG
    Serial.begin(9600);
  #endif

  FloatServo servoArm1(ARM1_PIN, 310, 2550);
  FloatServo servoArm2(ARM2_PIN, 560, 2650);

  #if defined(CALIBRATE_SERVO_ANGLES)
    servoArm1.attach();
    servoArm2.attach();

    servoArm1.writeFloat(ARM1_STRAIGHT_BRACKET_ANGLE);
    servoArm2.writeFloat(ARM2_STRAIGHT_BRACKET_ANGLE - ARM2_BRACKET_TO_LENS_ANGLE);

    while (true)
      delay(1000);
  #else

    IK2DOF ik2dof(
        70,  // arm1 length
        70,  // arm2 length
        ARM1_STRAIGHT_BRACKET_ANGLE,
        ARM2_STRAIGHT_BRACKET_ANGLE - ARM2_BRACKET_TO_LENS_ANGLE,
        true,  // arm1 inverted
        true,  // arm2 inverted
        servoArm1,
        servoArm2
    );

    String bmpFiles[MAX_FILES_COUNT];
    int bmpCount = 0;
    getBmpFileList(bmpFiles, bmpCount);
    initializeDisplay();

    int selectedIndex = -1;
    int8_t selectedSpeed = -1;  // From 1 to 10
    int selectedSunDirection = -1;
    int8_t selectedMonth = -1;
    bool done = false;
    while (true) {
      selectedIndex = selectFile(bmpFiles, bmpCount);
      while (confirmSelection(bmpFiles, selectedIndex)) {
        while (selectSpeed(selectedSpeed)) {
          while (selectSunDirection(selectedSunDirection, selectedMonth)) {
            while (prepFocusLens()) {
              while (focusLens(ik2dof)) {
                // Need to define this out otherwise out of flash storage when calibration is included
                #ifndef CALIBRATE_TOUCH
                  if (doBurn(ik2dof, 
                             selectedSpeed, 
                             selectedSunDirection, 
                             selectedMonth, 
                             bmpFiles[selectedIndex]))
                    done = true;
                #endif
                break;
              }

              if (done)
                break;
            }
          }

          if (done)
            break;
        }

        if (done)
          break;
      }
    }

  #endif
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
      draw_middle_label("Failed to open file!", 50, RED);
      delay(2000);
      return false;
  }

  SunBmp sunBmp(bmpFile);

  if(! sunBmp.bmpReadHeader()) {
    bmpFile.close();
    draw_middle_label("Bad header,", 50, RED);
    draw_middle_label("bmp must be " STR(SUN_BMP_WIDTH) "x" STR(SUN_BMP_HEIGHT) ",", 70, RED);
    draw_middle_label("not be compressed and be 24bit", 90, RED);
    delay(2000);
    return false;
  }

  sunBmp.displayPreview();
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

bool selectSpeed(int8_t &resultSpeed) {
  Tft.lcd_clear_screen(BLACK);
  draw_middle_label("Set speed 1..10:", 50, WHITE);

  Button confirmButton(Tft.LCD_WIDTH - 100, Tft.LCD_HEIGHT - 35, 80, 30, "OK >");
  Button backButton(20, Tft.LCD_HEIGHT - 35, 80, 30, "< Back");

  const int speedX = Tft.LCD_WIDTH / 2 - 30 / 2;
  const int speedY = Tft.LCD_HEIGHT / 2 - 20;

  Button speedMinus(speedX - 50, speedY, 30, 30, "-");
  Button speedPlus(speedX + 50, speedY, 30, 30, "+");
  Tft.lcd_draw_rect(speedX, speedY, 30, 30, WHITE);
  Tft.lcd_fill_rect(speedX+1, speedY+1, 30-1, 30-1, GREEN);

  int speed;
  EEPROM.get(SPEED_EEPROM_ADDR, speed);
  if (speed < 0 || speed > 10)
    speed = 5;
  bool redraw = true;
  while (true) {

    if (redraw) {
      Tft.lcd_fill_rect(speedX + 7, speedY + 7, 16, 16, GREEN);
      Tft.lcd_display_string(speedX + (speed < 10 ? 11 : 7), speedY + 7, (const uint8_t*)String(speed).c_str(), FONT_1608, BLACK);
      redraw = false;
    }

    if (speedMinus.isClicked()) {
      if (speed > 1) {
        speed--;
        redraw = true;
      }
    }

    if (speedPlus.isClicked()) {
      if (speed < 10) {
        speed++;
        redraw = true;
      }
    }

    if (confirmButton.isClicked()) {
      EEPROM.put(SPEED_EEPROM_ADDR, speed);
      resultSpeed = speed;
      return true;
    }

    if (backButton.isClicked()) {
      return false;
    }
  }
}

bool selectSunDirection(int &resultDirection, int8_t &resultMonth) {
  Tft.lcd_clear_screen(BLACK);
  draw_middle_label("Set sun movement direction:", 20, WHITE);

  Button confirmButton(Tft.LCD_WIDTH - 100, Tft.LCD_HEIGHT - 35, 80, 30, "OK >");
  Button backButton(20, Tft.LCD_HEIGHT - 35, 80, 30, "< Back");

  const int cx = Tft.LCD_WIDTH / 2;
  const int cy = Tft.LCD_HEIGHT / 2;

  const int dirCtlOffset = -40;
  const int monCtlOffset = 50;

  Button dirMinus(cx + 80 - 15, cy - 15 + dirCtlOffset, 30, 30, ">");
  Button dirPlus(cx - 80 - 15, cy - 15 + dirCtlOffset, 30, 30, "<");

  draw_middle_label("Month:", cy - 35 + monCtlOffset, WHITE);

  Button monMinus(cx - 80 - 15, cy - 15 + monCtlOffset, 30, 30, "-");
  Button monPlus(cx + 80 - 15, cy - 15 + monCtlOffset, 30, 30, "+");

  Tft.lcd_draw_rect(cx - 15, cy - 15 + monCtlOffset, 30, 30, WHITE);
  Tft.lcd_fill_rect(cx - 15 + 1, cy - 15 + monCtlOffset + 1, 30-1, 30-1, GREEN);

  int direction;
  EEPROM.get(SUN_DIR_EEPROM_ADDR, direction);
  if (direction < 0 || direction >= 360)
    direction = 90;

  int8_t month;
  EEPROM.get(MONTH_EEPROM_ADDR, month);
  if (month < 1 || month > 12)
    month = 7;

  int prevDirection = direction;
  bool redrawDir = true;
  bool redrawMon = true;
  while (true) {

    if (redrawDir) {
      drawArrow(cx, cy - dirCtlOffset, 60, prevDirection, BLACK);
      drawArrow(cx, cy - dirCtlOffset, 60, direction, YELLOW);

      prevDirection = direction;
      redrawDir = false;
    }

    if (redrawMon) {
      Tft.lcd_fill_rect(cx - 8, cy - 8 + monCtlOffset, 16, 16, GREEN);
      Tft.lcd_display_string(cx - 8 + ((month < 10) ? 4 : 0), cy - 8 + monCtlOffset, (const uint8_t*)String(month).c_str(), FONT_1608, BLACK);

      redrawMon = false;
    }

    if (dirMinus.isClicked()) {
      direction -= 15;
      redrawDir = true;
      if (direction < 0)
        direction += 360;
    }

    if (dirPlus.isClicked()) {
      direction += 15;
      redrawDir = true;
      if (direction >= 360)
        direction -= 360;
    }

    if (monMinus.isClicked()) {
      month--;
      redrawMon = true;
      if (month < 1)
        month += 12;
    }

    if (monPlus.isClicked()) {
      month++;
      redrawMon = true;
      if (month > 12)
        month -= 12;
    }

    if (confirmButton.isClicked()) {
      EEPROM.put(SUN_DIR_EEPROM_ADDR, direction);
      EEPROM.put(MONTH_EEPROM_ADDR, month);
      resultDirection = direction;
      resultMonth = month;
      return true;
    }

    if (backButton.isClicked()) {
      return false;
    }
  }
}

bool prepFocusLens() {
  Tft.lcd_clear_screen(BLACK);
  draw_middle_label("Prepare to focus", 50, WHITE);

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
  draw_middle_label("Focus", 50, WHITE);

  Button pauseButton(Tft.LCD_WIDTH - 100, Tft.LCD_HEIGHT - 75, 80, 30, "Pause");
  Button confirmButton(Tft.LCD_WIDTH - 100, Tft.LCD_HEIGHT - 35, 80, 30, "BURN >");
  Button backButton(20, Tft.LCD_HEIGHT - 35, 80, 30, "< Back");

  ik2dof.write(LENS_X_MIN, LENS_Y_MIN);

  bool result;
  bool pause = false;

  auto processButtons = [&pauseButton, &confirmButton, &backButton, &ik2dof, &result, &pause]() {
      if (confirmButton.isClicked()) {
        result = true;
        return true;
      }

      if (backButton.isClicked()) {
        ik2dof.detach();
        result = false;
        return true;
      }

      if (pauseButton.isClicked()) {
        pause = !pause;
      }

      return false;
  };

  while (true) {
    const float maxDelta = LENS_X_MAX - LENS_X_MIN;
    // Move along four sides of square
    for (int direction = 0; direction < 4; direction++) {

      // A timeout at the corner to allow for focusing at the corners easier
      // Have to process buttons inside it as well, to not degrade responsiveness
      for (uint8_t i = 0; i < 100; i++) {
        if (processButtons())
          return result;

        delay(10);
      }

      delay(1000);

      for (float i = 0; i < maxDelta; i++) {
        if (pause) {
          i--;
        } else {
          float x;
          float y;
          switch (direction) {
            case 0:
              x = LENS_X_MIN + i;
              y = LENS_Y_MIN;
              break;
            case 1:
              x = LENS_X_MAX;
              y = LENS_Y_MIN + i;
              break;
            case 2:
              x = LENS_X_MAX - i;
              y = LENS_Y_MAX;
              break;
            case 3:
              x = LENS_X_MIN;
              y = LENS_Y_MAX - i;
              break;
          }

          ik2dof.write(x, y);          
        }

        if (processButtons())
          return result;

        delay(10);
      }
    }
  }
}

bool doBurn(IK2DOF& ik2dof, int speedFactor, float sunDirection, int8_t month, String filename) {
  const uint16_t statusX = 120;
  const uint16_t statusY = Tft.LCD_HEIGHT - 27;
  Tft.lcd_clear_screen(BLACK);
  Tft.lcd_display_string(statusX, statusY, (const uint8_t*)"Burning...", FONT_1608, WHITE);

  Button backButton(20, Tft.LCD_HEIGHT - 35, 80, 30, "< Back");

  File bmpFile = SD.open(filename);

  if (! bmpFile) {
      bmpFile.close();
      Tft.lcd_display_string(20, 50, (const uint8_t*)"Failed to open file!", FONT_1608, RED);
      delay(2000);
      return false;
  }

  SunBmp sunBmp(bmpFile);

  if(! sunBmp.bmpReadHeader()) {
    bmpFile.close();
    Tft.lcd_display_string(20, 50, (const uint8_t*)"Bad header", FONT_1608, RED);
    delay(2000);
    return false;
  }

  #ifdef SKIP_BURNING
    ik2dof.write(LENS_X_MIN, LENS_Y_MIN);
    bool done = true;
  #else
    bool lastBurn = false;
    const float sunSpeed = getSunSpeed(month);
    const unsigned long start = millis();

    auto burnPixelFunc = [
      &ik2dof,
      &lastBurn, 
      &backButton,
      sunSpeed,
      speedFactor,
      sunDirection,
      start
    ] (
      int imageX, 
      int imageY, 
      bool burn,
      BitSet<IMAGE_WIDTH> prevLine,
      bool arm
    ){
      // Calculate lens position
      float lensX = mapFloat(imageX, 0, IMAGE_WIDTH, LENS_X_MIN, LENS_X_MAX);
      float lensY = mapFloat(imageY, 0, IMAGE_HEIGHT, LENS_Y_MIN, LENS_Y_MAX);
      const float timeDeltaSec = (float)(millis() - start) / 1000;      
      offsetBySunMovement(lensX, lensY, sunDirection, sunSpeed, timeDeltaSec);

      // Now calculate the delay we should stay at this pixel for actual 
      // burn to happen before moving to the next one

      // If there are many neighbor pixels burnt already, area is already dark and
      // burn will happen quicker

      // If more than 2 of 3 neighbor pixels were burn on the previous line
      bool prevLineBurnt =
        (prevLine.get(imageX - 1) ? 1 : 0) +
        (prevLine.get(imageX    ) ? 1 : 0) +
        (prevLine.get(imageX + 1) ? 1 : 0) 
          > 2;

      float speed = burn ? (prevLineBurnt ? SPEED_BURN_WHEN_DARK_NEIGHBORS 
                                          : SPEED_BURN) 
                         : SPEED_SKIP;  // In mm/second

      // Apply user-set speed factor
      if (speed != SPEED_SKIP)
        speed = (speed + (0.2 * (speedFactor - 5)));

      #ifdef DEBUG
        Serial.print("speed: ");
        Serial.println(speed);
      #endif
        
      // Calculate burn time for this pixel
      float burnTime = PIXEL_SIZE_MM / speed;  // In seconds
      if (burn && !lastBurn && !prevLineBurnt)
        burnTime += BURN_START_DELAY;

      // Display yellow pixel on a progress image
      const uint16_t progressViewX = (Tft.LCD_WIDTH - IMAGE_WIDTH * 2) / 2 + imageX * 2;
      const uint16_t progressViewY = (Tft.LCD_HEIGHT - ((Tft.LCD_HEIGHT - IMAGE_HEIGHT * 2) / 2 + 20)) - imageY * 2;
      drawQuadPoint(progressViewX, progressViewY, YELLOW);

      // Now move smoothly
      const uint16_t dT = 20;  // ms
      const unsigned long burnTimeMs = burnTime * 1000.0;  // s to ms
      const uint16_t steps = burnTimeMs / 20;

      float startX = ik2dof.x();
      float startY = ik2dof.y();
      float dX = (lensX - startX) / steps;
      float dY = (lensY - startY) / steps;

      const unsigned long startBurnTime = millis();
      for (uint16_t burnStep = 0; burnStep < steps; burnStep++) {
        // Move lens interpolated
        ik2dof.write(startX + dX * burnStep, startY + dY * burnStep);
        delay(dT - (millis() - (startBurnTime + dT * burnStep)));
      }

      // Move lens to final pos
      ik2dof.write(lensX, lensY);

      // Remaining delay
      unsigned long actualBurnTime = millis() - startBurnTime;
      if (burnTimeMs > actualBurnTime)
        delay(burnTimeMs - actualBurnTime);

      // Display final pixel on a progress image
      drawQuadPoint(progressViewX, progressViewY, burn ? BLACK : WHITE);
      lastBurn = burn;

      if (backButton.isClicked())
        return false;

      return true;
    };

    bool done = sunBmp.traverseImageForBurning(burnPixelFunc);
  #endif

  bmpFile.close();

  if (!done) {
    ik2dof.detach();
    return false;
  }

  // Done succesfully
  Tft.lcd_fill_rect(statusX, statusY, 10 * 8, 16, BLACK);
  Tft.lcd_display_string(statusX, statusY, (const uint8_t*)"COMPLETED", FONT_1608, GREEN);

  // now we have to move the lens quick enough to not burn anything
  // until the user clicks "Back"
  int direction = 1;
  int halfSpan = (LENS_X_MAX - LENS_X_MIN) / 2;
  while (true) {
    for (int dx = halfSpan - direction * halfSpan; dx >= 0 && dx < halfSpan * 2 + 1; dx += direction) {
      ik2dof.write(LENS_X_MIN + dx, LENS_Y_MAX);
      delay(20);
      if (backButton.isClicked()) {
        ik2dof.detach();
        return true;
      }
    }
    direction *= -1;
  }
}

void drawQuadPoint(uint16_t x, uint16_t y, uint16_t color) {
  Tft.lcd_draw_point(x, y, color);
  Tft.lcd_draw_point(x+1, y, color);
  Tft.lcd_draw_point(x, y+1, color);
  Tft.lcd_draw_point(x+1, y+1, color);
}

void drawArrow(uint16_t cx, uint16_t cy, uint16_t length, float angleDeg, uint16_t color) {
  float angleRad = angleDeg * (PI / 180.0);

  float dx = cos(angleRad) * (length / 2.0);
  float dy = sin(angleRad) * (length / 2.0);

  // Tip and tail coordinates
  float x0 = cx - dx;  // tail
  float y0 = cy - dy;
  float x1 = cx + dx;  // tip
  float y1 = cy + dy;

  Tft.lcd_draw_line(x0, Tft.LCD_HEIGHT - y0, x1, Tft.LCD_HEIGHT - y1, color);

  // Optional: Draw arrowhead
  float headSize = length * 0.2;  // size of arrowhead
  float angleLeft  = angleRad + PI * 3.0 / 4.0;
  float angleRight = angleRad - PI * 3.0 / 4.0;

  float hx0 = x1 + cos(angleLeft)  * headSize;
  float hy0 = y1 + sin(angleLeft)  * headSize;
  float hx1 = x1 + cos(angleRight) * headSize;
  float hy1 = y1 + sin(angleRight) * headSize;

  Tft.lcd_draw_line(x1, Tft.LCD_HEIGHT - y1, hx0, Tft.LCD_HEIGHT - hy0, color);  // left arrowhead
  Tft.lcd_draw_line(x1, Tft.LCD_HEIGHT - y1, hx1, Tft.LCD_HEIGHT - hy1, color);  // right arrowhead
}

void getBmpFileList(String* destList, int& count) {
  count = 0;

  __XPT2046_CS_DISABLE();

//  pinMode(SD_CS_PIN, OUTPUT);
//  digitalWrite(SD_CS_PIN, HIGH);
//  Sd2Card card;
//  card.init(SPI_FULL_SPEED, SD_CS_PIN); 
  if(!SD.begin(SD_CS_PIN))  { 
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

        if (count >= MAX_FILES_COUNT) 
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
      int x = FILE_LIST_X;
      int y = FILE_LIST_Y + i * FILE_ROW_HEIGHT;

      Tft.lcd_display_string(x, y, (const uint8_t *)filename.c_str(), FONT_1608, (i % 2 == 0) ? WHITE : YELLOW);
  }
}

void calibrateTouch() {
    Tp.tp_init();
    int pointer = 0;

    #ifdef CALIBRATE_TOUCH
      #ifdef DEBUG
        Serial.println("Starting calibration...");
      #endif

      Tp.tp_adjust();
      Tp.tp_dialog();

      // Store calibration in the EEPROM
      tp_dev_t &touchParams = Tp.get_touch_params();
      EEPROM.put(TOUCH_EEPROM_ADDR + pointer, touchParams.iXoff); pointer += sizeof(touchParams.iXoff);
      EEPROM.put(TOUCH_EEPROM_ADDR + pointer, touchParams.iYoff); pointer += sizeof(touchParams.iYoff);
      EEPROM.put(TOUCH_EEPROM_ADDR + pointer, touchParams.fXfac); pointer += sizeof(touchParams.fXfac);
      EEPROM.put(TOUCH_EEPROM_ADDR + pointer, touchParams.fYfac); pointer += sizeof(touchParams.fYfac);
      #ifdef DEBUG
        Serial.println("Touch calibration saved to EEPROM: ");
        Serial.print("  - iXoff: "); Serial.println(touchParams.iXoff);
        Serial.print("  - iYoff: "); Serial.println(touchParams.iYoff);
        Serial.print("  - fXfac: "); Serial.println(touchParams.fXfac);
        Serial.print("  - fYfac: "); Serial.println(touchParams.fYfac);
        Serial.println();
      #endif

      while (true) {
        Tp.tp_draw_board();
        delay(10);
      }
    #else
      // Read calibration from the EEPROM
      tp_dev_t &touchParams = Tp.get_touch_params();

      EEPROM.get(TOUCH_EEPROM_ADDR + pointer, touchParams.iXoff); pointer += sizeof(touchParams.iXoff);
      EEPROM.get(TOUCH_EEPROM_ADDR + pointer, touchParams.iYoff); pointer += sizeof(touchParams.iYoff);
      EEPROM.get(TOUCH_EEPROM_ADDR + pointer, touchParams.fXfac); pointer += sizeof(touchParams.fXfac);
      EEPROM.get(TOUCH_EEPROM_ADDR + pointer, touchParams.fYfac); pointer += sizeof(touchParams.fYfac);

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
  bool prevPressed = false;
  while (true) {
    const bool pressed = Tp.is_pressed(x, y);
    if (! pressed && prevPressed) {
      #ifdef DEBUG
        Serial.print("Touched at: ");
        Serial.print(x);
        Serial.print(", ");
        Serial.println(y);
      #endif

      int index = (y - FILE_LIST_Y) / FILE_ROW_HEIGHT;
      if (index >= 0 && index < count) {
        // Highlight the selected file
        Tft.lcd_display_string(FILE_LIST_X, FILE_LIST_Y + index * FILE_ROW_HEIGHT, (const uint8_t *)list[index].c_str(), FONT_1608, GREEN);
        delay(100);
        return index; // Return the selected index
      }
    }

    delay(10);
    prevPressed = pressed;
  }

  return -1; // Should never get here
}

void offsetXY(float& x, float& y, float angleDeg, float distance) {
  float angleRad = angleDeg * (PI / 180.0);
  x += cos(angleRad) * distance;
  y += sin(angleRad) * distance;
}

float getMeanDeclination(int8_t month) {
  const float declinationByMonth[] = {
    -20.0, // Jan
    -10.0, // Feb
     -2.0, // Mar
     10.0, // Apr
     18.0, // May
     23.0, // Jun
     20.0, // Jul
     12.0, // Aug
      2.0, // Sep
    -10.0, // Oct
    -18.0, // Nov
    -23.0  // Dec
  };
  return declinationByMonth[month - 1];  // month: 1–12
}

float getSunSpeed(int8_t month) {
  const float equatorSunSpeedDeg = 15.0 / 3600.0;  // deg/sec
  float declDeg = getMeanDeclination(month);
  float declRad = declDeg * PI / 180.0;
  float latRad = LATITUDE * PI / 180.0;

  // Adjust angular speed projection based on tilt
  float effectiveSpeedRad = (equatorSunSpeedDeg * PI / 180.0) * abs(cos(latRad - declRad));
  return effectiveSpeedRad * FOCAL_LENGTH;  // mm/s
}

void offsetBySunMovement(float &x, float &y, float sunDirection, float sunSpeed, float timeDeltaSec) {
  const float distance = sunSpeed * timeDeltaSec;

  // Focal point moves inverted compared to the sun direction in the sky,
  // but to compensate focal point movemnt, we need to invert again, hence
  // no inversion in the end
  float angleRad = sunDirection * (PI / 180.0);
  x += cos(angleRad) * distance;
  y += sin(angleRad) * distance;  
}

void draw_middle_label(const char *label, int y, uint16_t color) {
  size_t len = strlen(label);
  Tft.lcd_display_string((Tft.LCD_WIDTH - len*8)/2, y, (const uint8_t*)label, FONT_1608, color);
}
