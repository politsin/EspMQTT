#ifndef Mqtt_App_h
#define Mqtt_App_h

#include <string>
using std::string;

class EspApp {
  public:
    bool debug = true;
    bool app(string param, string message);
  private:
    string messageState = string("$message");
    string errorState = string("$error");
    string notReady = "Not ready yet";
};
extern EspApp eapp;

#endif /* !Mqtt_h */
