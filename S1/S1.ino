#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "env.h"

WiFiClientSecure client;
PubSubClient mqtt(client);


const byte ldr = 34;
int threshold = 0;

void setup() {
  Serial.begin(115200);
  pinMode(ldr, INPUT);
  pinMode(19, OUTPUT);
  threshold = analogRead(ldr);
  client.setInsecure();
  Serial.println("Conectando no WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASS); 
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
  mqtt.setCallback(callback);  
  Serial.println("\nConectado ao Broker!");
  Serial.println(threshold);
}

void loop() {
  int valor_ldr = analogRead(ldr);
  // Serial.println(valor_ldr);
  if ((valor_ldr > threshold + 100)||(valor_ldr < threshold - 100)) {
    mqtt.publish(TOPIC_ILUM, "Claro"); 
    mqtt.loop();
    delay(2000);
  } else {
    mqtt.publish(TOPIC_ILUM, "Escuro"); 
    mqtt.loop();
    delay(2000);
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  String msg = "";
  for (int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }
  msg = msg.c_str();
  if (strcmp(topic,TOPIC_ILUM) == 0 && msg == "Claro") {  
    Serial.println(msg);
    digitalWrite(19, LOW);
  } else if (strcmp(topic,TOPIC_ILUM)==0 && msg == "Escuro") { 
    digitalWrite(19, HIGH);
    Serial.println(msg);
  }
}