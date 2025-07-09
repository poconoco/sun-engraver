#include <Arduino.h>

//#define DEBUG

#include "IK2.h"

const int arm1Pin = 10;
const int arm2Pin = 11;

const float arm1ZeroAngle = 95;
const float arm2ZeroAngle = 82+60;


void setup() {

  Serial.begin(9600);

  Servo servoArm1;
  Servo servoArm2;

  servoArm1.attach(arm1Pin, 550, 2500);
  servoArm2.attach(arm2Pin, 550, 2450);

  IK2DOF ik2dof(
      70,  // arm1 length
      70,  // arm2 length
      arm1ZeroAngle,
      arm2ZeroAngle,
      false,  // arm1 not inverted
      false,  // arm2 not inverted
      servoArm1,
      servoArm2
  );

  ik2dof.write(40, 60);

//  ik2dof.write(75, 60);

//  servoArm1.write(arm1ZeroAngle);
//  servoArm2.write(arm2ZeroAngle);

  float angle = 0;
  float cx = 70;
  float cy = 70;
  float r = 40;

  while (true) {

    float x = cx + r * cos(radians(angle));
    float y = cy + r * sin(radians(angle));

    ik2dof.write(x, y);

    delay(10);

    angle += 1;
    if (angle > 360) {
      angle = 0;
    }
  }
}
