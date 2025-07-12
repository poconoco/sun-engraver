#include <SD.h>

class SunBmp {
    public:
        SunBmp(SDLib::File &file, uint16_t width, uint16_t height, uint16_t pixelBuffSize);
        bool bmpReadHeader();
        void displayPreview();
    private:
        SDLib::File &_file;
        uint16_t _width;
        uint16_t _height;
        uint16_t _pixelBuffSize;
        uint32_t _image_offset;

        uint16_t read16(SDLib::File &f);
        uint32_t read32(SDLib::File &f);
};
