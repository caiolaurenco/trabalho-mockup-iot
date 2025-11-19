#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ESP32Servo.h>
#include "env.h"

WiFiClientSecure client;
PubSubClient mqtt(client);

Servo meuServo;

const byte SERVO_PIN = 18;
const byte TRIGGER_PIN = 26;
const byte ECHO_PIN = 25;

unsigned long lastMsg = 0;
char msg[50];
bool objetoProximoAnterior = false; 

void conectaWiFi() {
  Serial.println("Conectando ao WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nWiFi conectado!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

void conectaMQTT() {
  while (!mqtt.connected()) {
    Serial.print("Conectando ao broker MQTT...");
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);

    if (mqtt.connect(clientId.c_str(), BROKER_USER, BROKER_PASS)) {
      Serial.println("Conectado!");
      mqtt.subscribe(TOPIC_ILUM);
    } else {
      Serial.print("Falha (rc=");
      Serial.print(mqtt.state());
      Serial.println("), tentando novamente em 5s");
      delay(5000);
    }
  }
}

long lerDistancia() {
  digitalWrite(TRIGGER_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIGGER_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGGER_PIN, LOW);

  long duracao = pulseIn(ECHO_PIN, HIGH);
  long distancia = duracao * 0.034 / 2;

  return distancia;
}

void callback(char* topic, byte* payload, unsigned int length) {
  String msg = "";
  for (int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }

  Serial.print("Mensagem recebida em ");
  Serial.print(topic);
  Serial.print(": ");
  Serial.println(msg);

  if (String(topic) == TOPIC_ILUM) {
    if (msg == "Acender") {
      for (int pos = 0; pos <= 180; pos++) {
        meuServo.write(pos);
        delay(10);
      }
    } else if (msg == "Apagar") {
      for (int pos = 180; pos >= 0; pos--) {
        meuServo.write(pos);
        delay(10);
      }
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(TRIGGER_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  meuServo.attach(SERVO_PIN);

  client.setInsecure();
  conectaWiFi();

  mqtt.setServer(BROKER_URL, BROKER_PORT);
  mqtt.setCallback(callback);
  conectaMQTT();
}

void loop() {
  if (!mqtt.connected()) {
    conectaMQTT();
  }
  mqtt.loop();

  unsigned long agora = millis();
  if (agora - lastMsg > 2000) {
    lastMsg = agora;

    long distancia = lerDistancia();
    Serial.print("Distância: ");
    Serial.print(distancia);
    Serial.println(" cm");

    if (distancia > 0 && distancia < 10) {
      if (!objetoProximoAnterior) { 
        mqtt.publish(TOPIC_ILUM, "Objeto proximo");
        Serial.println("Objeto próximo! Enviado ao MQTT.");
        objetoProximoAnterior = true;
      }
    } else {
      objetoProximoAnterior = false;
    }
  }
}