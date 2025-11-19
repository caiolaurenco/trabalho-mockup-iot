#include <WiFi.h>
#include <PubSubClient.h>
#include "env.h"

WiFiClient client;
PubSubClient mqtt(client);


void setup() {
  Serial.begin(115200);
  Serial.println("Conectando no WiFi");
  WiFi.begin(WIFI_SSID,WIFI_PASS); //Tenta conectar na rede
  while(WiFi.status() != WL_CONNECTED){
    Serial.println(".");
    delay(200);
  }
  
  Serial.println("\nConectado com Sucesso no WiFi!");


  Serial.println("Conectando ao Broker...");
  mqtt.setServer(BROKER_URL,BROKER_Port);
  String BoardID = "S1";
  BoardID += String(random(0xffff),HEX);
  mqtt.connect(BoardID.c_str() , BROKER_USER , BROKER_PASS);
  while(!mqtt.connected()){
    Serial.print(".");
    delay(200);
  }  
  mqtt.subscribe("TOPIC_ILUM");
  mqtt.setCallback(callbacck);
  Serial.println("\nConectado ao Broker!");
}

void loop() {
  String mensagem = "Nome: Leonardo";
  mensagem == "oi";
  mqtt.publish("Topico-DSM14" , mensagem.c_str());
  mqtt.loop();
  delay(1000);
}

void callback(char* topic, byte* payload, unsigned int length){
 String msg = "";
 for(int i =0 i < length; i++){
  msg += (char) payload[i];
 }
 if(topic =="TOPIC_ILUM && msg == "Acender"){
  digitalWrite(2,HIGH);
 }else if(topic == TOPIC_ILUM && msg == "Apagar"){
  digitalWrite(2,Low);
 }
}

