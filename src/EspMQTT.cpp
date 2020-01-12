#include <Arduino.h>
#include "EspMQTT.h"

// Wifi
WiFiClient espClient;

// MQTT.
PubSubClient client(espClient);

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
  this->metricRoot = r + String("/metric");
  // Info.
  String ip = r + String("/ip");
  String availability = r + String("/availability");
  this->ipTopic = (char*) strdup(ip.c_str());;
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

void EspMQTT::start() {
  if (this->debug) {
    Serial.println("MQTT Start");
  }
  WiFi.mode(WIFI_STA);
  WiFi.hostname(this->WiFiHost);
  WiFi.begin(this->WiFiSsid, this->WiFiPass);
  this->otaBegin();
  client.setServer(this->mqttServer, this->mqttPort);
  client.setCallback(callbackStatic);
  this->reconnect();
}

void EspMQTT::start(bool init) {
  this->initMqtt = init;
  if (this->initMqtt) {
    this->start();
  }
}

void EspMQTT::loop() {
  if (this->initMqtt) {
    if (!client.connected()) {
      this->reconnect();
    }
    else {
      client.loop();
      if ((millis() - availabilityInterval) >= availabilityTimer && millis() > availabilityInterval) {
        this->publishAvailability();
        availabilityTimer = millis();
      }
      if (this->ota) {
        ArduinoOTA.handle();
      }
    }
  }
}

void EspMQTT::setDebug(bool debug) {
  this->debug = debug;
  if (this->debug) {
    Serial.println("DEBUG > ON");
  }
}

void EspMQTT::setOta(bool ota) {
  this->ota = ota;
}

void EspMQTT::setReconnectInterval(int sec) {
  this->reconnectInterval = sec * 1000;
}
void EspMQTT::setAvailabilityInterval(int sec) {
  this->availabilityInterval = sec * 1000;
}

void EspMQTT::otaBegin() {
  if (this->ota) {
    ArduinoOTA.setHostname(this->WiFiHost);

    ArduinoOTA.onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH) {
        type = "sketch";
      }
      else { // U_SPIFFS
        type = "filesystem";
      }

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    });
    ArduinoOTA.onEnd([]() {
      Serial.println("\nEnd");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) {
        Serial.println("Auth Failed");
      }
      else if (error == OTA_BEGIN_ERROR) {
        Serial.println("Begin Failed");
      }
      else if (error == OTA_CONNECT_ERROR) {
        Serial.println("Connect Failed");
      }
      else if (error == OTA_RECEIVE_ERROR) {
        Serial.println("Receive Failed");
      }
      else if (error == OTA_END_ERROR) {
        Serial.println("End Failed");
      }
    });
    ArduinoOTA.begin();
  }
}


void EspMQTT::callbackStatic(char *topic, byte *payload, int length) {
  mqtt.callback(topic, payload, length);
}

void EspMQTT::callback(char *topic, byte *payload, int length) {
  int from = String(this->cmdTopic).length() - 1;
  String param = String(topic).substring(from);
  if (length == 1) {
    char op = (char)payload[0];
    this->callbackFunction(param, String(op));
  }
  else {
    String message = this->callbackGetMessage(payload, length);
    if ((char)payload[0] != '{') {
      this->callbackFunction(param, message);
    }
    else {
      this->callbackParceJson(message);
    }
  }
  this->publishRecovery();
  this->callbackDebug(topic, payload, length, param);
}

String EspMQTT::callbackGetMessage(byte *dat, int len) {
  char message[256];
  for (int i = 0; i < len; i++) {
    message[i] = dat[i];
    if (i == (len - 1)) {
      message[i + 1] = '\0';
    }
  }
  return String(message);
}

void EspMQTT::callbackParceJson(String message) {
  char json[256];
  message.toCharArray(json, 256);
  DynamicJsonDocument doc(1024);
  auto error = deserializeJson(doc, json);
  if (!error) {

  }
  else if (this->debug) {
    Serial.print(F("deserializeJson() failed with code "));
    Serial.println(error.c_str());
  }
}

void EspMQTT::callbackDebug(char *topic, byte *dat, int len, String param) {
  if (this->debug) {
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] *");
    Serial.print(param);
    Serial.print("*= ");
    for (int i = 0; i < len; i++) {
      Serial.print((char)dat[i]);
    }
    Serial.println();
  }
}

void EspMQTT::reconnect() {
  this->reconnectInit();
  this->reconnectWatchDog();
  this->reconnectWiFi();
  this->reconnectMqtt();
  this->reconnectSubscribe();
}

void EspMQTT::reconnectInit() {
  // 0 - Led ON!
  if (this->reconnectStep == 0) {
    digitalWrite(LED_BUILTIN, LOW);
    this->reconnectStart = millis();
    this->reconnectStep++;
    // Reconnect Delay.
    if (this->online == false) {
      delay(500);
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
      if (this->debug) {
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
  if (this->reconnectStep == 2) {
    if (!client.connected()) {
      if ((millis() - this->reconnectTimer) > this->reconnectInterval) {
        String clientId = "ESP8266-";
        clientId += ESP.getChipId();
        clientId += "-";
        clientId += String(random(0xffff), HEX);
        if (this->debug) {
          Serial.print("Awaiting MQTT Connection (");
          Serial.print((millis() - this->reconnectStart) / 1000);
          Serial.print("s) | clientId= ");
          Serial.println(clientId);
        }
        this->reconnectTimer = millis();
        client.connect(clientId.c_str(), mqttUser, mqttPass, availabilityTopic, 0, true, "0");
      }

      // Check the MQTT again.
      if (client.connected()) {
        this->reconnectTimer = 0;
        this->reconnectStep = 3;
        if (this->debug) {
          Serial.println("MQTT connected!\n");
        }
      }

      // Check the WiFi again.
      else if (WiFi.status() != WL_CONNECTED) {
        this->reconnectStep = 1;
      }
    }
    else {
      this->reconnectTimer = 0;
      this->reconnectStep = 3;
      if (this->debug) {
        Serial.println("MQTT connected!\n");
      }
    }
  }
}

void EspMQTT::reconnectSubscribe() {
  // 3 - All connected, turn the LED back on and then MQTT subscribe.
  if (this->reconnectStep == 3) {
    digitalWrite(LED_BUILTIN, HIGH);
    this->publishAvailability();
    int size = this->topics->getSize();
    char** list = this->topics->getTopics();
    // MQTT Subscriptions.
    for(int i = 0; i < size; i++){
      client.subscribe(list[i]);
      if (this->debug) {
        Serial.println(list[i]);
      }
    }
    if (this->online == false) {
      this->online = true;
      if (this->debug) {
        Serial.println("OnLine!");
        Serial.println("");
      }
    }
    // Reset Steps!
    this->reconnectStep = 0;
  }
}

void EspMQTT::publishRecovery() {
  if (false) {
    client.publish(recoveryTopic, "+", true);
  }
}

void EspMQTT::publishAvailability() {
  String ip = WiFi.localIP().toString();
  client.publish(ipTopic,(char*) strdup(ip.c_str()));
  client.publish(availabilityTopic, "online", true);
  if (this->debug) {
    Serial.println("Publish Availability");
  }
}

void EspMQTT::publishData(String data) {
  client.publish(dataTopic, data.c_str());
}

void EspMQTT::publishState(String key, String value){
  String topic = String(this->stateTopic) + "/" + String(key);
  client.publish(topic.c_str(), String(value).c_str(), true);
}

void EspMQTT::publishMetric(String key, float metric) {
  if (metric > 0) {
    this->publishMetric(key, metric, false);
  }
}

void EspMQTT::publishMetric(String key, float metric, bool force) {
  if (metric > 0 || force) {
    String topic = this->metricRoot + "/" + String(key);
    client.publish(topic.c_str(), String(metric).c_str(), true);
  }
}
