#include "SunBmp.h"

#include <Arduino.h>
#include "LCD.h"

SunBmp::SunBmp(SDLib::File &file, uint16_t width, uint16_t height, uint16_t pixelBuffSize)
    : _file(file)
    , _width(width)
    , _height(height)
    , _pixelBuffSize(pixelBuffSize)
    , _blackThreshold(200)  // If avg color intensity (0..255) less than this threshold, use black
{}

bool SunBmp::bmpReadHeader() {
    // read header
    uint32_t tmp;
    uint8_t bmpDepth;
    
    if (read16(_file) != 0x4D42) {
        // magic bytes missing
        return false;
    }

    // read file size
    tmp = read32(_file);
    Serial.print("Image size: ");
    Serial.print(tmp, DEC);
    Serial.println(" bytes");

    // read and ignore creator bytes
    read32(_file);

    _image_offset = read32(_file);
    Serial.print("Offset: ");
    Serial.print(_image_offset, DEC);
    Serial.println(" bytes");

    // read DIB header
    tmp = read32(_file);
    Serial.print("Header size: ");
    Serial.println(tmp, DEC);
    
    uint16_t bmp_width = read32(_file);
    uint16_t bmp_height = read32(_file);

    if(bmp_width != _width || bmp_height != _height)  {
        // Verify that the image is the right size
        Serial.print("Wrong image size: ");
        Serial.print(bmp_width, DEC);
        Serial.print("x");
        Serial.println(bmp_height, DEC);
        return false;
    }

    if (read16(_file) != 1)  // Number of color planes, must be 1
        return false;

    bmpDepth = read16(_file);   
    Serial.print("Bitdepth: ");
    Serial.println(bmpDepth, DEC);

    if (bmpDepth != 24) {
        // only 24 bit depth supported
        Serial.print("Wrong bit depth: ");
        Serial.println(bmpDepth, DEC);
        return false;
    }

    if (read32(_file) != 0) {
        // compression not supported
        Serial.println("Compression not supported");
        return false;
    }

    return true;
}

void SunBmp::displayPreview() {
    if (_image_offset == 0) {
        Serial.println("Image offset is not read fron header");
        return;
    }

    _file.seek(_image_offset);

    uint32_t time = millis();
    uint8_t sdbuffer[_pixelBuffSize * 3]; // 3 * pixels to buffer to house 3 color bytes

    // Leave enough place for image scaled up by 2x
    const int base_x = (Tft.LCD_WIDTH - _width * 2) / 2;
    const int base_y = (Tft.LCD_HEIGHT - _height * 2) / 2 + 20;

    for (uint16_t i = 0; i < _height; i++) {
        for(uint16_t j = 0; j < (_width/_pixelBuffSize); j++) {
            _file.read(sdbuffer, _pixelBuffSize * 3);
            
            uint8_t buffidx = 0;
            int offset_x = j * _pixelBuffSize;
            unsigned long __color[_pixelBuffSize];

            // convert from 24 bit to 16 bit
            for(uint16_t k = 0; k < _pixelBuffSize; k++) {
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
                for (uint16_t m = 0; m < _pixelBuffSize; m ++) {
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
