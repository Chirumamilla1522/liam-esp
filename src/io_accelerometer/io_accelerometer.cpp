#include <Arduino.h>
#include <ArduinoLog.h>
#include <FunctionalInterrupt.h>
#include "definitions.h"
#include "io_accelerometer.h"
#include "utils.h"

// https://github.com/sparkfun/ESP32_Motion_Shield/tree/master/Software
// https://learn.sparkfun.com/tutorials/esp32-thing-motion-shield-hookup-guide/using-the-imu

IO_Accelerometer::IO_Accelerometer(TwoWire& w): _Wire(w) {

  // the device's communication mode and addresses:
  imu.settings.device.commInterface = IMU_MODE_I2C;
  //imu.settings.device.mAddress = LSM9DS1_M;
  //imu.settings.device.agAddress = LSM9DS1_AG;
}

void IO_Accelerometer::start() {

  if (!imu.begin()) {
    Log.error(F("Failed to initialize gyro/accelerometer/compass, check connections!"));
  } else {
    Log.notice(F("Gyro/accelerometer/compass init success." CR));
    available = true;

    imu.calibrate(true);
    //imu.calibrateMag(true);   //TODO: check why this crashes with: Guru Meditation Error: Core  1 panic'ed (StoreProhibited). Exception was unhandled.

    // Set initial state.
    for (auto i = 0; i < GYRO_MEDIAN_SAMPLES; i++) {
      getReadings();
    }

    sensorReadingTicker.attach_ms<IO_Accelerometer*>(50, [](IO_Accelerometer* instance) {
      instance->getReadings();
    }, this);
  }
}

bool IO_Accelerometer::isAvailable() const {
  return available;
}

const orientation& IO_Accelerometer::getOrientation() const {
  return currentOrientation;
}

bool IO_Accelerometer::isFlipped() const {
  if (available == false) {
    return false;
  } else {    
    return (abs(currentOrientation.pitch) > Definitions::TILT_ANGLE_MAX || abs(currentOrientation.roll) > Definitions::TILT_ANGLE_MAX);
  }
}

void IO_Accelerometer::getReadings() {
  
  if (available) {    
    // Update the sensor values whenever new data is available
    if ( imu.gyroAvailable() ) {
      // To read from the gyroscope,  first call the
      // readGyro() function. When it exits, it'll update the
      // gx, gy, and gz variables with the most current data.
      imu.readGyro();
    }
    if ( imu.accelAvailable() ) {
      // To read from the accelerometer, first call the
      // readAccel() function. When it exits, it'll update the
      // ax, ay, and az variables with the most current data.
      imu.readAccel();
    }
    if ( imu.magAvailable() ) {
      // To read from the magnetometer, first call the
      // readMag() function. When it exits, it'll update the
      // mx, my, and mz variables with the most current data.
      imu.readMag();
    }

    ax_sample[medianIndex] = imu.ax;
    ay_sample[medianIndex] = imu.ay;
    az_sample[medianIndex] = imu.az;
    mx_sample[medianIndex] = imu.mx;
    my_sample[medianIndex] = imu.my;

    medianIndex = (medianIndex + 1) % GYRO_MEDIAN_SAMPLES;

    auto newAx = Utils::calculateMedian<int16_t>(ax_sample);
    auto newAy = Utils::calculateMedian<int16_t>(ay_sample);
    auto newAz = Utils::calculateMedian<int16_t>(az_sample);
    auto newMx = Utils::calculateMedian<int16_t>(mx_sample);
    auto newMy = Utils::calculateMedian<int16_t>(my_sample);

    float roll = atan2(newAy, newAz);
    float pitch = atan2(-newAx, sqrt(newAy * newAy + newAz * newAz));
    float yaw;

    if (-newMy == 0) {
      yaw = (-newMx < 0) ? PI : 0;
    } else {
      yaw = atan2(-newMx, -newMy);
    }
  
    // Convert everything from radians to degrees:
    yaw *= 180.0f / PI;
    pitch *= 180.0f / PI;
    roll  *= 180.0f / PI;
  
    yaw -= DECLINATION;

    if (yaw < 0) {
      yaw += 360.0f; // Ensure yaw stays between 0 and 360
    }

    //Log.notice("Pitch: %s, Roll: %s, Heading: %s" CR, String(pitch).c_str(), String(roll).c_str(), String(yaw).c_str());

    currentOrientation.roll = roundf(roll);
    currentOrientation.pitch = roundf(pitch);
    currentOrientation.heading = roundf(yaw);
  }
}

