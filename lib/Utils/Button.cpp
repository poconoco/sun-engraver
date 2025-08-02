#include "Button.h" 

#include <Touch.h>
#include <LCD.h>

Button::Button(int x, int y, int w, int h, const char* label) 
        : x(x)
        , y(y)
        , width(w)
        , height(h)
        , _labelText(label) 
{
    Tft.lcd_draw_rect(x, y, width, height, YELLOW); // Outline
    Tft.lcd_draw_rect(x+1, y+1, width-2, height-2, ORANGE);
    // Text at the center
    Tft.lcd_display_string(x + (width - strlen(label) * 8) / 2, y + (height - 16) / 2, (const uint8_t *)label, FONT_1608, WHITE);
}

bool Button::isClicked() {
    const bool pressed = isPressed();
    if (pressed != _prevPressed) {
        // Give feedback in the color of outline
        Tft.lcd_draw_rect(x, y, width, height, pressed ? BROWN : YELLOW);

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
    uint16_t x_, y_;
    if (Tp.is_pressed(x_, y_)) {
        // Keep pressed state even if coordinates moved outside
        if (_prevPressed)
            return true;

        // Check if the touch coordinates are within the button bounds
        if (x_ >= x && x_ <= x + width && y_ >= y && y_ <= y + height) {
            return true;
        }
    }

    return false;
}