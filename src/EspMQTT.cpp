#include <Arduino.h>
#include "EspMQTT.h"
#include "EspMQTT_App.h"

using std::string;
// MQTT.
AsyncMqttClient mqttClient;
// WiFi.
WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;
// Ticker.
Ticker mqttReconnectTimer;
Ticker wifiReconnectTimer;
Ticker mqttAvailabilityTimer;
// App.
EspApp eapp;

void EspMQTT::setWiFi(string ssid, string pass, string host) {
  strcpy(this->WiFiSsid, ssid.c_str());
  strcpy(this->WiFiPass, pass.c_str());
  strcpy(this->WiFiHost, host.c_str());
};

void EspMQTT::setMqtt(string server, string user, string pass) {
  strcpy(this->mqttServer, server.c_str());
  strcpy(this->mqttUser, user.c_str());
  strcpy(this->mqttPass, pass.c_str());
};

void EspMQTT::setCommonTopics(string root, string name) {
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

void EspMQTT::start() {
  this->online = false;
  digitalWrite(LED_BUILTIN, LOW);
  if (this->debugLevel >= 2) {
    Serial.println("MQTT Start");
  }
  WiFi.mode(WIFI_STA);
  WiFi.hostname(this->WiFiHost);

  wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
  wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);

  mqttClient.onConnect(onMqttConnectStatic);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onSubscribe(onMqttSubscribe);
  mqttClient.onUnsubscribe(onMqttUnsubscribe);
  mqttClient.onMessage(onMqttMessage);
  mqttClient.onPublish(onMqttPublish);
  mqttClient.setServer(this->mqttServer, this->mqttPort);
  mqttClient.setCredentials(this->mqttUser, this->mqttPass);
  mqttClient.setWill(availabilityTopic, 0, true, "offline");
  // string clientId = "ESP8266-";
  // clientId += ESP.getChipId();
  // clientId += "-";
  // clientId += string(random(0xffff), HEX);
  // mqttClient.setClientId(clientId.c_str());
  mqtt.connectToWifi();
}

void EspMQTT::start(bool init) {
  this->initMqtt = init;
  if (this->initMqtt) {
    this->start();
  }
}

void EspMQTT::loop() {
  if (this->online) {
    if (this->availabilityFlag) {
      this->availabilityFlag = false;
      this->publishAvailability();
    }
    if (this->onlineFlag) {
      this->onlineFlag = false;
      this->onMqttConnect();
      this->onMqttConnectTests();
    }
    if (this->messageFlag) {
      this->messageFlag = false;
      this->messageLoop();
    }
    if (this->ota) {
      // ArduinoOTA.handle();
    }
  }
}

void EspMQTT::messageLoop() {
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
    Serial.printf("MQTT [%s] %s=%s\n", topic, param.c_str(), message.c_str());
  }
}

void EspMQTT::connectToWifi() {
  if (mqtt.debugLevel >= 2) {
    Serial.println("Connecting to Wi-Fi...");
  }
  WiFi.begin(mqtt.WiFiSsid, mqtt.WiFiPass);
}

void EspMQTT::onWifiConnect(const WiFiEventStationModeGotIP& event) {
  string ip = WiFi.localIP().toString().c_str();
  strcpy(mqtt.ip, ip.c_str());
  if (mqtt.debugLevel >= 2) {
    Serial.printf("Connected to Wi-Fi. IP: %s\n", mqtt.ip);
  }
  connectToMqtt();
}

void EspMQTT::onWifiDisconnect(const WiFiEventStationModeDisconnected& event) {
  mqtt.setOffline();
  if (mqtt.debugLevel >= 1) {
    Serial.println("Disconnected from Wi-Fi.");
  }
  // Ensure we don't reconnect to MQTT while reconnecting to Wi-Fi.
  mqttAvailabilityTimer.detach();
  mqttReconnectTimer.detach();
  wifiReconnectTimer.once(2, connectToWifi);
}

void EspMQTT::connectToMqtt() {
  if (mqtt.debugLevel >=2 ) {
    Serial.println("Connecting to MQTT...");
  }
  mqttClient.connect();
}

void EspMQTT::onMqttConnectStatic(bool sessionPresent) {
  mqtt.setOnline();
  mqtt.availabilityFlag = true;
  mqttAvailabilityTimer.attach_ms(mqtt.availabilityInterval, publishAvailabilityStatic);
  if (mqtt.debugLevel >= 2) {
    Serial.printf("Session present: %d\n", sessionPresent);
  }
}

void EspMQTT::setOnline() {
  this->online = true;
  this->onlineFlag = true;
  digitalWrite(LED_BUILTIN, HIGH);
}

void EspMQTT::onMqttConnect() {
  this->publishAvailability();
  this->mqttSubscribe();
  if (this->debugLevel >= 1) {
    Serial.println("Connected to MQTT.");
  }
}

void EspMQTT::onMqttConnectTests() {
  if (this->test) {
    string test = string(cmdTopic).substr(0, cmdTopicLength) + string("test");
    const char* topic = test.c_str();
    Serial.println("--== Statr Tests == --");
    Serial.println(topic);
    uint16_t packetIdSub = mqttClient.subscribe(topic, 2);
    Serial.printf("T0:  Subscribing at QoS 2, packetId: %d\n", packetIdSub);
    mqttClient.publish(topic, 0, true, "test 1");
    Serial.println("T1:  Publishing at QoS 0");
    uint16_t packetIdPub1 = mqttClient.publish(topic, 1, true, "test 2");
    Serial.printf("Т2:  Publishing at QoS 1, packetId: %d\n", packetIdPub1);
    uint16_t packetIdPub2 = mqttClient.publish(topic, 2, true, "test 3");
    Serial.printf("Т3:  Publishing at QoS 2, packetId: %d\n", packetIdPub2);
    Serial.println("--== End Tests Inits == --");
  }
}

void EspMQTT::setAvailabilityInterval(uint16_t sec) {
  this->availabilityInterval = (uint32_t) sec * 1000;
  mqttAvailabilityTimer.detach();
  if (this->online) {
    mqttAvailabilityTimer.attach_ms(sec * 1000, publishAvailabilityStatic);
  }
  if (this->debugLevel >= 2) {
    Serial.printf("Availability Interval=%d ms\n", availabilityInterval);
  }
}

void EspMQTT::onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  mqtt.setOffline();
  if (mqtt.debugLevel >= 1) {
    Serial.println("Disconnected from MQTT.");
  }
  if (WiFi.isConnected()) {
    mqttReconnectTimer.once(2, connectToMqtt);
  }
}

void EspMQTT::onMqttSubscribe(uint16_t packetId, uint8_t qos) {
  if (mqtt.debugLevel >= 2) {
    Serial.printf("Subscribe: packetId=%d | QOS=%d\n", packetId, qos);
  }
}

void EspMQTT::onMqttUnsubscribe(uint16_t packetId) {
  if (mqtt.debugLevel >= 2) {
    Serial.printf("Unsubscribe: packetId=%d\n", packetId);
  }
}

void EspMQTT::onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
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
    Serial.println("Publish received.");
    Serial.printf("  topic: %s\n", topic);
    Serial.printf("  QoS=%d \t| dup=%d   \t| retain=%d\n", properties.qos, properties.dup, properties.retain);
    Serial.printf("  len=%d \t| index=%d \t| total=%d\n", len, index, total);
  }
}

void EspMQTT::onMqttPublish(uint16_t packetId) {
  if (false) {
    Serial.println("Publish acknowledged.");
    Serial.print("  packetId: ");
    Serial.println(packetId);
  }
}

void EspMQTT::mqttSubscribe() {
  mqttClient.subscribe(this->cmdTopic, 2);
}

void EspMQTT::setOffline() {
  this->online = false;
  digitalWrite(LED_BUILTIN, LOW);
}

void EspMQTT::setCallback(std::function<void(string param, string value)> cBack) {
    this->callbackFunction = cBack;
};

void EspMQTT::setDebugLevel(uint8_t debugLevel) {
  this->debugLevel = debugLevel;
  if (this->debugLevel) {
    Serial.printf("debugLevel > ON %d\n", debugLevel);
  }
}

void EspMQTT::publishAvailabilityStatic() {
  mqtt.availabilityFlag = true;
}

void EspMQTT::publishAvailability() {
  mqttClient.publish(ipTopic, 0, true, this->ip);
  mqttClient.publish(availabilityTopic, 0, true, "online");
  if (this->debugLevel >= 1) {
    Serial.printf("MQTT [Publish Availability] at %s\n", ip);
  }
}

void EspMQTT::publishData(string data) {
  mqttClient.publish(dataTopic, 0, true, data.c_str());
}

void EspMQTT::publishState(string key, string value) {
  string topic = string(this->stateTopic) + "/" + key;
  mqttClient.publish(topic.c_str(), 1, true, value.c_str());
}

void EspMQTT::publishMetric(char *key, uint16_t metric) {
  char message[16];
  itoa(metric, message, 10);
  char topic[255];
  strcpy(topic, this->metricRoot);
  strcat(topic, key);
  mqttClient.publish(topic, 0, true, message);
}

void EspMQTT::publishMetric(string key, uint16_t metric) {
  char message[16];
  itoa(metric, message, 10);
  string topic = string(this->metricRoot) + key;
  mqttClient.publish(topic.c_str(), 0, true, message);
}

void EspMQTT::publishMetric(string key, float metric) {
  if (metric > 0) {
    this->publishMetric(key, metric, false);
  }
}

void EspMQTT::publishMetric(string key, float metric, bool force) {
  if (metric > 0 || force) {
    char message[16];
    itoa(metric, message, 10);
    string topic = string(this->metricRoot) + key;
    mqttClient.publish(topic.c_str(), 0, true, message);
  }
}
