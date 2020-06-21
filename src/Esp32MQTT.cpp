#include <Arduino.h>
#include <WiFi.h>
#include "Esp32MQTT.h"
#include "EspMQTT_App.h"

using std::string;
// MQTT.
AsyncMqttClient mqttClient;
// Ticker.
TimerHandle_t mqttReconnectTimer;
TimerHandle_t wifiReconnectTimer;
TimerHandle_t mqttAvailabilityTimer;
// App.
EspApp eapp;

void Esp32MQTT::setWiFi(string ssid, string pass, string host) {
  strcpy(this->WiFiSsid, ssid.c_str());
  strcpy(this->WiFiPass, pass.c_str());
  strcpy(this->WiFiHost, host.c_str());
};

void Esp32MQTT::setMqtt(string server, string user, string pass) {
  char device[23];
  uint64_t chipid = ESP.getEfuseMac();
  uint16_t chip = (uint16_t)(chipid >> 32);
  snprintf(device, 23, "ESP32-%04X%08X", chip, (uint32_t)chipid);
  strcpy(this->mqttDevice, device);
  strcpy(this->mqttHost, server.c_str());
  strcpy(this->mqttUser, user.c_str());
  strcpy(this->mqttPass, pass.c_str());
};

void Esp32MQTT::setCommonTopics(string root, string name) {
  // Root: /home/[sensor/switch/light]/{name}
  string r = root + "/" + name;
  strcpy(this->mqttRootTopic, r.c_str());
  // Metrics.
  string mRoot = r + string("/metric/");
  strcpy(this->metricRoot, mRoot.c_str());
  // Info.
  string availability = r + string("/availability");
  string ip = availability + string("/$ip");
  strcpy(this->availabilityTopic, availability.c_str());
  strcpy(this->ipTopic, ip.c_str());
  // Commands & Data.
  string cmd = r + string("/cmd/*");
  string data = r + string("/data");
  string state = r + string("/state");
  string recovery = r + string("/recovery");
  this->cmdTopicLength = cmd.length() - 1;
  strcpy(this->cmdTopic, cmd.c_str());
  strcpy(this->dataTopic, data.c_str());
  strcpy(this->stateTopic, state.c_str());
  strcpy(this->recoveryTopic, recovery.c_str());
};

void Esp32MQTT::start(bool init) {
  this->initMqtt = init;
  if (this->initMqtt) {
    this->online = false;
    gpio_pad_select_gpio(LED);
    gpio_set_direction(LED, GPIO_MODE_OUTPUT);
    gpio_set_level(LED, LOW);
    if (this->debugLevel >= 2) {
      printf("MQTT Start\n");
    }
    setupTimers();
    mqttClient.onConnect(onMqttConnect);
    mqttClient.onDisconnect(onMqttDisconnect);
    mqttClient.onSubscribe(onMqttSubscribe);
    mqttClient.onUnsubscribe(onMqttUnsubscribe);
    mqttClient.onMessage(onMqttMessage);
    mqttClient.onPublish(onMqttPublish);
    mqttClient.setClientId(this->mqttDevice);
    mqttClient.setServer(this->mqttHost, this->mqttPort);
    mqttClient.setCredentials(this->mqttUser, this->mqttPass);
    mqttClient.setWill(availabilityTopic, 0, true, "offline");
    // WiFi.
    WiFi.mode(WIFI_MODE_STA);
    WiFi.onEvent(WiFiEvent);
    connectToWifi();
  }
}

void Esp32MQTT::setupTimers() {
  mqttReconnectTimer = xTimerCreate(
    "mqttTimer",
    pdMS_TO_TICKS(2000),
    pdFALSE,
    (void*)0,
    reinterpret_cast<TimerCallbackFunction_t>(connectToMqtt)
  );
  wifiReconnectTimer = xTimerCreate(
    "wifiTimer",
    pdMS_TO_TICKS(2000),
    pdFALSE,
    (void*)0,
    reinterpret_cast<TimerCallbackFunction_t>(connectToWifi)
  );
  
  mqttAvailabilityTimer = xTimerCreate(
    "availabilityTimer",
    pdMS_TO_TICKS(this->availabilityInterval),
    pdTRUE,
    (void*)0,
    reinterpret_cast<TimerCallbackFunction_t>(availabilityTime)
  );
}

void Esp32MQTT::messageLoop() {
  if (length >= 1024) {
    this->publishState("$error", "Message is too big");
    return;
  }
  string param = string(topic).substr(this->cmdTopicLength);
  string message = string(payload, length);
  if ((char)payload[0] != '{') {
    // JSON. Do Nothing.
  }
  if ((char)param.at(0) == '$') {
    eapp.app(param, message);
    return;
  }
  this->callbackFunction(param, message);
  if (this->debugLevel) {
    printf("MQTT [%s] %s=%s\n", topic, param.c_str(), message.c_str());
  }
}

void Esp32MQTT::connectToWifi() {
  if (mqtt.debugLevel >= 2) {
    printf("Connecting to Wi-Fi...\n");
  }
  WiFi.begin(mqtt.WiFiSsid, mqtt.WiFiPass);
}

void Esp32MQTT::WiFiEvent(WiFiEvent_t event) {
  if (event == SYSTEM_EVENT_STA_GOT_IP) {
    mqtt.onWifiConnect(event);
  }
  if (event == SYSTEM_EVENT_STA_DISCONNECTED) {
    mqtt.onWifiDisconnect(event);
  }
}

void Esp32MQTT::onWifiConnect(WiFiEvent_t event) {
  string ip = WiFi.localIP().toString().c_str();
  strcpy(mqtt.ip, ip.c_str());
  if (mqtt.debugLevel >= 2) {
    printf("Connected to Wi-Fi. IP: %s\n", mqtt.ip);
  }
  connectToMqtt();
}

void Esp32MQTT::onWifiDisconnect(WiFiEvent_t event) {
  mqtt.setOffline();
  xTimerStop(mqttAvailabilityTimer, 0);
  xTimerStop(mqttReconnectTimer, 0);
  xTimerStart(wifiReconnectTimer, 0);
  if (mqtt.debugLevel >= 1) {
    printf("Disconnected from Wi-Fi.\n");
  }
}

void Esp32MQTT::connectToMqtt() {
  if (mqtt.debugLevel >=2 ) {
    printf("Connecting to MQTT...\n");
  }
  mqttClient.connect();
}

void Esp32MQTT::onMqttConnect(bool sessionPresent) {
  mqtt.setOnline();
  gpio_set_level(LED, HIGH);
  xTimerStart(mqttAvailabilityTimer, 0);
  if (mqtt.debugLevel >= 2) {
    printf("Session present: %d\n", sessionPresent);
  }
}

void Esp32MQTT::setOnline() {
  this->online = true;
  this->publishAvailability();
  this->mqttSubscribe();
  WiFi.setHostname(this->WiFiHost);
  if (this->debugLevel >= 1) {
    printf("Connected to MQTT.\n");
    printf("Set HostName %s.\n", this->WiFiHost);
    this->mqttTests();
  }
}

void Esp32MQTT::setOffline() {
  this->online = false;
  gpio_set_level(LED, LOW);
}

void Esp32MQTT::mqttTests() {
  if (this->test) {
    string test = string(cmdTopic).substr(0, cmdTopicLength) + string("test");
    const char* topic = test.c_str();
    printf("--== Statr Tests == --\n");
    printf("%s\n", topic);
    uint16_t packetIdSub = mqttClient.subscribe(topic, 2);
    printf("T0:  Subscribing at QoS 2, packetId: %d\n", packetIdSub);
    mqttClient.publish(topic, 0, true, "test 1");
    printf("T1:  Publishing at QoS 0\n");
    uint16_t packetIdPub1 = mqttClient.publish(topic, 1, true, "test 2");
    printf("Т2:  Publishing at QoS 1, packetId: %d\n", packetIdPub1);
    uint16_t packetIdPub2 = mqttClient.publish(topic, 2, true, "test 3");
    printf("Т3:  Publishing at QoS 2, packetId: %d\n", packetIdPub2);
    printf("--== End Tests Inits == --\n");
  }
}

void Esp32MQTT::setAvailabilityInterval(uint16_t sec, bool onSetup) {
  this->availabilityInterval = (uint32_t) sec * 1000;
  if (onSetup) {
    xTimerStop(mqttAvailabilityTimer, 0);
    xTimerChangePeriod(
      mqttAvailabilityTimer, 
      pdMS_TO_TICKS(this->availabilityInterval),
      0
    );
  }
  if (this->online) {
    xTimerStart(mqttAvailabilityTimer, 0);
  }
  if (this->debugLevel >= 2) {
    printf("Availability Interval=%d ms\n", availabilityInterval);
  }
}

void Esp32MQTT::onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  mqtt.setOffline();
  if (WiFi.isConnected()) {
    xTimerStop(mqttAvailabilityTimer, 0);
    xTimerStart(mqttReconnectTimer, 0);
  }
  if (mqtt.debugLevel >= 1) {
    printf("Disconnected from MQTT.\n");
  }
}

void Esp32MQTT::onMqttSubscribe(uint16_t packetId, uint8_t qos) {
  if (mqtt.debugLevel >= 2) {
    printf("Subscribe: packetId=%d | QOS=%d\n", packetId, qos);
  }
}

void Esp32MQTT::onMqttUnsubscribe(uint16_t packetId) {
  if (mqtt.debugLevel >= 2) {
    printf("Unsubscribe: packetId=%d\n", packetId);
  }
}

void
Esp32MQTT::onMqttMessage(char* topic,
                         char* payload,
                         AsyncMqttClientMessageProperties prop,
                         size_t len,
                         size_t index,
                         size_t total)
{
  uint8_t pos = mqtt.cmdTopicLength;
  if ((char)topic[pos] == '$') {
    eapp.appInterrupt(topic, payload, len);
    return;
  }
  mqtt.messageFlag = true;
  mqtt.length = len;
  mqtt.topic = topic;
  mqtt.payload = payload;
  if (mqtt.test) {
    printf("Publish received.\n");
    printf("  topic: %s\n", topic);
    printf("  QoS=%d \t| dup=%d   \t| retain=%d\n", prop.qos, prop.dup, prop.retain);
    printf("  len=%d \t| index=%d \t| total=%d\n", len, index, total);
  }
}

void Esp32MQTT::onMqttPublish(uint16_t packetId) {
  if (false) {
    printf("Publish acknowledged.\n  packetId:%i", packetId);
  }
}

void Esp32MQTT::mqttSubscribe() {
  mqttClient.subscribe(this->cmdTopic, 2);
}

void Esp32MQTT::setCallback(std::function<void(string param, string value)> cBack) {
    this->callbackFunction = cBack;
};

void Esp32MQTT::setDebugLevel(uint8_t debugLevel) {
  this->debugLevel = debugLevel;
  if (this->debugLevel) {
    printf("debugLevel > ON %d\n", debugLevel);
  }
}

void Esp32MQTT::availabilityTime() {
  mqtt.publishAvailability();
}

void Esp32MQTT::publishAvailability() {
  mqttClient.publish(ipTopic, 0, true, this->ip);
  mqttClient.publish(availabilityTopic, 0, true, "online");
  if (this->debugLevel >= 1) {
    printf("MQTT [Publish Availability] at %s\n", ip);
  }
}

void Esp32MQTT::publishData(string data) {
  mqttClient.publish(dataTopic, 0, true, data.c_str());
}

void Esp32MQTT::publishState(string key, string value) {
  string topic = string(this->stateTopic) + "/" + key;
  mqttClient.publish(topic.c_str(), 1, true, value.c_str());
}

void Esp32MQTT::publishMetric(string key, string metric) {
  string topic = string(this->metricRoot) + key;
  mqttClient.publish(topic.c_str(), 0, true, metric.c_str());
}

void Esp32MQTT::publishMetric(char *key, uint16_t metric) {
  char message[16];
  itoa(metric, message, 10);
  char topic[255];
  strcpy(topic, this->metricRoot);
  strcat(topic, key);
  mqttClient.publish(topic, 0, true, message);
}

void Esp32MQTT::publishMetric(string key, uint16_t metric) {
  char message[16];
  itoa(metric, message, 10);
  string topic = string(this->metricRoot) + key;
  mqttClient.publish(topic.c_str(), 0, true, message);
}

void Esp32MQTT::publishMetric(string key, float metric) {
  if (metric > 0) {
    this->publishMetric(key, metric, false);
  }
}

void Esp32MQTT::publishMetric(string key, float metric, bool force) {
  if (metric > 0 || force) {
    char message[16];
    itoa(metric, message, 10);
    string topic = string(this->metricRoot) + key;
    mqttClient.publish(topic.c_str(), 0, true, message);
  }
}
