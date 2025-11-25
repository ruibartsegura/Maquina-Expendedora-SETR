#include "joystick_class.h"

JoystickClass::JoystickClass(int _pinJoyX, int _pinJoyY) {
  pinJoyX = _pinJoyX;
  pinJoyY = _pinJoyY;
}


const char* JoystickClass::get_dir() {
  int Xvalue = 0;
  int Yvalue = 0;

  //leer valores
  Xvalue = analogRead(pinJoyX);
  delay(100);  //es necesaria una pequeña pausa entre lecturas analógicas
  Yvalue = analogRead(pinJoyY);

  // Decide which direction is the joystick aiming

  // CHECK RIGHT
  if (Xvalue > RIGHT_VALUE) {
    // Check if Xvalue is the dominant direction
    int dist_x_max = MAX_VALUE - Xvalue;
    int dist_y_max = MAX_VALUE - Yvalue;
    int dist_y_min = Yvalue - MIN_VALUE;

    if (dist_x_max < dist_y_max && dist_x_max < dist_y_min) {
      return RIGHT;
    }

  // CHECK LEFT
  } else if (Xvalue < LEFT_VALUE) {
    // Check if Xvalue is the dominant direction
    int dist_x_min = Xvalue - MIN_VALUE;
    int dist_y_max = MAX_VALUE - Yvalue;
    int dist_y_min = Yvalue - MIN_VALUE;

    if (dist_x_min < dist_y_max && dist_x_min < dist_y_min) {
      return LEFT;
    }

  // CHECK UP
  } else if (Yvalue < UP_VALUE) {
    // Check if Yvalue is the dominant direction
    int dist_y_min = Yvalue - MIN_VALUE;
    int dist_x_max = MAX_VALUE - Xvalue;
    int dist_x_min = Xvalue - MIN_VALUE;

    if (dist_y_min < dist_x_max && dist_y_min < dist_x_min) {
      return UP;
    }

  // CHECK DOWN
  } else if (Yvalue > DOWN_VALUE) {
    // Check if Yvalue is the dominant direction
    int dist_y_max = MAX_VALUE - Yvalue;
    int dist_x_max = MAX_VALUE - Xvalue;
    int dist_x_min = Xvalue - MIN_VALUE;

    if (dist_y_max < dist_x_max && dist_y_max < dist_x_min) {
      return DOWN;
    }
  }
  
  // If any value is big enough return that
  return NO_MOVE;
}
