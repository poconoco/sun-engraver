#include <SD.h>

#include "config.h"
#include "BitSet.h"

class SunBmp {
    public:
        SunBmp(SDLib::File &file);
        bool bmpReadHeader();
        void displayPreview();

        template<typename Func>
        bool traverseImageForBurning(Func burnPixel, bool halfScan) {
            if (_image_offset == 0) {
                Serial.println("Image offset is not read fron header");
                return false;
            }

            _file.seek(_image_offset);

            BitSet<IMAGE_WIDTH> burnMask[2];
            BitSet<IMAGE_WIDTH> &prevMask = burnMask[0];
            uint8_t currentMaskIdx = 0;

            // Move lens to start position
            burnPixel(0, 0, false, false, false, burnMask[0]);
            delay(250);

            for (uint16_t y = 0; y < IMAGE_HEIGHT; y++) {
                BitSet<IMAGE_WIDTH> &currentMask = burnMask[currentMaskIdx];

                // Convert to monochrome
                // We need to read entire line first, because below we will traverse it 
                // eitehr forward or backward
                for(uint16_t x = 0; x < IMAGE_WIDTH; x++) {
                    const uint32_t allColors = _file.read() + _file.read() + _file.read();
                    currentMask.set(x, allColors < _blackThreshold * 3);
                }

                if (halfScan && (y % 2))
                    continue;

                bool prevBurn = false;
                // now traverse along x
                for (uint16_t x = 0; x < IMAGE_WIDTH; x++) {
                    const bool burn = currentMask.get(x);
                    const bool nextBurn = x + 1 < IMAGE_HEIGHT ? currentMask.get(x+1) : false;
                    if (! burnPixel(
                                x, y, 
                                burn,
                                prevBurn,
                                nextBurn,
                                prevMask
                            )
                    ) {
                        return false;
                    }

                    prevBurn = burn;
                }

                // rewind
                burnPixel(
                    0, y, 
                    false,
                    false,
                    false,
                    prevMask
                );

                currentMaskIdx = (currentMaskIdx + 1) % 2;
                prevMask = currentMask;
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
