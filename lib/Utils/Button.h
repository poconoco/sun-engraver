

class Button {
    public:
        Button(int x, int y, int w, int h, const char* label);
        bool isClicked();
    
    private:
        int _x;
        int _y;
        int _width;
        int _height;
        const char* _labelText;
        bool _prevPressed = false;

        bool isPressed();
};
