#include <WiFi.h>                
#include <WiFiClientSecure.h>    
#include <PubSubClient.h>        
#include "env.h"                 // Dados do WiFi e MQTT

WiFiClientSecure client;
PubSubClient mqtt(client);

// --- Sensores ultrassônicos ---
const byte TRIGGER_PIN = 19;
const byte ECHO_PIN = 13;

const byte TRIGGER_PIN2 = 27;
const byte ECHO_PIN2 = 18;

// --- LED ---
const byte LED_PIN = 12;

// Controle de mensagens
unsigned long lastMsg = 0;
bool objetoProximoAnterior = false;

// =====================================================================
//  CONEXÃO COM O WI-FI
// =====================================================================
void conectaWiFi() {
  Serial.println("Conectando ao WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  // Aguarda conectar
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println("\nWiFi conectado!");
  Serial.println(WiFi.localIP());     // Mostra o IP recebido
}

// =====================================================================
//  CONEXÃO COM O BROKER MQTT
// =====================================================================
void conectaMQTT() {
  while (!mqtt.connected()) {
    Serial.print("Conectando ao broker... ");

    // Cria ID único pro ESP32
    String clientId = "ESP32-" + String(random(0xffff), HEX);

    // Tenta conectar usando usuário e senha
    if (mqtt.connect(clientId.c_str(), BROKER_USER, BROKER_PASS)) {
      Serial.println("Conectado!");
      mqtt.subscribe(TOPIC_ILUM);   // Recebe comandos do LED
    } else {
      Serial.print("Falhou (rc=");
      Serial.print(mqtt.state());
      Serial.println("). Tentando...");
      delay(5000);
    }
  }
}

// =====================================================================
//  FUNÇÃO PARA LER DISTÂNCIA DOS SENSORES ULTRASSÔNICOS
// =====================================================================
long lerDistancia(byte triggerPin, byte echoPin) {
  // Pulso de disparo
  digitalWrite(triggerPin, LOW);
  delayMicroseconds(2);
  digitalWrite(triggerPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(triggerPin, LOW);

  // Mede retorno do som
  long duracao = pulseIn(echoPin, HIGH, 20000);
  if (duracao == 0) return -1;   // Erro de leitura

  return duracao * 0.034 / 2;    // Converte em centímetros
}

// =====================================================================
//  RECEBIMENTO DE MENSAGENS MQTT (CONTROLE DO LED)
// =====================================================================
void callback(char* topic, byte* payload, unsigned int length) {
  String msg = "";

  for (int i = 0; i < length; i++) msg += (char)payload[i];

  Serial.print("Mensagem recebida: ");
  Serial.println(msg);

  // Comandos de LED
  if (msg == "Acender") digitalWrite(LED_PIN, HIGH);
  else if (msg == "Apagar") digitalWrite(LED_PIN, LOW);
}

// =====================================================================
//  SETUP INICIAL
// =====================================================================
void setup() {
  Serial.begin(115200);

  // Configura sensores e LED
  pinMode(TRIGGER_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  pinMode(TRIGGER_PIN2, OUTPUT);
  pinMode(ECHO_PIN2, INPUT);

  pinMode(LED_PIN, OUTPUT);

  client.setInsecure();     // Conexão MQTT sem certificado
  conectaWiFi();            // Conecta WiFi

  mqtt.setServer(BROKER_URL, BROKER_PORT);
  mqtt.setCallback(callback);
  conectaMQTT();            // Conecta MQTT
}

// =====================================================================
//  LOOP PRINCIPAL
// =====================================================================
void loop() {
  if (!mqtt.connected()) conectaMQTT();
  mqtt.loop();   // Mantém comunicação MQTT ativa

  // Executa a cada 2 segundos
  if (millis() - lastMsg > 2000) {
    lastMsg = millis();

    long distancia1 = lerDistancia(TRIGGER_PIN, ECHO_PIN);
    long distancia2 = lerDistancia(TRIGGER_PIN2, ECHO_PIN2);

    Serial.print("Sensor 1: ");
    Serial.println(distancia1);

    Serial.print("Sensor 2: ");
    Serial.println(distancia2);

    // --- Detecção de objeto próximo ---
    if (distancia1 > 0 && distancia1 < 10) {
      if (!objetoProximoAnterior) {
        mqtt.publish(TOPIC_ILUM, "Objeto proximo no sensor 1");
        objetoProximoAnterior = true;
      }
    }

    else if (distancia2 > 0 && distancia2 < 10) {
      if (!objetoProximoAnterior) {
        mqtt.publish(TOPIC_ILUM, "Objeto proximo no sensor 2");
        objetoProximoAnterior = true;
      }
    }

    else {
      objetoProximoAnterior = false;   // Reseta estado
    }
  }
}
