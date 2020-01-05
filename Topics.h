
#ifndef Mqtt_Topics_h
#define Mqtt_Topics_h

#include <Arduino.h>
using namespace std;

class Topics {
  public:
    Topics(char*);
    ~Topics(void);
    void addTopic(char* item);
    char** getTopics();
    int getSize();
  private:
    char** list;
    int size;
};

#endif /* !Mqtt_Topics_h */
