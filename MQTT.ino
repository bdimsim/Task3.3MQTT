#include <WiFiNINA.h>
#include <PubSubClient.h>
#include "Secrets.h"

char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;

char server[] = "broker.emqx.io";
const int port = 1883;
const char *waveTopic = "SIT210/wave";
const char *patTopic = "SIT210/pat";
char name[] = "Brandon";

const uint8_t echoPin = 7;
const uint8_t trigPin = 6;
const uint8_t ledPin = 5;

const int WAVE_DISTANCE_THRESHOLD = 10;
const int PAT_DISTANCE_THRESHOLD = 20;

long duration_us = 0;
long distance_cm = 0;

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  pinMode(ledPin, OUTPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  Serial.begin(115200);
  client.setServer(server, port);
  client.setCallback(callback);
}

void loop() {
  handleConnections();
  if (detectWavePat()) {
    if (distance_cm <= WAVE_DISTANCE_THRESHOLD) client.publish(waveTopic, name);
    else client.publish(patTopic, name);
  }
  client.loop();
}

void connectWiFi() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print(F("Attempting to connect to SSID: "));
    Serial.println(ssid);
    while (WiFi.begin(ssid, pass) != WL_CONNECTED) delay(5000);
    Serial.println(F("WiFi connected!"));
  }
}

void connectMQTT() {
  while (!client.connected()) {
    Serial.println(F("Connecting to MQTT..."));
    if (client.connect("ArduinoNano-Task3.3D")) {
      Serial.println(F("MQTT connected!"));
      client.subscribe(waveTopic);
      client.subscribe(patTopic);
    } else {
      Serial.print(F("MQTT failed, rc="));
      Serial.print(client.state());
      delay(5000);
    }
  }
}

void handleConnections() {
  connectWiFi();
  connectMQTT();
}

// Callback handles actions based on what message was received
// Payload is a raw byte array of unsigned chars
void callback(char *topic, uint8_t *payload, unsigned int length) {
  payload[length] = '\0';  // Adds null terminator to end
  String message = (char*)payload;

  Serial.print(F("Published the message [\""));
  Serial.print(message);
  Serial.print(F("\"]"));
  Serial.print(F(" to the "));
  Serial.print(topic);
  Serial.println(F(" topic"));

  if (String(topic) == "SIT210/wave") blinkLED(3);
  else if (String(topic) == "SIT210/pat") blinkLED(5);
}

bool detectWavePat() {
  static unsigned long lastReadTime = 0;
  if (millis() - lastReadTime < 500) return false;

  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  lastReadTime = millis();

  duration_us = pulseIn(echoPin, HIGH);
  distance_cm = 0.01715 * duration_us;
  return distance_cm > 0 && distance_cm <= PAT_DISTANCE_THRESHOLD;
}

void blinkLED(int blinks) {
  for (int i = 0; i < blinks; i++) {
    digitalWrite(ledPin, HIGH);
    delay(200);
    digitalWrite(ledPin, LOW);
    delay(200);
  }
}
