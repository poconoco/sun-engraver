#include <SD.h>

class SunBmp {
    public:
        SunBmp(SDLib::File &file, uint16_t width, uint16_t height, uint16_t pixelBuffSize);
        bool bmpReadHeader();
        void displayPreview();

        template<typename Func>
        bool burnImage(Func burnPixel) {
            if (_image_offset == 0) {
                Serial.println("Image offset is not read fron header");
                return false;
            }

            _file.seek(_image_offset);
            uint8_t sdBuff[_width * 3]; // 3 * pixels to buffer to house 3 color bytes
            uint8_t lineBuff[_width];
            uint16_t buffidx = 0;

            // Move lens to start position
            burnPixel(0, 0, 0, false);
            delay(500);
            burnPixel(0, 0, 0, true);

            bool zig = true;
            for (uint16_t y = 0; y < _height; y++) {
                _file.read(sdBuff, _width * 3);
                buffidx = 0;

                // convert from 24 bit to 8 bit greyscale
                for(uint16_t x = 0; x < _width; x++) {
                    lineBuff[x] = (sdBuff[buffidx+2] + sdBuff[buffidx+1] + sdBuff[buffidx+0]) / 3;
//                    Serial.println(lineBuff[x]);
                    buffidx += 3;
                }

                // now we need to traverse line zig-zagging
                if (zig) {
                    for (uint16_t x = 0; x < _width; x++)
                        if (!burnPixel(x, y, 255 - lineBuff[x], true))
                            return false;
                } else { // zag
                    for (int x = _width - 1; x >= 0; x--)
                        if (!burnPixel(x, y, 255 - lineBuff[x], true))
                            return false;
                }

                zig = !zig;
            }

            return true;
        };

    private:
        SDLib::File &_file;
        uint16_t _width;
        uint16_t _height;
        uint16_t _pixelBuffSize;
        uint32_t _image_offset;

        uint16_t read16(SDLib::File &f);
        uint32_t read32(SDLib::File &f);
};
