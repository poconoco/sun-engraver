
#include "HFServo.h"

#ifndef FLOAT_SERVO_H
#define FLOAT_SERVO_H

float mapFloat(float x, float in_min, float in_max, float out_min, float out_max);

class FloatServo: private HFServo {
    public:
        FloatServo(int pin, int min, int max);

        void attach();
        void detach();
        void writeFloat(float angle);

    private:
        bool _attached;
        int _pin;
        int _min;
        int _max;
};

#endif
