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
};
extern EspApp eapp;

#endif /* !Mqtt_h */
