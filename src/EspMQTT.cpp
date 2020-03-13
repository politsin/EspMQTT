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
  if (this->debug) {
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
  this->connectToWifi();
}

void EspMQTT::start(bool init) {
  this->initMqtt = init;
  if (this->initMqtt) {
    this->start();
  }
}

void EspMQTT::loop() {
  if (this->online && this->ota) {
    // ArduinoOTA.handle();
  }
}


void EspMQTT::connectToWifi() {
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(this->WiFiSsid, this->WiFiPass);
}

void EspMQTT::connectToWifiStatic() {
  mqtt.connectToWifi();
}

void EspMQTT::onWifiConnect(const WiFiEventStationModeGotIP& event) {
  string ip = WiFi.localIP().toString().c_str();
  strcpy(mqtt.ip, ip.c_str());
  if (mqtt.debug) {
    Serial.print("Connected to Wi-Fi. IP:");
    Serial.println(ip.c_str());
  }
  connectToMqtt();
}

void EspMQTT::onWifiDisconnect(const WiFiEventStationModeDisconnected& event) {
  mqtt.setOffline();
  if (mqtt.debug) {
    Serial.println("Disconnected from Wi-Fi.");
  }
  // Ensure we don't reconnect to MQTT while reconnecting to Wi-Fi.
  mqttAvailabilityTimer.detach();
  mqttReconnectTimer.detach();
  wifiReconnectTimer.once(2, connectToWifiStatic);
}

void EspMQTT::connectToMqtt() {
  if (mqtt.debug) {
    Serial.println("Connecting to MQTT...");
  }
  // string clientId = "ESP8266-";
  // clientId += ESP.getChipId();
  // clientId += "-";
  // clientId += string(random(0xffff), HEX);
  // mqttClient.setClientId(clientId.c_str());
  mqttClient.connect();
}

void EspMQTT::onMqttConnectStatic(bool sessionPresent) {
  mqttAvailabilityTimer.attach_ms(mqtt.availabilityInterval, publishAvailabilityStatic);
  mqtt.onMqttConnect();
  mqtt.onMqttConnectTests();
  if (mqtt.debug) {
    Serial.printf("Session present: %d\n", sessionPresent);
  }
}

void EspMQTT::onMqttConnect() {
  this->publishAvailability();
  this->mqttSubscribe();
  this->setOnline();
  if (this->debug) {
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
  this->availabilityInterval = sec * 1000;
  mqttAvailabilityTimer.detach();
  if (this->online) {
    mqttAvailabilityTimer.attach_ms(sec * 1000, publishAvailabilityStatic);
  }
}

void EspMQTT::onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  mqtt.setOffline();
  if (mqtt.debug) {
    Serial.println("Disconnected from MQTT.");
  }
  if (WiFi.isConnected()) {
    mqttReconnectTimer.once(2, connectToMqtt);
  }
}

void EspMQTT::onMqttSubscribe(uint16_t packetId, uint8_t qos) {
  if (mqtt.debug) {
    Serial.printf("Subscribe: packetId=%d | QOS=%d\n", packetId, qos);
  }
}

void EspMQTT::onMqttUnsubscribe(uint16_t packetId) {
  if (mqtt.debug) {
    Serial.printf("Unsubscribe: packetId=%d\n", packetId);
  }
}

void EspMQTT::onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
  mqtt.callback(topic, payload, len);
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

void EspMQTT::setOnline() {
  this->online = true;
  digitalWrite(LED_BUILTIN, HIGH);
}

void EspMQTT::setOffline() {
  this->online = false;
  digitalWrite(LED_BUILTIN, LOW);
  if (this->debug) {
    Serial.println("Disconnected from MQTT.");
  }
}

void EspMQTT::setCallback(std::function<void(string param, string value)> cBack) {
    this->callbackFunction = cBack;
};

void EspMQTT::setDebug(bool debug) {
  this->debug = debug;
  if (this->debug) {
    Serial.println("DEBUG > ON");
  }
}

void EspMQTT::callback(char *topic, char* payload, uint16_t length) {
  if (length >= 1024) {
    this->publishState("$error", "Message is too big");
    return;
  }
  string param = string(topic).substr(this->cmdTopicLength);
  string message = string(payload, length);
  if ((char)payload[0] != '{') {
    // JSON. Do Nothing.
  }
  if ((char)param.at(0) != '$') {
    this->callbackFunction(param, message);
  }
  else {
    // App.
    eapp.app(param, message);
  }
  if (this->debug) {
    Serial.printf("MQTT [%s] %s=%s\n", topic, param.c_str(), message.c_str());
  }
}

void EspMQTT::publishAvailabilityStatic() {
  mqtt.publishAvailability();
}

void EspMQTT::publishAvailability() {
  mqttClient.publish(ipTopic, 0, true, this->ip);
  mqttClient.publish(availabilityTopic, 0, true, "online");
  if (this->debug) {
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
