# ESP32 > MQTT
ESP32 foundation library for MQTT based HomeIOT

```cpp
#include <Arduino.h>
using std::string;

#include <freertos/queue.h>
QueueHandle_t mqttQueue;
typedef struct {
  string name;
  string metric;
} mqttMessage;

// EspMqtt.
#include "EspMQTT.h"
EspMQTT mqtt;
void mqtt_callback(std::string param, std::string value);

void mqttSetup() {
  uint16_t debugLevel = 0;
  if (debugLevel) {
    mqtt.debugLevel = debugLevel;
    mqtt.setAvailabilityInterval(5);
  }
  // MqTT:
  mqtt.setWiFi("wifi-name", "wifi-pass", "hostname");
  mqtt.setMqtt("mqtt-server", "mqtt-user", "mqtt-pass");
  mqtt.setCommonTopics("my/root/topic/dir", "ds18b20");
  mqtt.setCallback(mqtt_callback);
  mqtt.start(true);
}


// Main.cpp Setup.
void setup() {
  mqttSetup();
  mqttQueue = xQueueCreate(10, sizeof(mqttMessage));
}

// Main.cpp Loop.
float counter = 1.1;
void loop() {
  if (xQueueReceive(mqttQueue, &message, 100 / portTICK_PERIOD_MS) == pdFALSE) {
    mqtt.publishMetric(message.name, message.metric);
    // printf("mqtt [%s] push = %s\n", msg.name.c_str(), msg.metric.c_str());
  }
  else {
    counter += 0.1;
    char data[10];
    sprintf(data, "%.2f", counter);
    string metric = std::string(data);
    mqttMessage msg = {"temperature", metric};
    xQueueSend(mqttQueue, &msg, portMAX_DELAY);
  }
  vTaskDelay(5000 / portTICK_PERIOD_MS);
}


void mqtt_callback(std::string param, std::string value) {
  uint16_t val = atoi(value.c_str());
  printf("%s=%s\n", param.c_str(), message.c_str());
}
```

## Ref:
 * https://github.com/marvinroger/async-mqtt-client/blob/master/src/AsyncMqttClient.hpp
 * https://github.com/Dullage/ESP-LED-MQTT
