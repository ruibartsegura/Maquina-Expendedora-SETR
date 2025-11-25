#include "dht_class.h"

DHTClass::DHTClass(int _dht, uint8_t _dht_type) {
  dht = new DHT_Unified(_dht, _dht_type);  // crear objeto
  dht->begin();

}


float DHTClass::get_temp() {
  sensors_event_t event;
  dht->temperature().getEvent(&event);
  return event.temperature;
}

float DHTClass::get_hum() {
  sensors_event_t event;
  dht->humidity().getEvent(&event);
  return event.relative_humidity;
}