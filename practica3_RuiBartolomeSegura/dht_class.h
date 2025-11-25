#ifndef DHT_CLASS_H
#define DHT_CLASS_H

#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <Arduino.h>

class DHTClass {
public:
  DHTClass(int _dht, uint8_t _dht_type);

  float get_temp();
  float get_hum();

private:
  DHT_Unified* dht;
};

#endif
