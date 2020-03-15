#ifndef Mqtt_h
#define Mqtt_h

#include <AsyncMqttClient.h>
#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <functional>
#include <string>
#include "EspMQTT_App.h"
using std::string;

class EspMQTT {
  public:
    bool ota = false;
    bool test = false;
    bool online = false;
    int8_t debugLevel = 0;
    // Flag.
    bool onlineFlag = false;
    bool messageFlag = false;
    bool availabilityFlag = false;
    // Set.
    char WiFiSsid[255];
    char WiFiPass[255];
    char WiFiHost[255];
    // Config.
    char mqttServer[255];
    char mqttUser[255];
    char mqttPass[255];
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

    void start();
    void start(bool init);
    void otaBegin();
    void setDebugLevel(uint8_t debugLevel);
    void setOta(bool ota);
    // WiFi setters.
    void setWiFi(string ssid, string pass, string host);
    void setMqtt(string server, string user, string pass);
    void setCommonTopics(string root, string name);
    // Mqtt setters.
    void setstringValue(char* name, char* value);
    void addSubsribeTopic(string topic);
    // Loop.
    void loop();
    void setAvailabilityInterval(uint16_t sec);

    // Send.
    void publishData(string data);
    void publishState(string key, string value);
    void publishMetric(char *key, uint16_t metric);
    void publishMetric(string key, uint16_t metric);
    void publishMetric(string key, float metric);
    void publishMetric(string key, float metric, bool force);
    // Callbacks.
    // void callback(char *topic, char *payload, uint16_t length);
    void setCallback(std::function<void(string param, string value)> cBack);
    // Timers.

    // Online.
    void setOnline();
    void setOffline();
    void mqttSubscribe();
    void publishAvailability();

    // Async.
    void onMqttConnect();
    void onMqttConnectTests();
    static void connectToWifi();
    static void onWifiConnect(const WiFiEventStationModeGotIP& event);
    static void onWifiDisconnect(const WiFiEventStationModeDisconnected& event);
    static void connectToMqtt();
    static void publishAvailabilityStatic();
    static void onMqttConnectStatic(bool sessionPresent);
    static void onMqttConnectStaticDemo(bool sessionPresent);
    static void onMqttDisconnect(AsyncMqttClientDisconnectReason reason);
    static void onMqttSubscribe(uint16_t packetId, uint8_t qos);
    static void onMqttUnsubscribe(uint16_t packetId);
    static void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total);
    static void onMqttPublish(uint16_t packetId);

  private:
    bool initMqtt = true;
    uint32_t availabilityInterval = 30000;
    void setup();
    void setupOta();
    void subsribe();
    void callbackParceJson(string message);
    std::function<void(string param, string value)> callbackFunction;
};
extern EspMQTT mqtt;

#endif /* !Mqtt_h */
