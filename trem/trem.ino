#include <WiFi.h>              // Biblioteca para conexão WiFi
#include <PubSubClient.h>      // Biblioteca para MQTT
#include "env.h"               // Arquivo com constantes (SSID, senha, broker etc.)

WiFiClient client;             // Cria cliente WiFi
PubSubClient mqtt(client);     // Cliente MQTT usando o WiFiClient


void setup() {
  Serial.begin(115200);
  Serial.println("Conectando no WiFi");

  // Inicia a conexão WiFi com SSID e senha definidos em env.h
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  // Aguarda conexão
  while (WiFi.status() != WL_CONNECTED) {
    Serial.println(".");       // Mostra barra de progresso no monitor serial
    delay(200);
  }
  Serial.println("\nConectado com Sucesso no WiFi!");

  // --- Conexão com Broker MQTT ---
  Serial.println("Conectando ao Broker...");
  mqtt.setServer(BROKER_URL, BROKER_PORT);  // Define endereço e porta do broker

  // Cria um ID aleatório para o dispositivo
  String BoardID = "Trem";
  BoardID += String(random(0xffff), HEX);

  // Tenta conectar ao broker com usuário e senha definidos em env.h
  mqtt.connect(BoardID.c_str(), BROKER_USER, BROKER_PASS);

  // Aguarda conexão
  while (!mqtt.connected()) {
    Serial.print(".");
    delay(200);
  }

  mqtt.subscribe(TOPIC_ILUM);      // Inscreve-se no tópico de iluminação
  mqtt.setCallback(callback);      // Define função que será chamada ao receber mensagens

  Serial.println("\nConectado ao Broker!");

  pinMode(2, OUTPUT);              // Configura o pino do LED (ex.: D2)
}

void loop() {
  // Publica mensagem no tópico
  mqtt.publish(TOPIC_ILUM, "Acender");

  mqtt.loop();       // Mantém a conexão com o broker ativa
  delay(1000);       // Publica a cada 1 segundo
}


// --- Função acionada sempre que uma mensagem MQTT é recebida ---
void callback(char* topic, byte* payload, unsigned int length) {
  String msg = "";

  // Converte o payload recebido em String
  for (int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }

  // Exibe dados recebidos
  Serial.print("Tópico: ");
  Serial.println(topic);
  Serial.print("Mensagem: ");
  Serial.println(msg);

  // Verifica se o tópico e a mensagem correspondem
  if (String(topic) == TOPIC_ILUM && msg == "Acender") {
    digitalWrite(2, HIGH);     // Liga o LED
  }
  else if (String(topic) == TOPIC_ILUM && msg == "Apagar") {
    digitalWrite(2, LOW);      // Desliga o LED
  }
}
