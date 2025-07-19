#define TOUCH_EEPROM_ADDR 0
#define SPEED_EEPROM_ADDR 16
#define SUN_DIR_EEPROM_ADDR 18

#define MAX_FILES_COUNT 11

#define ARM1_PIN 6
#define ARM2_PIN 8
#define SD_CS_PIN 5

#define FILE_ROW_HEIGHT 21 // Height of each file row in pixels
#define FILE_LIST_X 20 // X position of the file list
#define FILE_LIST_Y 10 // Y position of the first file row

// Should be square (LENS_X_MAX - LENS_X_MIN == LENS_Y_MAX - LENS_Y_MIN)
#define LENS_X_MIN 50
#define LENS_X_MAX 110
#define LENS_Y_MIN -15
#define LENS_Y_MAX 45

// Also should be square
#define IMAGE_WIDTH 100
#define IMAGE_HEIGHT 100
#define IMAGE_BUFF_SIZE 10  // Should be a divisor of imageHeight

#define SPEED_SKIP 75  // mm/s
#define SPEED_BURN 2.4  // mm/s
#define SPEED_BURN_WHEN_DARK_NEIGHBORS 3  // mm/s, speed when on previous line there are many burnt neighbors
#define BURN_START_DELAY 250  // ms, additional time to start dark pixel after white

#define ARM1_STRAIGHT_BRACKET_ANGLE 98
#define ARM2_STRAIGHT_BRACKET_ANGLE 74
#define ARM2_BRACKET_TO_LENS_ANGLE 60

// This latitude and lens focal length is used to estimate sun movement speed
#define LATITUDE 48.29  // Chernivtsi, Ukraine
#define FOCAL_LENGTH 190  // mm

