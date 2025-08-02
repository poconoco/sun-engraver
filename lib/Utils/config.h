#define TOUCH_EEPROM_ADDR 0
#define SPEED_EEPROM_ADDR 16
#define SUN_DIR_EEPROM_ADDR 18
#define MONTH_EEPROM_ADDR 20
#define HALF_SCAN_EEPROM_ADDR 22

#define MAX_FILES_COUNT 11

#define ARM1_PIN 6
#define ARM2_PIN 8
#define SD_CS_PIN 5

#define FILE_ROW_HEIGHT 21 // Height of each file row in pixels
#define FILE_LIST_X 20 // X position of the file list
#define FILE_LIST_Y 10 // Y position of the first file row

// Should be square (LENS_X_MAX - LENS_X_MIN == LENS_Y_MAX - LENS_Y_MIN)
#define LENS_X_MIN 60
#define LENS_X_MAX 120
#define LENS_Y_MIN -15
#define LENS_Y_MAX 45

// Also should be square
#define IMAGE_WIDTH 100
#define IMAGE_HEIGHT 100
#define IMAGE_BUFF_SIZE 10  // Should be a divisor of imageHeight

#define PIXEL_SIZE_MM ((float)(LENS_X_MAX - LENS_X_MIN) / (float)IMAGE_WIDTH)

#define SPEED_SKIP 75  // mm/s
#define SPEED_BURN 2.0  // mm/s
#define SPEED_BURN_WHEN_DARK_NEIGHBORS 3.0  // mm/s, speed when on previous line there are many burnt neighbors
#define BURN_START_DELAY_MIN 0.250  // s, additional time to start dark pixel after white
#define BURN_START_DELAY_MAX 2.0  // s, additional time to start dark pixel after white

#define ARM1_STRAIGHT_BRACKET_ANGLE 88
#define ARM2_STRAIGHT_BRACKET_ANGLE 90
#define ARM2_BRACKET_TO_LENS_ANGLE 60

// This latitude and lens focal length is used to estimate sun movement speed
#define LATITUDE 48.29  // Chernivtsi, Ukraine
#define FOCAL_LENGTH 180  // mm

