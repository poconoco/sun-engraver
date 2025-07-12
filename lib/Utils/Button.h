#include <Arduino.h>

class Button {
    public:
        Button(int x, int y, int w, int h, const char* label);
        bool isClicked();
    
    private:
        uint16_t _x;
        uint16_t _y;
        uint16_t _width;
        uint16_t _height;
        const char* _labelText;
        bool _prevPressed = false;

        bool isPressed();
};
