#include "SunBmp.h"

#include <Arduino.h>
#include "LCD.h"

SunBmp::SunBmp(SDLib::File &file)
    : _file(file)
    , _blackThreshold(200)  // If avg color intensity (0..255) less than this threshold, use black
{}

bool SunBmp::bmpReadHeader() {
    // read header
    #ifdef DEBUG
        uint32_t tmp;
    #endif
    uint8_t bmpDepth;
    
    if (read16(_file) != 0x4D42) {
        // magic bytes missing
        return false;
    }

    // read file size
    #ifdef DEBUG
        tmp = read32(_file);
        Serial.print("Image size: ");
        Serial.print(tmp, DEC);
        Serial.println(" bytes");
    #else
        read32(_file);
    #endif

    // read and ignore creator bytes
    read32(_file);

    _image_offset = read32(_file);
    #ifdef DEBUG
        Serial.print("Offset: ");
        Serial.print(_image_offset, DEC);
        Serial.println(" bytes");
    #endif

    // read DIB header
    #ifdef DEBUG
        tmp = read32(_file);
        Serial.print("Header size: ");
        Serial.println(tmp, DEC);
    #else
        read32(_file);
    #endif
    
    uint16_t bmp_width = read32(_file);
    uint16_t bmp_height = read32(_file);

    if(bmp_width != IMAGE_WIDTH || bmp_height != IMAGE_HEIGHT)  {
        // Verify that the image is the right size
        #ifdef DEBUG
            Serial.print("Wrong image size: ");
            Serial.print(bmp_width, DEC);
            Serial.print("x");
            Serial.println(bmp_height, DEC);
        #endif
        return false;
    }

    if (read16(_file) != 1)  // Number of color planes, must be 1
        return false;

    bmpDepth = read16(_file);   
    #ifdef DEBUG
        Serial.print("Bitdepth: ");
        Serial.println(bmpDepth, DEC);
    #endif

    if (bmpDepth != 24) {
        // only 24 bit depth supported
        #ifdef DEBUG
            Serial.print("Wrong bit depth: ");
            Serial.println(bmpDepth, DEC);
        #endif
        return false;
    }

    if (read32(_file) != 0) {
        // compression not supported
        #ifdef DEBUG
            Serial.println("Compression not supported");
        #endif
        return false;
    }

    return true;
}

void SunBmp::displayPreview() {
    if (_image_offset == 0) {
        #ifdef DEBUG
            Serial.println("Image offset is not read fron header");
        #endif
        return;
    }

    _file.seek(_image_offset);

    uint8_t sdbuffer[IMAGE_BUFF_SIZE * 3]; // 3 * pixels to house 3 color bytes

    // Leave enough place for image scaled up by 2x
    const int base_x = (Tft.LCD_WIDTH - IMAGE_WIDTH * 2) / 2;
    const int base_y = (Tft.LCD_HEIGHT - IMAGE_HEIGHT * 2) / 2 + 20;

    for (uint16_t i = 0; i < IMAGE_HEIGHT; i++) {
        for(uint16_t j = 0; j < (IMAGE_WIDTH/IMAGE_BUFF_SIZE); j++) {
            _file.read(sdbuffer, IMAGE_BUFF_SIZE * 3);
            
            uint8_t buffidx = 0;
            int offset_x = j * IMAGE_BUFF_SIZE;
            uint16_t __color[IMAGE_BUFF_SIZE];

            // convert from 24 bit to 16 bit
            for(uint16_t k = 0; k < IMAGE_BUFF_SIZE; k++) {
                #ifdef KEEP_COLOR
                    // Display in color
                    __color[k] = sdbuffer[buffidx+2]>>3;                        // red
                    __color[k] = __color[k]<<6 | (sdbuffer[buffidx+1]>>2);      // green
                    __color[k] = __color[k]<<5 | (sdbuffer[buffidx+0]>>3);      // blue
                #else
                    // Convert to monochrome for preview
                    uint32_t allColors = (sdbuffer[buffidx+2] + sdbuffer[buffidx+1] + sdbuffer[buffidx+0]);
                    __color[k] = (allColors > _blackThreshold * 3) ? WHITE : BLACK;
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
                for (uint16_t m = 0; m < IMAGE_BUFF_SIZE; m ++) {
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
}

// Convert data from little to big endian for SD card reading
uint16_t SunBmp::read16(File &f)
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
uint32_t SunBmp::read32(File &f)
{
    uint32_t d;
    uint16_t b;

    b = read16(f);
    d = read16(f);
    d <<= 16;
    d |= b;
    return d;
}
