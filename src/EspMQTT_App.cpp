#include <Arduino.h>
#include "EspMQTT.h"
#include "EspMQTT_App.h"
using std::string;

bool EspApp::app(string param, string message) {
  if (param.compare("$echo") == 0) {
    mqtt.publishState("$message", message);
    return true;
  }
  else if (param.compare("$update") == 0) {
    mqtt.publishState("$error", "Not ready yet");
    return true;
  }
  else if (param.compare("$setInterval") == 0) {
    int16_t interval = this->stringToInt(message);
    if (interval > 0) {
      mqtt.setAvailabilityInterval(interval);
      return true;
    }
    return false;
  }
  else if (param.compare("$pinRead") == 0) {
    mqtt.publishState("$error", "Not ready yet");
    return true;
  }
  else if (param.compare("$pinReadAnalog") == 0) {
    mqtt.publishState("$error", "Not ready yet");
    return true;
  }
  else if (param.compare("$pinSet") == 0) {
    mqtt.publishState("$error", "Not ready yet");
    return true;
  }
  else if (param.compare("$pinSetPwm") == 0) {
    mqtt.publishState("$error", "Not ready yet");
    return true;
  }
  if (this->debug) {
    Serial.printf("MQTT-APP %s=%s\n", param.c_str(), message.c_str());
  }
  return false;
}

int16_t EspApp::stringToInt(string message) {
  const char* number = message.c_str();
  int size = message.length();
  char stackbuf[size+1];
  memcpy(stackbuf, number, size);
  stackbuf[size] = '\0';
  int num = atoi(stackbuf);
  int16_t interval = static_cast<uint16_t>(num);
  return interval;
}
