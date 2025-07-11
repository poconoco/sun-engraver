#include <SD.h>

#define SUN_BMP_WIDTH 320
#define SUN_BMP_HEIGHT 240

bool bmpReadHeader(SDLib::File f);
void displayPreview(SDLib::File f);