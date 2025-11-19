#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "env.h"
#include <DHT.h>
#define DHTPIN 4
#define DHTTYPE DHT11  // tipo do sensor

WiFiClientSecure client;
PubSubClient mqtt(client);
DHT dht(DHTPIN, DHTTYPE);


const byte ldr = 34;
int threshold = 0;
const byte TRIGGER_PIN = 22;
const byte ECHO_PIN = 23;

void setup() {
  Serial.begin(115200);
  pinMode(ldr, INPUT);
  pinMode(19, OUTPUT);
  pinMode(TRIGGER_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  dht.begin();
  threshold = analogRead(ldr);
  client.setInsecure();
  Serial.println("Conectando no WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASS);  //Tenta conectar na rede
  while (WiFi.status() != WL_CONNECTED) {
    Serial.println(".");
    delay(200);
  }
  Serial.println("\nConectado com Sucesso no WiFi!");

  Serial.println("Conectando ao Broker...");
  mqtt.setServer(BROKER_URL, BROKER_PORT);
  String BoardID = "S1";
  BoardID += String(random(0xffff), HEX);
  mqtt.connect(BoardID.c_str(), BROKER_USER, BROKER_PASS);
  while (!mqtt.connected()) {
    Serial.print(".");
    delay(200);
  }
  mqtt.subscribe(TOPIC_ILUM);
  mqtt.setCallback(callback);  // Recebe a mensagem
  Serial.println("\nConectado ao Broker!");
  Serial.println(threshold);
}

long lerDistancia() {
  digitalWrite(TRIGGER_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIGGER_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGGER_PIN, LOW);
  
  long duracao = pulseIn(ECHO_PIN, HIGH);
  long distancia = duracao * 349.24 / 2 / 10000; // cm
  
  return distancia;
}

void loop() {
  long distancia = lerDistancia();
  int valor_ldr = analogRead(ldr);
  float umidade = dht.readHumidity();
  float temp = dht.readTemperature();

  Serial.printf("Temperatura: %.2f | Distância: %d  | Umidade: %.2f\n", temp , distancia , umidade);

  if (valor_ldr > threshold) {
    mqtt.publish(TOPIC_ILUM, "Escuro");  // Envia a mensagem
  } else {
    mqtt.publish(TOPIC_ILUM, "Claro");  // Envia a mensagem
  }

  mqtt.publish("s1/distancia", String(distancia).c_str());
  mqtt.publish("s1/temperatura", String(temp).c_str());
  mqtt.publish("s1/umidade", String(umidade).c_str());
  
  delay(1000);
  mqtt.loop();
}

void callback(char* topic, byte* payload, unsigned int length) {
  String msg = "";
  for (int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }
  msg = msg.c_str();

  if (strcmp(topic, TOPIC_ILUM) == 0 && msg == "Claro") {  // Verifica mensagem do tópico de luz do S1 e fala para acender led
    Serial.println(msg);
    digitalWrite(19, LOW);
  } else if (strcmp(topic, TOPIC_ILUM) == 0 && msg == "Escuro") {  // Verifica mensagem do tópico de luz do S1 e fala para apagar led
    digitalWrite(19, HIGH);
    Serial.println(msg);
  }
}