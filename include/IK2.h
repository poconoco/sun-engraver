#include <math.h>
#include <Servo.h>

float sqr(float value) {
    return value * value;
}

class IK2DOF {
public:
    IK2DOF(
        float arm1Length,
        float arm2Length,
        float arm1ZeroAngle, // Angle to write to servo so the arm1 is straight forward 
        float arm2ZeroAngle, // Angle to write to servo so the arm2 is parallel to arm1
        bool arm1Inverted,
        bool arm2Inverted,
        Servo &servoArm1, // Provided servos should be already attached (and make sure min and max pulse lengts are calibrated)
        Servo &servoArm2) 
      : _arm1Length(arm1Length)
      , _arm2Length(arm2Length)
      , _arm1ZeroAngle(arm1ZeroAngle)
      , _arm2ZeroAngle(arm2ZeroAngle)
      , _arm1Inverted(arm1Inverted)
      , _arm2Inverted(arm2Inverted)
      , _servoArm1(servoArm1)
      , _servoArm2(servoArm2) {
    }

    void write(float x, float y) {
        #ifdef DEBUG
            Serial.println("  -- Reaching for coordinates:");
            Serial.print(x);
            Serial.print(", ");
            Serial.print(y);
            Serial.println();
        #endif

        // Get coordinates of the arm1-arm2 joint
        IK2DOF::Point2D joint = circleIntersection(x, y, _arm1Length, _arm2Length);
        if (joint.invalid) {
            #ifdef DEBUG
                Serial.println("No solution found for the given coordinates.");
            #endif
            return;
        }

        #ifdef DEBUG
            Serial.println("Got the following solution:");
            Serial.print(joint.x);
            Serial.print(", "); 
            Serial.print(joint.y);
            Serial.println();
        #endif

        // Get arm angles
        float arm1AbsAngle = atan2(joint.y, joint.x) * 180 / M_PI;
        float arm2AbsAngle = atan2(y - joint.y, x - joint.x) * 180 / M_PI;
        float arm1Angle = _arm1ZeroAngle - 90 + (_arm1Inverted ? -1.0 : 1.0) * arm1AbsAngle;
        float arm2Angle = _arm2ZeroAngle - 180 + (_arm2Inverted ? -1.0 : 1.0) * (180 - arm1AbsAngle + arm2AbsAngle);

        #ifdef DEBUG
            Serial.println("Got the following absolute angles:");
            Serial.print(arm1AbsAngle);
            Serial.print(", ");
            Serial.print(arm2AbsAngle);
            Serial.println();

            Serial.println("Got the following angles to write to servos:");
            Serial.print(arm1Angle);
            Serial.print(", ");
            Serial.print(arm2Angle);
            Serial.println();
        #endif

        _servoArm1.write(arm1Angle);
        _servoArm2.write(arm2Angle);
    }

private:
    float _arm1Length;
    float _arm2Length;
    float _arm1ZeroAngle;
    float _arm2ZeroAngle;
    bool _arm1Inverted;
    bool _arm2Inverted;
    Servo &_servoArm1;
    Servo &_servoArm2;

    struct Point2D {
        float x;
        float y;
        bool invalid;
    };

    IK2DOF::Point2D circleIntersection(float x2, float y2, float r1, float r2) {  // x1,y1 assumed to be 0,0
        float d = sqrt(x2 * x2 + y2 * y2); // distance between circle centers

        IK2DOF::Point2D result;

        // No solution
        if (d == 0) {
            result.invalid = true;
            return result;
        }

        float a = (r1 * r1 - r2 * r2 + d * d) / (2 * d);

        // Circles don't intersect
        if (r1 < a) {
            result.invalid = true;
            return result;
        }

        float h = sqrt(r1 * r1 - a * a);

        float xt = a * (x2 / d);
        float yt = a * (y2 / d);

        // Two solutions
        float jointXa = xt + h * (y2 / d);
        float jointYa = yt - h * (x2 / d);
        float jointXb = xt - h * (y2 / d);
        float jointYb = yt + h * (x2 / d);

        // Pick the one that has higher joint
        if (jointYa > jointYb) {
            result.x = jointXa;
            result.y = jointYa;
        } else {
            result.x = jointXb;
            result.y = jointYb;
        }

        result.invalid = false;

        return result;
    }
};
