#ifndef Mqtt_h
#define Mqtt_h

#include <Arduino.h>

#include "Topics.h"
#include <AsyncMqttClient.h>
#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <functional>
#include <string>
using std::string;

class EspMQTT {
  public:
    bool ota = true;
    bool debug = false;
    bool online = false;
    char* WiFiSsid;
    char* WiFiPass;
    char* WiFiHost;
    // Config.
    char* mqttServer;
    char* mqttUser;
    char* mqttPass;
    int   mqttPort = 1883;

    // Topics.
    Topics *topics;
    string mqttRootTopic;
    char* metricRoot;
    // Info.
    char* ip;
    char* availabilityTopic;
    char* ipTopic;
    // Cmd & Data
    char* cmdTopic;
    char* stateTopic;
    char* recoveryTopic;
    char* dataTopic;

    void start();
    void start(bool init);
    void otaBegin();
    void setDebug(bool debug);
    void setOta(bool debug);
    // WiFi setters.
    void setWiFi(string ssid, string pass, string host);
    void setMqtt(string server, string user, string pass);
    void setCommonTopics(string root, string name);
    // Mqtt setters.
    void setstringValue(char* name, char* value);
    void addSubsribeTopic(string topic);
    // Loop.
    void loop();
    void setAvailabilityPeriod(uint16_t debug);
    // Send.
    void publishData(string data);
    void publishState(string key, string value);
    void publishMetric(char *key, int metric);
    void publishMetric(string key, int metric);
    void publishMetric(string key, float metric);
    void publishMetric(string key, float metric, bool force);
    // Callbacks.
    void callback(char *topic, char *payload, int length);
    void setCallback(std::function<void(string param, string value)> cBack);
    // Timers.
    void setReconnectInterval(int sec);
    void setAvailabilityInterval(int sec);

    // Online.
    void setOnline();
    void setOffline();
    void mqttSubscribe();

    // Async.
    void connectToWifi();
    void onMqttConnect();
    static void connectToWifiStatic();
    static void onWifiConnect(const WiFiEventStationModeGotIP& event);
    static void onWifiDisconnect(const WiFiEventStationModeDisconnected& event);
    static void connectToMqtt();
    static void onMqttConnectStatic(bool sessionPresent);
    static void onMqttConnectStaticDemo(bool sessionPresent);
    static void onMqttDisconnect(AsyncMqttClientDisconnectReason reason);
    static void onMqttSubscribe(uint16_t packetId, uint8_t qos);
    static void onMqttUnsubscribe(uint16_t packetId);
    static void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total);
    static void onMqttPublish(uint16_t packetId);

  private:
    bool initMqtt = true;
    int  reconnectStep = 0;
    unsigned long reconnectStart = 0;
    unsigned long reconnectTimer = 0;
    unsigned long reconnectInterval = 1000;
    unsigned long availabilityTimer = 0;
    unsigned long availabilityInterval = 30000;
    void setup();
    void setup_ota();
    void reconnectInit();
    void reconnectWatchDog();
    void reconnectWiFi();
    void reconnectMqtt();
    void reconnectSubscribe();
    void subsribe();
    void callbackParceJson(string message);
    std::function<void(string param, string value)> callbackFunction;
    void publishAvailability();
};
extern EspMQTT mqtt;

#endif /* !Mqtt_h */
