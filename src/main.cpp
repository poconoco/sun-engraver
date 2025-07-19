#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <EEPROM.h>

// Uncommenting DEBUG requiers disabling manually some features, otehrwise 
// you'll get memory overflow
//#define DEBUG

//#define CALIBRATE_TOUCH
//#define CALIBRATE_SERVO_ANGLES
 
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
int selectSpeed();
int selectSunDirection();
bool prepFocusLens();
bool focusLens(IK2DOF&);
bool doBurn(IK2DOF&, int, float, String);
void drawQuadPoint(uint16_t, uint16_t, uint16_t);
void drawArrow(uint16_t cx, uint16_t cy, uint16_t length, float angleDeg, uint16_t color);
void offsetXY(float& x, float& y, float angleDeg, float distance);
float getSunSpeed();
void offsetBySunMovement(float &x, float &y, float sunDirection, float sunSpeed, float timeDeltaSec);

void setup() {

  #ifdef DEBUG
    Serial.begin(9600);
  #endif

  FloatServo servoArm1(ARM1_PIN, 465, 2500);
  FloatServo servoArm2(ARM2_PIN, 465, 2750);

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
    int selectedSpeed = -1;  // From 1 to 10
    int selectedSunDirection = -1;
    bool done = false;
    while (true) {
      selectedIndex = selectFile(bmpFiles, bmpCount);
      while (confirmSelection(bmpFiles, selectedIndex)) {
        while (true) {
          selectedSpeed = selectSpeed();
          if (selectedSpeed > 0) {

            while (true) {
              selectedSunDirection = selectSunDirection();
              if (selectedSunDirection > 0) {
                while (prepFocusLens()) {
                  while (focusLens(ik2dof)) {
                    // Need to define this out otherwise out of flash storage when calibration is included
                    #ifndef CALIBRATE_TOUCH
                      if (doBurn(ik2dof, selectedSpeed, selectedSunDirection, bmpFiles[selectedIndex]))
                        done = true;
                    #endif
                    break;
                  }

                  if (done)
                    break;
                }
              } else {
                break;
              }
            }
          } else {
            break;
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
      Tft.lcd_display_string(20, 50, (const uint8_t*)"Failed to open file!", FONT_1608, RED);
      delay(2000);
      return false;
  }

  SunBmp sunBmp(bmpFile);

  if(! sunBmp.bmpReadHeader()) {
    bmpFile.close();
    Tft.lcd_display_string(20, 50, (const uint8_t*)"Bad header,", FONT_1608, RED);
    Tft.lcd_display_string(20, 70, (const uint8_t*)"bmp must be " STR(SUN_BMP_WIDTH) "x" STR(SUN_BMP_HEIGHT) ",", FONT_1608, RED);
    Tft.lcd_display_string(20, 90, (const uint8_t*)"not be compressed and be 24bit", FONT_1608, RED);
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
  Tft.lcd_fill_rect(speedX+1, speedY+1, 30-1, 30-1, GREEN);

  int speed = 5;
  bool redraw = true;
  while (true) {

    if (redraw) {
      Tft.lcd_fill_rect(speedX + 7, speedY + 7, 16, 16, GREEN);
      Tft.lcd_display_string(speedX + (speed < 10 ? 11 : 7), speedY + 7, (const uint8_t*)String(speed).c_str(), FONT_1608, BLACK);
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

int selectSunDirection() {
  Tft.lcd_clear_screen(BLACK);
  Tft.lcd_display_string((Tft.LCD_WIDTH - 27*8)/2, 20, (const uint8_t*)"Set sun movement direction:", FONT_1608, WHITE);

  Button confirmButton(Tft.LCD_WIDTH - 100, Tft.LCD_HEIGHT - 35, 80, 30, "OK >");
  Button backButton(20, Tft.LCD_HEIGHT - 35, 80, 30, "< Back");

  const int cx = Tft.LCD_WIDTH / 2;
  const int cy = Tft.LCD_HEIGHT / 2;

  Button dec(cx + 80 - 15, cy - 15, 30, 30, "-");
  Button inc(cx - 80 - 15, cy - 15, 30, 30, "+");

  int direction = 90;
  int prevDirection = direction;
  bool redraw = true;
  while (true) {

    if (redraw) {
      drawArrow(cx, cy, 60, prevDirection, BLACK);
      drawArrow(cx, cy, 60, direction, ORANGE);
      prevDirection = direction;
      redraw = false;
    }

    if (dec.isClicked()) {
      direction -= 15;
      redraw = true;
      if (direction < 0)
        direction += 360;
    }

    if (inc.isClicked()) {
      direction += 15;
      redraw = true;
      if (direction >= 360)
        direction -= 360;
    }

    if (confirmButton.isClicked()) {
      return direction;
    }

    if (backButton.isClicked()) {
      return -1;
    }
  }
}

bool prepFocusLens() {
  Tft.lcd_clear_screen(BLACK);
  Tft.lcd_display_string(100, 50, (const uint8_t*)"Cover lens with", FONT_1608, WHITE);
  Tft.lcd_display_string(100+2*8, 70, (const uint8_t*)"focusing cap", FONT_1608, WHITE);

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
  Tft.lcd_display_string(100, 50, (const uint8_t*)"Now focus lens", FONT_1608, WHITE);

  Button confirmButton(Tft.LCD_WIDTH - 100, Tft.LCD_HEIGHT - 35, 80, 30, "OK >");
  Button backButton(20, Tft.LCD_HEIGHT - 35, 80, 30, "< Back");

  ik2dof.write(LENS_X_MIN, LENS_Y_MIN);

  while (true) {
    const float maxDelta = LENS_X_MAX - LENS_X_MIN;
    // Move along four sides of square
    for (int direction = 0; direction < 4; direction++) {

      // A timeout at the corner to allow for focusing at the corners easier
      // Have to process buttons inside it as well, to not degrade responsiveness
      for (uint8_t i = 0; i < 100; i++) {
        if (confirmButton.isClicked()) {
          return true;
        }

        if (backButton.isClicked()) {
          ik2dof.detach();
          return false;
        }
        delay(10);
      }

      delay(1000);

      for (float i = 0; i < maxDelta; i++) {
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
        if (confirmButton.isClicked()) {
          return true;
        }

        if (backButton.isClicked()) {
          ik2dof.detach();
          return false;
        }
        delay(10);
      }
    }
  }
}

bool doBurn(IK2DOF& ik2dof, int speedFactor, float sunDirection, String filename) {
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
    ik2dof.write(lensXmin, lensYmin);
    bool done = true;
  #else
    bool lastBurn = false;
    const float sunSpeed = getSunSpeed();
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
      const uint16_t progressViewX = (Tft.LCD_WIDTH - IMAGE_WIDTH * 2) / 2 + imageX * 2;
      const uint16_t progressViewY = (Tft.LCD_HEIGHT - ((Tft.LCD_HEIGHT - IMAGE_HEIGHT * 2) / 2 + 20)) - imageY * 2;
      drawQuadPoint(progressViewX, progressViewY, YELLOW);

      float lensX = mapFloat(imageX, 0, IMAGE_WIDTH, LENS_X_MIN, LENS_X_MAX);
      float lensY = mapFloat(imageY, 0, IMAGE_HEIGHT, LENS_Y_MIN, LENS_Y_MAX);
      const float timeDeltaSec = (float)(millis() - start) / 1000;
      
      offsetBySunMovement(lensX, lensY, sunDirection, sunSpeed, timeDeltaSec);

      ik2dof.write(lensX, lensY);

      // If more than 2 of 3 neighbor pixels were burn on the previous line
      bool prevLineBurnt = 
        (prevLine.get(imageX - 1) ? 1 : 0) +
        (prevLine.get(imageX    ) ? 1 : 0) +
        (prevLine.get(imageX + 1) ? 1 : 0) 
          > 2;

      float speed = burn ? (prevLineBurnt ? SPEED_BURN_WHEN_DARK_NEIGHBORS 
                                          : SPEED_BURN) 
                         : SPEED_SKIP;  // In mm/second

      // Apply speed factor
      if (speed != SPEED_SKIP)
        speed = (speed + (0.2 * (speedFactor - 5)));
        
      float pixelDistance = (LENS_X_MAX - LENS_X_MIN) / (float)IMAGE_WIDTH;  // In mm
      float deltaT = pixelDistance / speed;  // In seconds

      #ifdef DEBUG
        Serial.print("speed: ");
        Serial.println(speed);
      #endif

      if (burn && !lastBurn && !prevLineBurnt)
        delay(BURN_START_DELAY);

      delay((int)(deltaT * 1000.0));

      drawQuadPoint(progressViewX, progressViewY, burn ? BLACK : WHITE);
      lastBurn = burn;

      if (backButton.isClicked())
        return false;

      return true;
    };

    bool done = sunBmp.burnImage(burnPixelFunc);
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

      while (true) {
        Tp.tp_draw_board();
        delay(10);
      }
    #else
      // Read calibration from the EEPROM
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

float getSunSpeed() {
  const float degreePerSec = 15.0 * cos(LATITUDE) / 3600;
  return degreePerSec * (PI / 180) * FOCAL_LENGTH;
}

void offsetBySunMovement(float &x, float &y, float sunDirection, float sunSpeed, float timeDeltaSec) {
  const float distance = sunSpeed * timeDeltaSec;

  // Invert sun direction, because focal point moves inverted compared to the sun in the sky
  float angleRad = (sunDirection + 180) * (PI / 180.0);
  x += cos(angleRad) * distance;
  y += sin(angleRad) * distance;  
}
