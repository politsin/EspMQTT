#ifndef Mqtt_h
#define Mqtt_h

#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>
#include <WiFiUdp.h>
#include <ESP8266mDNS.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <string>
#include "Topics.h"
#include <functional>

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
    String mqttRootTopic;
    String metricRoot;
    // Info.
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
    void setWiFi(String ssid, String pass, String host);
    void setMqtt(String server, String user, String pass);
    void setCommonTopics(String root, String name);
    // Mqtt setters.
    void setStringValue(char* name, char* value);
    void addSubsribeTopic(String topic);
    // Loop.
    void loop();
    void setAvailabilityPeriod(uint16_t debug);
    // Send.
    void publishData(String data);
    void publishState(String key, String value);
    void publishMetric(String key, float metric);
    void publishMetric(String key, float metric, bool force);
    // Callbacks.
    void callback(char *topic, byte *payload, int length);
    static void callbackStatic(char *topic, byte *payload, int length);
    void setCallback(std::function<void(String param, String value)> cBack);
    // Timers.
    void setReconnectInterval(int sec);
    void setAvailabilityInterval(int sec);

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
    void reconnect();
    void reconnectInit();
    void reconnectWatchDog();
    void reconnectWiFi();
    void reconnectMqtt();
    void reconnectSubscribe();
    void subsribe();
    void callbackDebug(char *topic, byte *dat, int len, String param);
    void callbackParceJson(String message);
    String callbackGetMessage(byte *dat, int len);
    std::function<void(String param, String value)> callbackFunction;
    void publishRecovery();
    void publishAvailability();
};
extern EspMQTT mqtt;

#endif /* !Mqtt_h */
