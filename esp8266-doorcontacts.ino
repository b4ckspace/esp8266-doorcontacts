#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoOTA.h>
#include <Bounce2.h>
#include "settings.h"

typedef struct {
  uint8_t pin;
  const char* mqttTopic;
  const char* valueUp;
  const char* valueDown;
  uint16_t debounceMs;
  Bounce *debouncer;
} t_pinConfiguration;

t_pinConfiguration pins[] = {
  {D2, "sensor/door/hackcenter", "open", "closed", 200, NULL},
  {D5, "sensor/door/cnc",        "open", "closed", 200, NULL},
  {D7, "sensor/door/lounge",     "open", "closed", 200, NULL}
};

WiFiClient wifiClient;
PubSubClient mqttClient;
Bounce debouncer = Bounce();

void setup() {
  
  Serial.begin(115200);
  delay(10);

  WiFi.mode(WIFI_STA);
  WiFi.hostname(HOSTNAME);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  for (uint8_t i = 0; i < ARRAY_SIZE(pins); i++) {
    pinMode(pins[i].pin, INPUT_PULLUP);

    pins[i].debouncer = new Bounce();
    pins[i].debouncer->attach(pins[i].pin);
    pins[i].debouncer->interval(100);
  }
  
  mqttClient.setClient(wifiClient);
  mqttClient.setServer(MQTT_HOST, 1883);
  
  ArduinoOTA.setHostname(HOSTNAME);
  ArduinoOTA.setPassword(OTA_PASSWORD);
  ArduinoOTA.begin();
}

void connectMqtt() {
  while (!mqttClient.connected()) {
    if (mqttClient.connect(HOSTNAME, MQTT_TOPIC_LAST_WILL, 1, true, "disconnected")) {
      mqttClient.publish(MQTT_TOPIC_LAST_WILL, "connected", true);
    } else {
      delay(1000);
    }
  }
}

void loop() {

  for (uint8_t i = 0; i < ARRAY_SIZE(pins); i++) {
    pins[i].debouncer->update();

    if (pins[i].debouncer->rose()) {
      mqttClient.publish(pins[i].mqttTopic, pins[i].valueUp, true);
    } else if (pins[i].debouncer->fell()) {
      mqttClient.publish(pins[i].mqttTopic, pins[i].valueDown, true);
    }
  }

  connectMqtt();
  mqttClient.loop();
  ArduinoOTA.handle();
}
