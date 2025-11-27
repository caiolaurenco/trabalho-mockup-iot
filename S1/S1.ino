#include <WiFi.h>                 // Biblioteca para usar WiFi no ESP32
#include <WiFiClientSecure.h>     // Permite conexão segura (HTTPS / TLS)
#include <PubSubClient.h>         // Biblioteca para comunicação MQTT
#include "env.h"                  // Arquivo com credenciais (WiFi/MQTT)
#include <DHT.h>                  // Biblioteca do sensor DHT

#define DHTPIN 4                  // Pino do sensor DHT11
#define DHTTYPE DHT11             // Define o tipo do sensor

WiFiClientSecure client;          // Cliente WiFi seguro
PubSubClient mqtt(client);        // Cliente MQTT usando o cliente seguro
DHT dht(DHTPIN, DHTTYPE);         // Objeto do sensor DHT

const byte ldr = 34;              // Pino analógico onde o LDR está conectado
int threshold = 0;                // Valor de referência para "claro/escuro"
const byte TRIGGER_PIN = 22;      // Pino trigger do sensor ultrassônico
const byte ECHO_PIN = 23;         // Pino echo do sensor ultrassônico

void setup() {
  Serial.begin(115200);           // Inicializa o monitor serial
  pinMode(ldr, INPUT);            // LDR como entrada
  pinMode(19, OUTPUT);            // LED no pino 19
  pinMode(TRIGGER_PIN, OUTPUT);   // Trigger do ultrassônico como saída
  pinMode(ECHO_PIN, INPUT);       // Echo como entrada
  dht.begin();                    // Inicia o sensor DHT11
  threshold = analogRead(ldr);    // Lê luz ambiente para definir threshold
  client.setInsecure();           // Permite conexão TLS sem certificado
  Serial.println("Conectando no WiFi");

  WiFi.begin(WIFI_SSID, WIFI_PASS);  // Tenta conectar ao WiFi
  while (WiFi.status() != WL_CONNECTED) {
    Serial.println(".");
    delay(200);
  }
  Serial.println("\nConectado com Sucesso no WiFi!");

  Serial.println("Conectando ao Broker...");
  mqtt.setServer(BROKER_URL, BROKER_PORT);   // Configura o servidor MQTT

  String BoardID = "S1";                     // ID base para o cliente
  BoardID += String(random(0xffff), HEX);    // Adiciona ID aleatório

  mqtt.connect(BoardID.c_str(), BROKER_USER, BROKER_PASS);  // Conecta ao broker
  while (!mqtt.connected()) {
    Serial.print(".");
    delay(200);
  }

  mqtt.subscribe(TOPIC_ILUM);               // Inscreve no tópico de iluminação
  mqtt.setCallback(callback);               // Define a função que trata mensagens recebidas
  Serial.println("\nConectado ao Broker!");
  Serial.println(threshold);                // Exibe o threshold definido
}

long lerDistancia() {
  digitalWrite(TRIGGER_PIN, LOW);           // Limpa trigger
  delayMicroseconds(2);
  digitalWrite(TRIGGER_PIN, HIGH);          // Pulso de 10us
  delayMicroseconds(10);
  digitalWrite(TRIGGER_PIN, LOW);
  
  long duracao = pulseIn(ECHO_PIN, HIGH);   // Mede o tempo do eco
  long distancia = duracao * 349.24 / 2 / 10000; // Converte tempo em distância (cm)
  
  return distancia;                          // Retorna distância
}

void loop() {
  long distancia = lerDistancia();           // Lê sensor ultrassônico
  int valor_ldr = analogRead(ldr);           // Lê luminosidade do LDR
  float umidade = dht.readHumidity();        // Lê umidade do DHT11
  float temp = dht.readTemperature();        // Lê temperatura do DHT11

  // Imprime dados no monitor serial
  Serial.printf("Temperatura: %.2f | Distância: %d  | Umidade: %.2f\n", temp , distancia , umidade);

  // Envia mensagem baseado na luminosidade
  if (valor_ldr > threshold) {
    mqtt.publish(TOPIC_ILUM, "Escuro");      // Envia "Escuro" ao broker
  } else {
    mqtt.publish(TOPIC_ILUM, "Claro");       // Envia "Claro" ao broker
  }

  // Publica dados dos sensores
  mqtt.publish("s1/distancia", String(distancia).c_str());
  mqtt.publish("s1/temperatura", String(temp).c_str());
  mqtt.publish("s1/umidade", String(umidade).c_str());
  
  delay(1000);                               // Espera 1 segundo
  mqtt.loop();                                // Mantém conexão MQTT ativa
}

void callback(char* topic, byte* payload, unsigned int length) {
  String msg = "";
  for (int i = 0; i < length; i++) {          // Converte payload em string
    msg += (char)payload[i];
  }
  msg = msg.c_str();

  // Se o tópico recebido for de iluminação...
  if (strcmp(topic, TOPIC_ILUM) == 0 && msg == "Claro") {  
    Serial.println(msg);
    digitalWrite(19, LOW);                   // Apaga LED
  } else if (strcmp(topic, TOPIC_ILUM) == 0 && msg == "Escuro") {
    digitalWrite(19, HIGH);                  // Acende LED
    Serial.println(msg);
  }
}
