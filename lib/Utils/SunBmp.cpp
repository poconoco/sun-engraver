#include "SunBmp.h"

#include <Arduino.h>
#include "LCD.h"

unsigned char __Gnbmp_image_offset  = 0;

uint16_t read16(SDLib::File f);
uint32_t read32(SDLib::File f);

bool bmpReadHeader(SDLib::File f) {
    // read header
    uint32_t tmp;
    uint8_t bmpDepth;
    
    if (read16(f) != 0x4D42) {
        // magic bytes missing
        return false;
    }

    // read file size
    tmp = read32(f);
    Serial.print("Image size: ");
    Serial.print(tmp, DEC);
    Serial.println(" bytes");

    // read and ignore creator bytes
    read32(f);

    __Gnbmp_image_offset = read32(f);
    Serial.print("Offset: ");
    Serial.print(__Gnbmp_image_offset, DEC);
    Serial.println(" bytes");

    // read DIB header
    tmp = read32(f);
    Serial.print("Header size: ");
    Serial.println(tmp, DEC);
    
    int bmp_width = read32(f);
    int bmp_height = read32(f);

    if(bmp_width != SUN_BMP_WIDTH || bmp_height != SUN_BMP_HEIGHT)  {
        // Verify that the image is the right size
        Serial.print("Wrong image size: ");
        Serial.print(bmp_width, DEC);
        Serial.print("x");
        Serial.println(bmp_height, DEC);
        return false;
    }

    if (read16(f) != 1)  // Number of color planes, must be 1
        return false;

    bmpDepth = read16(f);   
    Serial.print("Bitdepth: ");
    Serial.println(bmpDepth, DEC);

    if (bmpDepth != 24) {
        // only 24 bit depth supported
        Serial.print("Wrong bit depth: ");
        Serial.println(bmpDepth, DEC);
        return false;
    }

    if (read32(f) != 0) {
        // compression not supported
        Serial.println("Compression not supported");
        return false;
    }

    return true;
}

void displayPreview(SDLib::File bmpFile) {
    bmpFile.seek(__Gnbmp_image_offset);

    uint32_t time = millis();
    uint8_t sdbuffer[BUFFPIXEL_X3]; // 3 * pixels to buffer to house 3 color bytes

    // Leave enough place for image scaled up by 2x
    const int base_x = (Tft.LCD_WIDTH - SUN_BMP_WIDTH * 2) / 2;
    const int base_y = (Tft.LCD_HEIGHT - SUN_BMP_HEIGHT * 2) / 2 + 20;

    for (int i=0; i < SUN_BMP_HEIGHT; i++) {
        for(int j=0; j < (SUN_BMP_WIDTH/BUFFPIXEL); j++) {
            bmpFile.read(sdbuffer, BUFFPIXEL_X3);
            
            uint8_t buffidx = 0;
            int offset_x = j*BUFFPIXEL;
            unsigned long __color[BUFFPIXEL];

            // convert from 24 bit to 16 bit
            for(int k=0; k<BUFFPIXEL; k++) {
                #ifdef KEEP_COLOR
                    // Display in color
                    __color[k] = sdbuffer[buffidx+2]>>3;                        // red
                    __color[k] = __color[k]<<6 | (sdbuffer[buffidx+1]>>2);      // green
                    __color[k] = __color[k]<<5 | (sdbuffer[buffidx+0]>>3);      // blue
                #else
                    // Convert to greyscale for preview
                    uint32_t grey = (sdbuffer[buffidx+2] + sdbuffer[buffidx+1] + sdbuffer[buffidx+0]) / 3;
                    __color[k] = (grey>>3) << 11 | (grey>>2) << 5 | (grey>>3);
                #endif
                buffidx += 3;
            }
                        
            // While writing, enlarge the image by 2x
            // Execute this loop twice for double the size on the Y axis
            for (int dblscan = 0; dblscan < 2; dblscan++) {
                Tft.lcd_set_cursor(base_x + offset_x * 2, Tft.LCD_HEIGHT - (base_y + i * 2 + dblscan));
                Tft.lcd_write_byte(0x2C, LCD_CMD);  //change 0x22 if driver is hx8347d
                __LCD_DC_SET();
                __LCD_CS_CLR();
                for (int m = 0; m < BUFFPIXEL; m ++) {
                    __LCD_WRITE_BYTE(__color[m] >> 8 );
                    __LCD_WRITE_BYTE(__color[m] & 0xFF);
                    // Twice for double the size on the X axis
                    __LCD_WRITE_BYTE(__color[m] >> 8 );
                    __LCD_WRITE_BYTE(__color[m] & 0xFF);
                }
                __LCD_CS_SET();
            }
        }
    }
    
    Serial.print(millis() - time, DEC);
    Serial.println(" ms");    
}

// Convert data from little to big endian for SD card reading
uint16_t read16(File f)
{
    uint16_t d;
    uint8_t b;
    b = f.read();
    d = f.read();
    d <<= 8;
    d |= b;
    return d;
}

// Same for 32 bit
uint32_t read32(File f)
{
    uint32_t d;
    uint16_t b;

    b = read16(f);
    d = read16(f);
    d <<= 16;
    d |= b;
    return d;
}
