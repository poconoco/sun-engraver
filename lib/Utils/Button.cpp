#include "Button.h" 

#include <Touch.h>
#include <LCD.h>

Button::Button(int x, int y, int w, int h, const char* label) 
        : _x(x)
        , _y(y)
        , _width(w)
        , _height(h)
        , _labelText(label) 
{
    Tft.lcd_draw_rect(_x, _y, _width, _height, YELLOW); // Outline
    Tft.lcd_draw_rect(_x+1, _y+1, _width-2, _height-2, ORANGE);
    // Text at the center
    Tft.lcd_display_string(_x + (_width - strlen(label) * 8) / 2, _y + (_height - 16) / 2, (const uint8_t *)label, FONT_1608, WHITE);
}

bool Button::isClicked() {
    const bool pressed = isPressed();
    if (pressed != _prevPressed) {
        // Give feedback in the color of outline
        Tft.lcd_draw_rect(_x, _y, _width, _height, pressed ? BROWN : YELLOW);

        // Debounce and ensure there is time to see the color change
        if (pressed)
            delay(100);
    }
    
    

    if (!pressed && _prevPressed) {
        _prevPressed = false;
        return true; // Button was just released, emit click response
    }

    // Only return true once when the button is released
    _prevPressed = pressed; // Update previous state
    return false;
}

bool Button::isPressed() {
    uint16_t x, y;
    if (Tp.is_pressed(x, y)) {
        // Keep pressed state even if coordinates moved outside
        if (_prevPressed)
            return true;

        // Check if the touch coordinates are within the button bounds
        if (x >= _x && x <= _x + _width && y >= _y && y <= _y + _height) {
            return true;
        }
    }

    return false;
}