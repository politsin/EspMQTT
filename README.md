# ESP8266 > MQTT
ESP8266 foundation library for MQTT based HomeIOT

```cpp
#include <Arduino.h>
#include "EspMQTT.h"

EspMQTT mqtt;
void mqtt_callback(std::string param, std::string value);

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  Serial.begin(115200);
  Serial.println("\nSetup");
  // MqTT:
  mqtt.setWiFi("MY_ssid", "MY_pass", "MY_hostname");
  mqtt.setMqtt("MY_host", "MY_user", "MY_pass");
  mqtt.setCommonTopics("home/light", "roomled");  // (topicRoot, device)
  mqtt.setCallback(mqtt_callback);
  // mqtt.ota = true;                             // Default: false;
  // mqtt.test = true;                            // Default: false;
  // mqtt.debug = true;                           // Default: false;
  // mqtt.setAvailabilityInterval(sec)            // Default: 30sec;
  mqtt.start();
}

void loop() {
  mqtt.loop(); // otaHandle;
  if (WiFi.isConnected() && mqtt.online) {
    // Do someshing.
  }
  yield();
}

void mqtt_callback(std::string param, std::string value) {
  uint16_t val = atoi(value.c_str());
  Serial.printf("%s=%s\n", param.c_str(), message.c_str());
}
```

## Ref:
 * https://github.com/marvinroger/async-mqtt-client/blob/master/src/AsyncMqttClient.hpp
 * https://github.com/Dullage/ESP-LED-MQTT
