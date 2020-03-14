#include <Arduino.h>
#include "EspMQTT.h"
#include "EspMQTT_App.h"
using std::string;

bool EspApp::app(string param, string message) {
  if (param.compare(this->echoTopic) == 0) {
    mqtt.publishState(this->messageState, message);
    return true;
  }
  else if (param.compare(this->updateTopic) == 0) {
    mqtt.publishState(this->errorState, "Not ready yet");
    return true;
  }
  else if (param.compare(this->setIntervalTopic) == 0) {
    int16_t interval = this->stringToInt(message);
    if (interval > 0) {
      mqtt.setAvailabilityInterval(interval);
      return true;
    }
    return false;
  }
  else if (param.compare(this->pinReadTopic) == 0) {
    mqtt.publishState(this->errorState, "Not ready yet");
    return true;
  }
  else if (param.compare(this->pinReadAnalogTopic) == 0) {
    mqtt.publishState(this->errorState, "Not ready yet");
    return true;
  }
  else if (param.compare(this->pinSetTopic) == 0) {
    mqtt.publishState(this->errorState, "Not ready yet");
    return true;
  }
  else if (param.compare(this->pinSetPwmTopic) == 0) {
    mqtt.publishState(this->errorState, "Not ready yet");
    return true;
  }
  if (this->debug) {
    Serial.printf("MQTT-APP %s=%s\n", param.c_str(), message.c_str());
  }
  return false;
}

int16_t EspApp::stringToInt(string message) {
  int16_t interval(0);
  int size = message.length();
  if (size < 1024) {
  /* TODO:
    const char* number = message.c_str();
    char stackbuf[size+1];
    memcpy(stackbuf, number, size);
    stackbuf[size] = '\0';
    int num = atoi(stackbuf);
    interval = static_cast<uint16_t>(num);
  */
  }
  return interval;
}
