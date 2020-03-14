#ifndef Mqtt_App_h
#define Mqtt_App_h

#include <string>
using std::string;

class EspApp {
  public:
    bool debug = true;
    bool app(string param, string message);
  private:
    int16_t stringToInt(string message);
    string messageState = string("$message");
    string errorState = string("$error");
    const char *echoTopic = "$echo";
    const char *updateTopic = "$update";
    const char *setIntervalTopic = "$setInterval";
    const char *pinReadTopic = "$pinRead";
    const char *pinReadAnalogTopic = "$pinReadAnalog";
    const char *pinSetTopic = "$pinSet";
    const char *pinSetPwmTopic = "$pinSetPwm";
};
extern EspApp eapp;

#endif /* !Mqtt_h */
