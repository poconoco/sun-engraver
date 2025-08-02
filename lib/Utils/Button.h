#include <Arduino.h>

class Button {
    public:
        Button(int x, int y, int w, int h, const char* label);
        bool isClicked();

        uint16_t x;
        uint16_t y;
        uint16_t width;
        uint16_t height;

    private:
        const char* _labelText;
        bool _prevPressed = false;

        bool isPressed();
};
