#include <WiFi.h>                // Biblioteca para conectar o ESP32 ao WiFi
#include <WiFiClientSecure.h>    // Permite conexão segura (TLS)
#include <PubSubClient.h>        // Biblioteca para usar o protocolo MQTT
#include "env.h"                 // Arquivo separado com senhas, usuário e URL do broker

WiFiClientSecure client;         // Cliente WiFi seguro
PubSubClient mqtt(client);       // Cliente MQTT usando o WiFi seguro

// --- Pinos do primeiro sensor ultrassônico ---
const byte TRIGGER_PIN = 19;
const byte ECHO_PIN = 13;

// --- Pinos do segundo sensor ultrassônico ---
const byte TRIGGER_PIN2 = 27;
const byte ECHO_PIN2 = 18;

// --- Pino do LED ---
const byte LED_PIN = 12;

unsigned long lastMsg = 0;       // Controle de tempo entre mensagens
bool objetoProximoAnterior = false; // Evita enviar mensagens repetidas

// =====================================================================
//  FUNÇÃO: CONEXÃO COM O WI-FI
// =====================================================================
void conectaWiFi() {
  Serial.println("Conectando ao WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASS);  // Inicia conexão com Wi-Fi

  // Fica tentando até conectar
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println("\nWiFi conectado!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());    // Mostra o IP obtido
}

// =====================================================================
//  FUNÇÃO: CONEXÃO COM O BROKER MQTT
// =====================================================================
void conectaMQTT() {
  while (!mqtt.connected()) {           // Repetição até conectar
    Serial.print("Conectando ao broker MQTT...");
    
    // Cria um ID aleatório para o ESP32
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);

    // Tenta conectar ao broker com usuário e senha
    if (mqtt.connect(clientId.c_str(), BROKER_USER, BROKER_PASS)) {
      Serial.println("Conectado!");
      mqtt.subscribe(TOPIC_ILUM); // Assina o tópico para receber comandos do LED
    } else {
      Serial.print("Falha (rc=");
      Serial.print(mqtt.state());
      Serial.println("), tentando novamente em 5s");
      delay(5000);
    }
  }
}

// =====================================================================
//  FUNÇÃO: LER DISTÂNCIA DO SENSOR ULTRASSÔNICO
// =====================================================================
long lerDistancia(byte triggerPin, byte echoPin) {
  digitalWrite(triggerPin, LOW);
  delayMicroseconds(2);

  digitalWrite(triggerPin, HIGH);  // Pulso de 10 microsegundos
  delayMicroseconds(10);
  digitalWrite(triggerPin, LOW);

  // Mede o tempo que o som levou para voltar
  long duracao = pulseIn(echoPin, HIGH, 20000);

  if (duracao == 0) return -1; // Se não houve retorno, erro de leitura

  // Conversão para centímetros
  return (long) duracao * 0.034 / 2;
}

// =====================================================================
//  FUNÇÃO: RECEBER MENSAGENS MQTT (EX: "Acender" / "Apagar")
// =====================================================================
void callback(char* topic, byte* payload, unsigned int length) {
  String msg = "";
  
  // Converte o payload (bytes) para texto
  for (int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }

  Serial.print("Mensagem recebida em ");
  Serial.print(topic);
  Serial.print(": ");
  Serial.println(msg);

  // Se chegou comando no tópico do LED
  if (String(topic) == TOPIC_ILUM) {
    if (msg == "Acender") {
      digitalWrite(LED_PIN, HIGH);   // Liga o LED
    } else if (msg == "Apagar") {
      digitalWrite(LED_PIN, LOW);    // Desliga o LED
    }
  }
}

// =====================================================================
//  SETUP - Executa uma vez ao iniciar
// =====================================================================
void setup() {
  Serial.begin(115200);     // Inicia comunicação serial

  // Configura pinos dos sensores
  pinMode(TRIGGER_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  pinMode(TRIGGER_PIN2, OUTPUT);
  pinMode(ECHO_PIN2, INPUT);

  pinMode(LED_PIN, OUTPUT); // Configura pino do LED

  client.setInsecure();     // Aceita conexão MQTT sem certificado
  conectaWiFi();            // Conecta ao Wi-Fi

  mqtt.setServer(BROKER_URL, BROKER_PORT); // Configura broker MQTT
  mqtt.setCallback(callback);              // Define função que trata mensagens recebidas
  conectaMQTT();                           // Conecta ao MQTT
}

// =====================================================================
//  LOOP PRINCIPAL - executa repetidamente
// =====================================================================
void loop() {
  if (!mqtt.connected()) {  // Se cair a conexão, tenta reconectar
    conectaMQTT();
  }
  mqtt.loop();  // Mantém a comunicação MQTT ativa

  unsigned long agora = millis();

  // Executa a cada 2 segundos
  if (agora - lastMsg > 2000) {
    lastMsg = agora;

    // Lê os dois sensores
    long distancia1 = lerDistancia(TRIGGER_PIN, ECHO_PIN);
    long distancia2 = lerDistancia(TRIGGER_PIN2, ECHO_PIN2);

    // Mostra no monitor serial
    Serial.print("Sensor 1: ");
    Serial.print(distancia1);
    Serial.println(" cm");

    Serial.print("Sensor 2: ");
    Serial.print(distancia2);
    Serial.println(" cm");

    // --- Lógica de detecção de presença ---
    if (distancia1 > 0 && distancia1 < 10) {
      if (!objetoProximoAnterior) {
        mqtt.publish(TOPIC_ILUM, "Objeto proximo no sensor 1");
        objetoProximoAnterior = true; // Impede mensagens repetidas
      }
    } else if (distancia2 > 0 && distancia2 < 10) {
      if (!objetoProximoAnterior) {
        mqtt.publish(TOPIC_ILUM, "Objeto proximo no sensor 2");
        objetoProximoAnterior = true;
      }
    } else {
      objetoProximoAnterior = false; // Reseta se não há objeto
    }
  }
}
