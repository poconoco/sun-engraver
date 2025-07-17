#include <Adafruit_PWMServoDriver.h>

#ifndef FLOAT_SERVO_H
#define FLOAT_SERVO_H

float mapFloat(float x, float in_min, float in_max, float out_min, float out_max);

class FloatServo {
    public:
        FloatServo(Adafruit_PWMServoDriver &pwm, uint8_t channel, int min, int max);

        void attach();
        void detach();
        void writeFloat(float angle);

    private:
        bool _attached;
        Adafruit_PWMServoDriver &_pwm;
        uint8_t _channel;
        int _min;
        int _max;
};

#endif
