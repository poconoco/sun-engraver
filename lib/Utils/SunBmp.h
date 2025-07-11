#include <SD.h>

#define SUN_BMP_WIDTH 100
#define SUN_BMP_HEIGHT 100

#define BUFFPIXEL    25   // must be a divisor of HEIGHT 
#define BUFFPIXEL_X3 75   // BUFFPIXELx3

bool bmpReadHeader(SDLib::File f);
void displayPreview(SDLib::File f);