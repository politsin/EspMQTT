# ESP8266 > MQTT
ESP8266 foundation library for MQTT based HomeIOT

```cpp
#include <Arduino.h>
#include "EspMQTT.h"

void mqtt_callback(String param, String value);
void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  Serial.begin(115200);
  Serial.println("\nSetup");
  // MqTT:
  mqtt.setWiFi("MY_ssid", "MY_pass", "MY_hostname");
  mqtt.setMqtt("MY_host", "MY_user", "MY_pass");
  mqtt.setCommonTopics("home/light", "roomled"); // (topicRoot, device)
  mqtt.setCallback(mqtt_callback);
  mqtt.start();
}

void loop() {
  mqtt.loop();
  if (mqtt.online) {
    // Do something if online.
  }
  yield();
}

void mqtt_callback(String param, String value) {
  Serial.println(param + ": " + value);
}
```
