#include <WiFi.h>               // Biblioteca para conectar o ESP32 ao Wi-Fi
#include <WiFiClientSecure.h>  // Permite conexões seguras
#include <PubSubClient.h>      // Biblioteca do protocolo MQTT
#include "env.h"               // Arquivo com Wi-Fi, Senha, Broker e Tópicos
#include <ESP32Servo.h>        // Biblioteca para controlar servos no ESP32

WiFiClientSecure client;       // Cliente de conexão segura
PubSubClient mqtt(client);     // Cliente MQTT usando o cliente seguro

// Pinos dos servomotores
const byte SERVO_PIN = 18;
Servo Servo1;

const byte SERVO_PIN2 = 19;
Servo Servo2;

// Pinos do sensor ultrassônico
const byte TRIGGER_PIN = 25;
const byte ECHO_PIN = 26;

void setup() {
  Serial.begin(115200);  // Inicia comunicação serial (para ver mensagens no PC)

  // Conecta os servos aos pinos definidos
  Servo1.attach(SERVO_PIN);
  Servo2.attach(SERVO_PIN2);

  // Configuração dos pinos do sensor ultrassônico
  pinMode(TRIGGER_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  client.setInsecure();  // Permite conexão sem certificado (útil para testes)

  // ---------------- CONEXÃO AO WI-FI ----------------
  Serial.println("Conectando ao WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASS); // Conecta usando dados do env.h

  while (WiFi.status() != WL_CONNECTED) { // Espera conectar
    Serial.print(".");
    delay(200);
  }
  Serial.print("\nConectado com Sucesso!");

  // ---------------- CONEXÃO AO BROKER MQTT ----------------
  Serial.println("Conectando ao Broker...");
  mqtt.setServer(BROKER_URL, BROKER_PORT); // Configura o endereço do broker
  mqtt.setCallback(callback);               // Define a função que trata mensagens recebidas

  // Cria um ID único para o ESP32 no MQTT
  String BoardID = "S2";
  BoardID += String(random(0xffff, HEX));

  mqtt.connect(BoardID.c_str(), BROKER_USER, BROKER_PASS); // Conecta ao MQTT
  
  while (!mqtt.connected()) { // Espera até conectar
    Serial.println(".");
    delay(200);
  }
  
  Serial.println("\nConectado ao Broker!");

  // Assina os tópicos para receber comandos
  mqtt.subscribe(TOPIC_ILUM);
  mqtt.subscribe(TOPIC_PRESENCA1);
  mqtt.subscribe(TOPIC_PRESENCA2);
  mqtt.subscribe(TOPIC_PRESENCA3);
}

// ---------------- FUNÇÃO PARA MEDIR DISTÂNCIA COM O SENSOR ----------------
long lerDistancia() {
  // Envia pulso de disparo (trigger)
  digitalWrite(TRIGGER_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIGGER_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGGER_PIN, LOW);

  // Mede o tempo que o sinal leva para ir e voltar
  long duracao = pulseIn(ECHO_PIN, HIGH);

  // Converte o tempo em centímetros (velocidade do som)
  long distancia = duracao * 349.24 / 2 / 10000;

  return distancia;
}

void loop() {

  mqtt.loop(); // Mantém a conexão MQTT ativa e verifica mensagens

  long distancia = lerDistancia(); // Lê a distância atual do sensor

  Serial.print("Distância: ");
  Serial.print(distancia);
  Serial.println(" cm");

  // Se um objeto estiver muito perto (<10 cm), envia alerta
  if (distancia < 10) {
    Serial.println("Objeto próximo!");
    mqtt.publish(TOPIC_DISTANCIA, "Objeto próximo!");
  }

  delay(1000); // Aguarda 1 segundo antes da próxima leitura
}

// ---------------- FUNÇÃO QUE TRATA MENSAGENS RECEBIDAS PELO MQTT ----------------
void callback(char* topic, byte* payload, unsigned int length) {
  String msg = "";

  // Constrói a mensagem recebida (que vem em bytes)
  for (int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }

  Serial.print("TOPICO: "); Serial.println(topic);
  Serial.print("MSG: "); Serial.println(msg);

  // ------ CONTROLE DE ILUMINAÇÃO ------
  if (strcmp(topic, TOPIC_ILUM) == 0) {
    if (msg == "ligar")    digitalWrite(2, HIGH); // Liga o pino 2
    if (msg == "desligar") digitalWrite(2, LOW);  // Desliga o pino 2
  }

  // ------ CONTROLE DO SERVO 1 ------
  else if (strcmp(topic, TOPIC_PRESENCA1) == 0) {
    if (msg == "servo1") {
      Servo1.write(0); // Move o servo 1 para 0°
      Serial.println("Servo1 → posição 1 (0°)");
    }
  }

  // ------ CONTROLE DO SERVO 2 ------
  else if (strcmp(topic, TOPIC_PRESENCA2) == 0) {
    if (msg == "servo2") {
      Servo2.write(0); // Move o servo 2 para 0°
      Serial.println("Servo2 → posição 1 (0°)");
    }
  }

  // ------ ABRIR AMBOS OS SERVOS ------
  else if (strcmp(topic, TOPIC_PRESENCA3) == 0) {
    if (msg == "abrir") {
      Servo1.write(90); // Move o servo 1 para 90°
      Servo2.write(90); // Move o servo 2 para 90°
      Serial.println("Servo1 e Servo2 → posição 2 (90°)");
    }
  }
}
