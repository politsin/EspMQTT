#include <Arduino.h>
#include "EspMQTT.h"
#include "EspMQTT_App.h"
using std::string;

bool EspApp::app(string param, string message) {
  if (param == "$echo") {
    mqtt.publishState(this->messageState, message);
    return true;
  }
  if (param == "$setInterval") {
    int number = std::atoi(message.c_str());
    if (number > 0) {
      uint16_t sec = static_cast<uint16_t>(number);
      mqtt.setAvailabilityInterval(sec);
      return true;
    }
    return false;
  }
  if (param == "$update") {
    mqtt.publishState(this->errorState, this->notReady);
    return true;
  }
  if (param == "$pinRead") {
    mqtt.publishState(this->errorState, this->notReady);
    return true;
  }
  if (param == "$pinReadAnalog") {
    mqtt.publishState(this->errorState, this->notReady);
    return true;
  }
  if (param == "$pinSet") {
    mqtt.publishState(this->errorState, this->notReady);
    return true;
  }
  if (param == "$pinSetPwm") {
    mqtt.publishState(this->errorState, this->notReady);
    return true;
  }
  if (this->debug) {
    Serial.printf("MQTT-APP %s=%s\n", param.c_str(), message.c_str());
  }
  return false;
}
