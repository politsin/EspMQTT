#ifndef Mqtt_App_h
#define Mqtt_App_h

#include <string>
using std::string;

class EspApp {
  public:
    bool debug = false;
    bool app(string param, string message);
    bool appInterrupt(char* topic, char* payload, size_t length);
  private:
    string messageState = string("$message");
    string errorState = string("$error");
    string notReady = "Not ready yet";
    bool compareStr(char a[],char b[]){
      for (int i = 0; a[i] != '\0'; i++) {
        if (a[i] != b[i]) {
          return 0;
        }
      }
      return 1;
    }
};
extern EspApp eapp;

#endif /* !Mqtt_h */
