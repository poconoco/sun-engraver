#include "FloatServo.h"

float mapFloat(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

FloatServo::FloatServo(Adafruit_PWMServoDriver &pwm, uint8_t channel, int min, int max) 
    : _attached(false)
    , _pwm(pwm)
    , _channel(channel)
    , _min(min)
    , _max(max)
{}

void FloatServo::attach() {
    if (_attached)
        return;

    _attached = true;
}

void FloatServo::detach() {
    if (!_attached)
        return;

    _pwm.setPWM(_channel, 0, 0);
    _attached = false;
}

void FloatServo::writeFloat(float angle) {
    int pulse = mapFloat(angle, 0.0, 180.0, _min, _max);
    _pwm.writeMicroseconds(_channel, pulse);
}

