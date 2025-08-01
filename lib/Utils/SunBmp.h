#include <SD.h>

#include "config.h"
#include "BitSet.h"

class SunBmp {
    public:
        SunBmp(SDLib::File &file);
        bool bmpReadHeader();
        void displayPreview();

        template<typename Func>
        bool traverseImageForBurning(Func burnPixel) {
            if (_image_offset == 0) {
                Serial.println("Image offset is not read fron header");
                return false;
            }

            _file.seek(_image_offset);

            BitSet<IMAGE_WIDTH> burnMask[2];
            uint8_t currentMaskIdx = 0;
            uint16_t buffidx = 0;

            // Move lens to start position
            burnPixel(0, 0, false, false, burnMask[0], true);
            delay(250);

            bool leftToRight = true;
            for (uint16_t y = 0; y < IMAGE_HEIGHT; y++) {
                buffidx = 0;

                BitSet<IMAGE_WIDTH> &currentMask = burnMask[currentMaskIdx];
                BitSet<IMAGE_WIDTH> &prevMask = burnMask[(currentMaskIdx + 1) % 2];

                // Convert to monochrome
                // We need to read entire line first, because below we will traverse it 
                // eitehr forward or backward
                for(uint16_t x = 0; x < IMAGE_WIDTH; x++) {
                    const uint32_t allColors = _file.read() + _file.read() + _file.read();
                    currentMask.set(x, allColors < _blackThreshold * 3);
                    buffidx += 3;
                }

                // now we need to traverse line zig-zagging
                if (leftToRight) {
                    for (uint16_t x = 0; x < IMAGE_WIDTH; x++)
                        if (! burnPixel(
                                  x, y, 
                                  currentMask.get(x),
                                  x + 1 < IMAGE_HEIGHT ? currentMask.get(x+1) : false,
                                  prevMask,
                                  leftToRight
                              )
                        ) {
                            return false;
                        }
                } else { // zag
                    for (int x = IMAGE_WIDTH - 1; x >= 0; x--)
                        if (! burnPixel(
                                  x, y, 
                                  currentMask.get(x),
                                  x - 1 >= 0 ? currentMask.get(x-1) : false,
                                  prevMask,
                                  leftToRight
                              )
                        ) {
                            return false;
                        }
                }

                currentMaskIdx = (currentMaskIdx + 1) % 2;
                leftToRight = !leftToRight;
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
