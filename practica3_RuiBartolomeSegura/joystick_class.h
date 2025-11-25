#ifndef JOYSTICK_CLASS_H
#define JOYSTICK_CLASS_H

#include <Arduino.h>


class JoystickClass {
public:
  JoystickClass(int _pinJoyX, int _pinJoyY);

  const char* JoystickClass::get_dir();

private:
  int pinJoyX;
  int pinJoyY;

  static const int RIGHT_VALUE = 800;
  static const int LEFT_VALUE = 200;
  static const int DOWN_VALUE = 800;
  static const int UP_VALUE = 200;

  static const int MAX_VALUE = 1050;
  static const int MIN_VALUE = -50;

  static constexpr const char* RIGHT = "RIGHT";
  static constexpr const char* LEFT = "LEFT";
  static constexpr const char* UP = "UP";
  static constexpr const char* DOWN = "DOWN";
  static constexpr const char* NO_MOVE = "NO_MOVE";

};

#endif