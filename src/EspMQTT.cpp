#include "EspMQTT.h"

// MQTT.
AsyncMqttClient mqttClient;
Ticker mqttReconnectTimer;

WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;
Ticker wifiReconnectTimer;

void EspMQTT::start() {
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
  connectToWifiStatic();
}

void EspMQTT::start(bool init) {
  this->initMqtt = init;
  if (this->initMqtt) {
    this->start();
  }
}

void EspMQTT::loop() {
  if (this->initMqtt && this->online) {
    if ((millis() - availabilityInterval) >= availabilityTimer && millis() > availabilityInterval) {
      availabilityTimer = millis();
      this->publishAvailability();
    }
    /*
    if (this->ota) {
      // ArduinoOTA.handle();
    }*/
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
  String local_ip = WiFi.localIP().toString();
  char* ip = (char*) strdup(local_ip.c_str());
  mqtt.ip = ip;
  if (mqtt.debug) {
    Serial.print("Connected to Wi-Fi. IP:");
    Serial.println(ip);
  }
  connectToMqtt();
}

void EspMQTT::onWifiDisconnect(const WiFiEventStationModeDisconnected& event) {
  mqtt.setOffline();
  if (mqtt.debug) {
    Serial.println("Disconnected from Wi-Fi.");
  }
  // Ensure we don't reconnect to MQTT while reconnecting to Wi-Fi.
  mqttReconnectTimer.detach();
  wifiReconnectTimer.once(2, connectToWifiStatic);
}

void EspMQTT::connectToMqtt() {
  Serial.println("Connecting to MQTT...");
  mqttClient.connect();
}

void EspMQTT::onMqttConnectStatic(bool sessionPresent) {
  digitalWrite(LED_BUILTIN, HIGH);
  String ip = WiFi.localIP().toString();
  mqttClient.publish(mqtt.ipTopic, 1, true, (char*) strdup(ip.c_str()));
  mqttClient.publish(mqtt.availabilityTopic, 1, true, "online");
  mqtt.onMqttConnect();
  // mqtt.onMqttConnectStaticDemo(sessionPresent);
}

void EspMQTT::onMqttConnect() {
  digitalWrite(LED_BUILTIN, HIGH);
  this->mqttSubscribe();
  this->setOnline();
}

void EspMQTT::onMqttConnectStaticDemo(bool sessionPresent) {
  // Old.
  Serial.println("Connected to MQTT.");
  Serial.print("Session present: ");
  Serial.println(sessionPresent);
  uint16_t packetIdSub = mqttClient.subscribe("test/lol", 2);
  Serial.print("Subscribing at QoS 2, packetId: ");
  Serial.println(packetIdSub);
  mqttClient.publish("test/lol", 0, true, "test 1");
  Serial.println("Publishing at QoS 0");
  uint16_t packetIdPub1 = mqttClient.publish("test/lol", 1, true, "test 2");
  Serial.print("Publishing at QoS 1, packetId: ");
  Serial.println(packetIdPub1);
  uint16_t packetIdPub2 = mqttClient.publish("test/lol", 2, true, "test 3");
  Serial.print("Publishing at QoS 2, packetId: ");
  Serial.println(packetIdPub2);
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
  Serial.println("Subscribe acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
  Serial.print("  qos: ");
  Serial.println(qos);
}

void EspMQTT::onMqttUnsubscribe(uint16_t packetId) {
  Serial.println("Unsubscribe acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
}

void EspMQTT::onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
  mqtt.callback(topic, payload, len);
  if (false) {
    Serial.println("Publish received.");
    Serial.print("  topic: ");
    Serial.println(topic);
    Serial.print("  qos: ");
    Serial.println(properties.qos);
    Serial.print("  dup: ");
    Serial.println(properties.dup);
    Serial.print("  retain: ");
    Serial.println(properties.retain);
    Serial.print("  len: ");
    Serial.println(len);
    Serial.print("  index: ");
    Serial.println(index);
    Serial.print("  total: ");
    Serial.println(total);
  }
}

void EspMQTT::onMqttPublish(uint16_t packetId) {
  if (false) {
    Serial.println("Publish acknowledged.");
    Serial.print("  packetId: ");
    Serial.println(packetId);
  }
}

void EspMQTT::setOffline() {
  this->online = false;
  digitalWrite(LED_BUILTIN, LOW);
  this->reconnectStart = millis();
  if (this->debug) {
    Serial.println("Disconnected from MQTT.");
  }
}

void EspMQTT::setOnline() {
  this->online = true;
  digitalWrite(LED_BUILTIN, HIGH);
}



void EspMQTT::setWiFi(String ssid, String pass, String host) {
  this->WiFiSsid = (char*) strdup(ssid.c_str());
  this->WiFiPass = (char*) strdup(pass.c_str());
  this->WiFiHost = (char*) strdup(host.c_str());
};

void EspMQTT::setMqtt(String server, String user, String pass) {
  this->mqttServer = (char*) strdup(server.c_str());
  this->mqttUser   = (char*) strdup(user.c_str());
  this->mqttPass   = (char*) strdup(pass.c_str());
};

void EspMQTT::setCommonTopics(String root, String name) {
  // Root: /home/[sensor/switch/light]/{name}
  String r = root + "/" + name;
  this->mqttRootTopic = r;
  // Metrics.
  String mRoot = r + String("/metric/");
  this->metricRoot = (char*) strdup(mRoot.c_str());
  // Info.
  String availability = r + String("/availability");
  String ip = availability + String("/$ip");
  this->ipTopic = (char*) strdup(ip.c_str());
  this->availabilityTopic = (char*) strdup(availability.c_str());
  // Commands & Data.
  String cmd = r + String("/cmd/*");
  String data = r + String("/data");
  String state = r + String("/state");
  String recovery = r + String("/recovery");
  this->dataTopic     = (char*) strdup(data.c_str());
  this->stateTopic    = (char*) strdup(state.c_str());
  this->recoveryTopic = (char*) strdup(recovery.c_str());
  // Subscribe.
  this->cmdTopic = (char*) strdup(cmd.c_str());
  Topics *topics = new Topics(this->cmdTopic);
  this->topics = topics;
};

void EspMQTT::addSubsribeTopic(String topic) {
  String subscribe = this->mqttRootTopic + "/" + String(topic);
  this->topics->addTopic((char*) strdup(subscribe.c_str()));
}

void EspMQTT::setCallback(std::function<void(String param, String value)> cBack) {
  this->callbackFunction = cBack;
};


void EspMQTT::setDebug(bool debug) {
  this->debug = debug;
  if (this->debug) {
    Serial.println("DEBUG > ON");
  }
}

void EspMQTT::setReconnectInterval(int sec) {
  this->reconnectInterval = sec * 1000;
}
void EspMQTT::setAvailabilityInterval(int sec) {
  this->availabilityInterval = sec * 1000;
}

void EspMQTT::callback(char *topic, char* payload, int length) {
  int from = String(this->cmdTopic).length() - 1;
  String param = String(topic).substring(from);
  String message = String(payload);
  this->callbackFunction(param, message);
  if (this->debug) {
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] *");
    Serial.print(param);
    Serial.print("*= ");
    Serial.println(payload);
  }
  /*
  if (length == 1) {
    char op = (char)payload[0];
  }
  else {
    String message = this->callbackGetMessage(payload, length);
    if ((char)payload[0] != '{') {
      this->callbackFunction(param, message);
    }
  }
  this->callbackDebug(topic, payload, length, param);
  */
}

void EspMQTT::reconnectInit() {
  // 0 - Led ON!
  if (this->reconnectStep == 0) {
    digitalWrite(LED_BUILTIN, LOW);
    this->reconnectStart = millis();
    this->reconnectStep++;
    // Reconnect Delay.
    if (this->online == false) {
      delay(100);
    }
  }
}

void EspMQTT::reconnectWatchDog() {
  // Reboot after 2 minutes fails.
  if (this->online == false && ((millis() - this->reconnectStart) > 120000)) {
    if (this->debug) {
      Serial.println("Restarting!");
    }
    ESP.restart();
  }
}

void EspMQTT::reconnectWiFi() {
  // 1 - Awaiting WiFi Connection.
  if (this->reconnectStep == 1) {
    if (WiFi.status() != WL_CONNECTED) {
      this->online = false;
      while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        if (this->debug) {
          Serial.print(".");
        }
      }
      if ((millis() - this->reconnectTimer) > this->reconnectInterval) {
        if (this->debug) {
          Serial.print("Awaiting WiFi Connection (");
          Serial.print((millis() - this->reconnectStart) / 1000);
          Serial.print("s) ");
          Serial.println(WiFi.macAddress());
        }
        this->reconnectTimer = millis();
      }
    }
    else {
      if (this->debug) {
        Serial.println("WiFi connected!");
        Serial.print("SSID: ");
        Serial.print(WiFi.SSID());
        Serial.println("");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
        Serial.println("");
      }

      this->reconnectTimer = 0;
      this->reconnectStep = 2;
    }
  }
}

void EspMQTT::reconnectMqtt() {
  // 2 - Check MQTT Connection.
  // client.connect(clientId.c_str(), mqttUser, mqttPass, availabilityTopic, 0, true, "0");
  if (this->reconnectStep == 2) {
    String clientId = "ESP8266-";
    clientId += ESP.getChipId();
    clientId += "-";
    clientId += String(random(0xffff), HEX);
  }
}

void EspMQTT::mqttSubscribe() {
  int size = this->topics->getSize();
  char** list = this->topics->getTopics();
  // Subscribe all.
  for(int i = 0; i < size; i++){
    mqttClient.subscribe(list[i], 2);
    if (this->debug) {
      Serial.println(list[i]);
    }
  }
}

void EspMQTT::publishAvailability() {
  mqttClient.publish(ipTopic, 1, true, this->ip);
  mqttClient.publish(availabilityTopic, 1, true, "online");
  if (this->debug) {
    Serial.println("Publish Availability");
    Serial.println(ip);
  }
}

void EspMQTT::publishData(String data) {
  // mqttClient.publish(dataTopic, data.c_str());
}

void EspMQTT::publishState(String key, String value){
  String topic = String(this->stateTopic) + "/" + String(key);
  mqttClient.publish(topic.c_str(), 1, true, String(value).c_str());
}

void EspMQTT::publishMetric(char *key, int metric) {
  char message[16];
  itoa(metric, message, 10);
  char topic[255];
  strcpy(topic, this->metricRoot);
  strcat(topic, key);
  mqttClient.publish(topic, 1, true, message);
}

void EspMQTT::publishMetric(String key, int metric) {
  char message[16];
  itoa(metric, message, 10);
  String topic = String(this->metricRoot) + key;
  mqttClient.publish(topic.c_str(), 1, true, message);
}

void EspMQTT::publishMetric(String key, float metric) {
  if (metric > 0) {
    this->publishMetric(key, metric, false);
  }
}

void EspMQTT::publishMetric(String key, float metric, bool force) {
  if (metric > 0 || force) {
    char message[16];
    itoa(metric, message, 10);
    String topic = String(this->metricRoot) + key;
    mqttClient.publish(topic.c_str(), 1, true, message);
  }
}
