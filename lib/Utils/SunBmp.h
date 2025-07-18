#include <SD.h>

#include "config.h"

class SunBmp {
    public:
        SunBmp(SDLib::File &file);
        bool bmpReadHeader();
        void displayPreview();

        template<typename Func>
        bool burnImage(Func burnPixel) {
            if (_image_offset == 0) {
                Serial.println("Image offset is not read fron header");
                return false;
            }

            _file.seek(_image_offset);
            bool burnMaskBuff[IMAGE_WIDTH];
            uint16_t buffidx = 0;

            // Move lens to start position
            burnPixel(0, 0, 0, false);
            delay(500);
            burnPixel(0, 0, 0, true);

            bool zig = true;
            for (uint16_t y = 0; y < IMAGE_HEIGHT; y++) {
                buffidx = 0;

                // Convert to monochrome
                // We need to read entire line first, because below we will traverse it 
                // eitehr forward or backward
                for(uint16_t x = 0; x < IMAGE_WIDTH; x++) {
                    const uint32_t allColors = _file.read() + _file.read() + _file.read();
                    burnMaskBuff[x] = allColors < _blackThreshold * 3;
                    buffidx += 3;
                }

                // now we need to traverse line zig-zagging
                if (zig) {
                    for (uint16_t x = 0; x < IMAGE_WIDTH; x++)
                        if (!burnPixel(x, y, burnMaskBuff[x], true))
                            return false;
                } else { // zag
                    for (int x = IMAGE_WIDTH - 1; x >= 0; x--)
                        if (!burnPixel(x, y, burnMaskBuff[x], true))
                            return false;
                }

                zig = !zig;
            }

            return true;
        };

    private:
        SDLib::File &_file;
        uint32_t _image_offset;
        uint8_t _blackThreshold;

        uint16_t read16(SDLib::File &f);
        uint32_t read32(SDLib::File &f);
};
