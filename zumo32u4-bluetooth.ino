/* 
- HM-10
- korrigertes Geradeausfahren mit Encoder
- Drehung nur über delay
*/
#include <Zumo32U4.h>

Zumo32U4ProximitySensors proxSensors;
Zumo32U4Encoders encoders;
Zumo32U4Motors motors;
Zumo32U4ButtonA buttonA;

#define Kp 1

const uint16_t MOTOR_SPEED = 250;
const uint16_t TURN_SPEED = 190;
const int ACCELERATION = 4;

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
  // Get the proximity sensors initialized
  proxSensors.initThreeSensors();
  resetEncoders();
  Serial1.begin(9600);
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
    straight();
   else if (state == scan_left_state)
    turnLeft();
  else if (state == scan_right_state)
    turnRight();
  else if (state == reverse_state) {
    reverse();
    delay(500);
    turnLeft();
    turnLeft();
    turnLeft();
    state = pause_state;
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
// go straight via encoders
int rightOffset() {
  return Kp*(encoders.getCountsLeft() - encoders.getCountsRight()); 
}
void straight() {
    motors.setSpeeds(curSpeed, curSpeed + rightOffset());
}

void turnRight() {
    motors.setSpeeds(TURN_SPEED, -TURN_SPEED);
    delay(500);
     curSpeed = 0;
    resetEncoders();
    state = pause_state;

}
void turnLeft() {
    motors.setSpeeds(-TURN_SPEED, TURN_SPEED);
    delay(500);
     curSpeed = 0;
    resetEncoders();
    state = pause_state;

}
void stop() {
  curSpeed = 0;
  motors.setSpeeds(0, 0);
}

void reverse() {
  motors.setSpeeds(-curSpeed, -curSpeed);
}
