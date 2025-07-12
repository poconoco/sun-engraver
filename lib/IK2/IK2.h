#include <math.h>
#include <Servo.h>

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
        Servo &servoArm2
    );

    void write(float x, float y);

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

    IK2DOF::Point2D circleIntersection(float x2, float y2, float r1, float r2);  // x1,y1 assumed to be 0,0

};
