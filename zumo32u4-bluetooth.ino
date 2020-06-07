/* 
- HM-10 Control
- go straight with gyro
- rotate with gyro
- stop if obstacle ahead
*/
#include <Wire.h>
#include <Zumo32U4.h>
#include "TurnSensor.h"

Zumo32U4ProximitySensors proxSensors;
Zumo32U4Encoders encoders;
Zumo32U4Motors motors;
Zumo32U4ButtonA buttonA;
L3G gyro;
Zumo32U4LCD lcd;

const uint16_t MOTOR_SPEED = 350;
const uint16_t TURN_SPEED = 190;
const int ACCELERATION = 2;

int curSpeed = 0;
int obstacleDistance = 6;

enum State {
  pause_state,
  forward_left_state,
  forward_right_state,
  scan_left_state,
  scan_right_state,
  reverse_state,
  forward_state
};

State state = pause_state;

char state_prev = state;

String msg; 

// --- Setup function
void setup()
{
  Serial1.begin(9600);
  // Get the proximity sensors initialized
  proxSensors.initThreeSensors();
  // gyro calibration, don't move!!!
  resetEncoders();
  turnSensorSetup();
  delay(500);
  turnSensorReset();
}

void resetEncoders() {
  encoders.getCountsAndResetLeft();
  encoders.getCountsAndResetRight();
}
// --- Main program loop
void loop()
{
  bool buttonPress = buttonA.getSingleDebouncedPress();

  // --- Read the sensors
  proxSensors.read();
  int left_sensor = proxSensors.countsLeftWithLeftLeds();
  int center_left_sensor = proxSensors.countsFrontWithLeftLeds();
  int center_right_sensor = proxSensors.countsFrontWithRightLeds();
  int right_sensor = proxSensors.countsRightWithRightLeds();

 // BT
 if (Serial1.available()){      
     msg = Serial1.readString(); 
     if (msg == "fwd") 
         state = forward_state;
      else if (msg == "rev") 
         state = reverse_state;
      else if (msg == "lft") 
         state = scan_left_state;
      else if (msg == "rgt") 
         state = scan_right_state;
      else if (msg == "stp") 
         state = pause_state;
      else if (msg == "sensor") {
          static char buffer[80];
          sprintf(buffer, "%d %d %d %d\n",left_sensor,center_left_sensor,center_right_sensor,right_sensor);
          Serial1.println(buffer);
      } else 
         Serial1.println("unkown command");
  }

 // --- Change states
  if (state == pause_state) {
    if (buttonPress) {
      state = forward_state;
      resetEncoders();
    }
  }
  else if (buttonPress) {
    state = pause_state;
  } else if (state == forward_state) {  
    if (center_left_sensor >= obstacleDistance && center_right_sensor >= obstacleDistance) {
        Serial1.println("obstacle ahead");
        state = pause_state;
    }
  }

    // --- Set motor speed
  if (state != pause_state && curSpeed < MOTOR_SPEED) {
    curSpeed += ACCELERATION;
  }

  
  if (state == pause_state) {
    stop();
    curSpeed = 0;
  } else if (state == forward_state)
    forward();
   else if (state == scan_left_state)
    turnLeft(90);
  else if (state == scan_right_state)
    turnRight(90);
  else if (state == reverse_state) {
    reverse();
    delay(250);
     turnLeft(135);
    delay(250);
    state = pause_state;
    curSpeed = 0;
    resetEncoders();
  }

  if (state != state_prev) {
    state_prev = state;
    String str = "";
    switch (state) {
      case pause_state: str = "pause"; break;
      case forward_state: str = "forward"; break;
      case reverse_state: str = "reverse"; break;
      case scan_left_state: str = "left"; break;
      case scan_right_state: str = "right"; break;
      default: break;
    }
    Serial1.print("state: ");
    Serial1.println(str);
  }
}

void forward() {
  // Get out heading
  turnSensorUpdate();
  int angle = (((int32_t)turnAngle >> 16) * 360) >> 16;

  // Move foward, adjusting motor speed to hold heading
  motors.setSpeeds(curSpeed + (angle * 5), curSpeed - (angle * 5));
}

void stop() {
  motors.setSpeeds(0, 0);
}

void reverse() {
  motors.setSpeeds(-curSpeed, -curSpeed);
}
// Turn left
void turnLeft(int degrees) {
  turnSensorReset();
  motors.setSpeeds(-TURN_SPEED, TURN_SPEED);
  int angle = 0;
  do {
    delay(1);
    turnSensorUpdate();
    angle = (((int32_t)turnAngle >> 16) * 360) >> 16;
    //Serial1.println(angle);
  } while (angle < degrees);
    resetEncoders();
    turnSensorReset();
    state = pause_state;
   curSpeed = 0;
}

// Turn right
void turnRight(int degrees) {
  turnSensorReset();
  motors.setSpeeds(TURN_SPEED, -TURN_SPEED);
  int angle = 0;
  do {
    delay(1);
    turnSensorUpdate();
    angle = (((int32_t)turnAngle >> 16) * 360) >> 16;
    //Serial1.println(angle);
  } while (angle > -degrees);
      resetEncoders();
      turnSensorReset();
    state = pause_state;
   curSpeed = 0;

}
