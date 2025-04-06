#include <WiFiNINA.h>
#include <PubSubClient.h>
#include "Secrets.h"

char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;

char server[] = "broker.emqx.io"; // Public MQTT broker
const int port = 1883; // MQTT port

// Topics to subscribe to
const char *waveTopic = "SIT210/wave";
const char *patTopic = "SIT210/pat";
char name[] = "Brandon"; // Message to publish in the Topics

// HC-SR04 digital pins
const uint8_t echoPin = 7;
const uint8_t trigPin = 6;

// Led digital pin
const uint8_t ledPin = 5;

// Thresholds used to categories echo readings to publish messages to respective topics
const int WAVE_DISTANCE_THRESHOLD = 10;
const int PAT_DISTANCE_THRESHOLD = 20;

// Variables to store ultrasonic sensor readings
unsigned long duration_us = 0;
unsigned long distance_cm = 0;

// The Wifi and MQTT clients
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
  handleConnections(); // Ensure WiFi and MQTT are connected
  if (detectWavePat()) { // Check if movement is detected

    // Categorise the readings based on the different thresholds to publish a message to the respective topics
    if (distance_cm <= WAVE_DISTANCE_THRESHOLD) client.publish(waveTopic, name); // Publish name to the wave topic
    else client.publish(patTopic, name); // Publish name to the pat topic
  }
  client.loop(); // Keeps the MQTT connection active and handle message callback (actions based on what topic the mesage was received on)
}

// Handless wifi connections and reconnections
void connectWiFi() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print(F("Attempting to connect to SSID: "));
    Serial.println(ssid);
    while (WiFi.begin(ssid, pass) != WL_CONNECTED) delay(5000);
    Serial.println(F("WiFi connected!"));
  }
}

// Handless MQTT connections and reconnections
void connectMQTT() {
  while (!client.connected()) {
    Serial.println(F("Connecting to MQTT..."));
    if (client.connect("ArduinoNano-Task3.3D")) {
      Serial.println(F("MQTT connected!"));
      client.subscribe(waveTopic); // Subscribe to the wave topic
      client.subscribe(patTopic); // Subscribe to the pat topic
    } else { // Print the error if the connection failed
      Serial.print(F("MQTT failed, rc="));
      Serial.print(client.state());
      delay(5000);
    }
  }
}

// Bundles Wifi and MQTT cconnections into a singular call
void handleConnections() {
  connectWiFi();
  connectMQTT();
}

// Callback handles actions based on what message was received
void callback(char *topic, uint8_t *payload, unsigned int length) {
  payload[length] = '\0';  // Adds null terminator to end
  String message = (char*)payload;

  // Prints what topic the message was published to
  Serial.print(F("Published the message [\""));
  Serial.print(message);
  Serial.print(F("\"]"));
  Serial.print(F(" to the "));
  Serial.print(topic);
  Serial.println(F(" topic"));

  // Blink the led differently based on what topic the message was received on
  if (String(topic) == "SIT210/wave") blinkLED(3);
  else if (String(topic) == "SIT210/pat") blinkLED(5);
}

bool detectWavePat() {
  static unsigned long lastReadTime = 0; // Tracks when the sensor was last read
  if (millis() - lastReadTime < 500) return false; // Ensures readings are taken every 0.5s

  // Activate the trigger of the ultrasonic sensor
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  duration_us = pulseIn(echoPin, HIGH); // Read the echo duration
  distance_cm = 0.01715 * duration_us; // Calculate the distance

  lastReadTime = millis(); // Update when the sensor was last read

  // Return true only if distance is valid and within upper bounds threshold
  return distance_cm > 0 && distance_cm <= PAT_DISTANCE_THRESHOLD;
}

// Blink the LED a specified number of times
void blinkLED(int blinks) {
  for (int i = 0; i < blinks; i++) {
    digitalWrite(ledPin, HIGH);
    delay(200);
    digitalWrite(ledPin, LOW);
    delay(200);
  }
}
