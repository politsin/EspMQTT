#ifndef Mqtt_h
#define Mqtt_h
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include <AsyncMqttClient.h>
#include <WiFi.h>
#include <functional>
#include <stdio.h>
#include <string>
using std::string;

// TODO:
#define LED GPIO_NUM_22

class EspMQTT {
public:
  bool test = false;
  bool online = false;
  int8_t debugLevel = 0;
  // Flag (TODO).
  bool messageFlag = false;
  // Set.
  char WiFiSsid[255];
  char WiFiPass[255];
  char WiFiHost[255];
  // Config.
  char mqttHost[255];
  char mqttUser[255];
  char mqttPass[255];
  char mqttDevice[255];
  const uint16_t mqttPort = 1883;

  char ip[255];
  // Topics.
  char mqttRootTopic[255];
  char metricRoot[255];
  // Info.
  char availabilityTopic[255];
  char ipTopic[255];
  char testTopic[255];
  // Cmd & Data
  char cmdTopic[255];
  uint16_t cmdTopicLength = 0;
  char stateTopic[255];
  char recoveryTopic[255];
  char dataTopic[255];
  // Message.
  size_t length;
  char *topic;
  char *payload;
  void messageLoop();

  void start(bool init = true);
  void setupTimers();
  void setDebugLevel(uint8_t debugLevel);
  void setOta(bool ota);
  // WiFi setters.
  void setWiFi(string ssid, string pass, string host);
  void setMqtt(string server, string user, string pass);
  void setCommonTopics(string root, string name);
  // Mqtt setters.
  void setstringValue(char *name, char *value);
  void addSubsribeTopic(string topic);
  // Loop.
  void setAvailabilityInterval(uint16_t se, bool onSetup = false);

  // Send.
  void publishData(string data);
  void publishState(string key, string value);
  void publishMetric(char *key, uint16_t metric);
  void publishMetric(string key, uint16_t metric);
  void publishMetric(string key, string metric);
  void publishMetric(string key, float metric);
  void publishMetric(string key, float metric, bool force);
  // Callbacks.
  // void callback(char *topic, char *payload, uint16_t length);
  void setCallback(std::function<void(string param, string value)> cBack);
  // Timers.
  // MqttClient.
  void mqttClientSetup(bool proxy = true);
  void mqttClientProxyConnect();
  uint16_t subscribe(const char *topic, uint8_t qos);
  uint16_t publish(const char *topic, uint8_t qos, bool retain,
                   const char *payload = nullptr, size_t length = 0,
                   bool dup = false, uint16_t message_id = 0);

  // Online.
  void mqttTests();
  void setOnline();
  void setOffline();
  void mqttSubscribe();
  void publishAvailability();

  // Async.
  static void connectToWifi();
  static void onWifiConnect(WiFiEvent_t even);
  static void onWifiDisconnect(WiFiEvent_t even);
  static void WiFiEvent(WiFiEvent_t event);
  static void connectToMqtt();
  static void availabilityTime();
  static void onMqttConnect(bool sessionPresent);
  static void onMqttDisconnect(AsyncMqttClientDisconnectReason reason);
  static void onMqttSubscribe(uint16_t packetId, uint8_t qos);
  static void onMqttUnsubscribe(uint16_t packetId);
  static void onMqttMessage(char *topic, char *payload,
                            AsyncMqttClientMessageProperties prop, size_t len,
                            size_t index, size_t total);
  static void onMqttPublish(uint16_t packetId);

private:
  bool initMqtt = true;
  uint32_t availabilityInterval = 30000;
  void setup();
  void callbackParceJson(string message);
  std::function<void(string param, string value)> callbackFunction;
};
extern EspMQTT mqtt;

#endif /* !Mqtt_h */
