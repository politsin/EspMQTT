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

bool EspApp::appInterrupt(char* topic, char* payload, size_t length) {
  if (length >= 1024) {
    return false;
  }
  char param[255];
  memcpy(param, &topic[mqtt.cmdTopicLength], strlen(topic));
  if (this->compareStr(param, (char*) "$setInterval")) {
    int number = std::atoi(payload);
    if (number > 0) {
      uint16_t sec = static_cast<uint16_t>(number);
      mqtt.setAvailabilityInterval(sec);
      return true;
    }
  }
  if (this->compareStr(param, (char*) "$debugInterrupt")) {
    string message = string(payload, length);
    Serial.printf("MQTT-Interrupt %s=%s\n", param, message.c_str());
    mqtt.publishState(this->messageState, message);
    return true;
  }
  return false;
}
