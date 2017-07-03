#include <Arduino.h>
#include "timer.h"
// https://www.baldengineer.com/arduino-how-do-you-reset-millis.html

Timer::Timer() {
  previousMillis = 0;
}

unsigned long Timer::getMillis() {
  previousMillis = millis();
  return previousMillis;
}

unsigned long Timer::millisSinceLast() {
  // Get snapshot of time
  unsigned long currentMillis = millis();
  // How much time has passed, accounting for rollover with subtraction!
  unsigned long diffMillis = (unsigned long)(currentMillis - previousMillis);
  previousMillis = currentMillis;

  return diffMillis;
}

bool Timer::hasAmountTimePassed(unsigned long time) {
  // How much time has passed, accounting for rollover with subtraction!
  return (unsigned long)(millis() - previousMillis) >= time;
}