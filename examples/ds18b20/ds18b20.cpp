#include <Arduino.h>
using std::string;

// Queue.
#include <freertos/queue.h>
QueueHandle_t mqttQueue;
typedef struct {
  string name;
  string metric;
} mqttMessage;

// Ds18b20.
#include <DallasTemperature.h>
#include <OneWire.h>

#define ONE_WIRE_BUS1 GPIO_NUM_18
OneWire oneWire1(ONE_WIRE_BUS1);
DallasTemperature ds18(&oneWire1);

TaskHandle_t ds18b20;
void ds18b20Task(void *pvParam) {
  ds18.begin();
  while (true) {
    float temperature = ds18.getTempCByIndex(0);
    if (temperature != DEVICE_DISCONNECTED_C) {
        char data[10];
        sprintf(data, "%.2f", temperature);
        string metric = std::string(data);
        mqttMessage msg = {"temperature", metric};
        xQueueSend(mqttQueue, &msg, portMAX_DELAY);
        // printf("Tin = %.2f C\n", temperature);
    } // else: printf("Error: Could not read temperature data\n");
    vTaskDelay(10356 / portTICK_PERIOD_MS);
  }
}

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
  // mqtt.setCallback(mqtt_callback);
  mqtt.start(true);
}


// Main.cpp Setup.
void setup() {
  mqttSetup();
  mqttQueue = xQueueCreate(10, sizeof(mqttMessage));
  xTaskCreate(ds18b20Task, "ds18b20", 1024, NULL, 1, &ds18b20);
}

// Main.cpp Loop.
mqttMessage message;
QueueHandle_t mqttQueue;
void loop() {
  if (xQueueReceive(mqttQueue, &message, 100 / portTICK_PERIOD_MS) == pdTRUE) {
    mqtt.publishMetric(message.name, message.metric);
    // printf("mqtt [%s] push = %s\n", msg.name.c_str(), msg.metric.c_str());
  }
  // vTaskDelay(10 / portTICK_PERIOD_MS);
}
