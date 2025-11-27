#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "env.h"

// --- OBJETOS ---
WiFiClientSecure wifiSecure;
PubSubClient mqttClient(wifiSecure);

// --- Pinos dos sensores ---
const byte PIN_TRIG_A = 19;
const byte PIN_ECHO_A = 13;

const byte PIN_TRIG_B = 27;
const byte PIN_ECHO_B = 18;

// --- LED ---
const byte PIN_LED_STATUS = 12;

// --- Controle ---
unsigned long ultimoEnvio = 0;
bool detectouAnterior = false;

// =====================================================================
//  LEITURA DO SENSOR ULTRASSÔNICO
// =====================================================================
long medirCM(byte trig, byte echo) {
  digitalWrite(trig, LOW);
  delayMicroseconds(3);

  digitalWrite(trig, HIGH);
  delayMicroseconds(12);
  digitalWrite(trig, LOW);

  long tempo = pulseIn(echo, HIGH, 20000);
  if (tempo <= 0) return -1;

  return tempo * 0.034 / 2;
}

// =====================================================================
//  CALLBACK MQTT (responde quando chega mensagem)
// =====================================================================
void mqttCallback(char* topico, byte* mensagem, unsigned int tam) {
  String conteudo = "";
  for (int i = 0; i < tam; i++) conteudo += (char)mensagem[i];

  Serial.print("Recebido: ");
  Serial.println(conteudo);

  if (conteudo == "Acender") {
    digitalWrite(PIN_LED_STATUS, HIGH);
  } 
  else if (conteudo == "Apagar") {
    digitalWrite(PIN_LED_STATUS, LOW);
  }
}

// =====================================================================
//  CONEXÃO MQTT
// =====================================================================
void iniciarMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("Conectando ao MQTT... ");
    String id = "DISP-" + String(random(0xFFFF), HEX);

    if (mqttClient.connect(id.c_str(), BROKER_USER, BROKER_PASS)) {
      Serial.println("OK");
      mqttClient.subscribe(TOPIC_ILUM);
    } else {
      Serial.print("Falhou (");
      Serial.print(mqttClient.state());
      Serial.println("). Tentando...");
      delay(3000);
    }
  }
}

// =====================================================================
//  CONEXÃO WI-FI
// =====================================================================
void iniciarWiFi() {
  Serial.println("Conectando WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("#");
    delay(400);
  }

  Serial.println("\nConectado!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

// =====================================================================
//  SETUP
// =====================================================================
void setup() {
  Serial.begin(115200);

  // Pinos dos sensores
  pinMode(PIN_TRIG_A, OUTPUT);
  pinMode(PIN_ECHO_A, INPUT);

  pinMode(PIN_TRIG_B, OUTPUT);
  pinMode(PIN_ECHO_B, INPUT);

  // LED
  pinMode(PIN_LED_STATUS, OUTPUT);

  wifiSecure.setInsecure();    // Aceita MQTT sem certificado
  iniciarWiFi();

  mqttClient.setServer(BROKER_URL, BROKER_PORT);
  mqttClient.setCallback(mqttCallback);
  iniciarMQTT();
}

// =====================================================================
//  LOOP PRINCIPAL
// =====================================================================
void loop() {

  if (!mqttClient.connected()) iniciarMQTT();
  mqttClient.loop();

  unsigned long atual = millis();

  if (atual - ultimoEnvio >= 2000) {
    ultimoEnvio = atual;

    long distA = medirCM(PIN_TRIG_A, PIN_ECHO_A);
    long distB = medirCM(PIN_TRIG_B, PIN_ECHO_B);

    Serial.print("A: "); Serial.println(distA);
    Serial.print("B: "); Serial.println(distB);

    bool pertoA = (distA > 0 && distA < 10);
    bool pertoB = (distB > 0 && distB < 10);

    if (pertoA && !detectouAnterior) {
      mqttClient.publish(TOPIC_ILUM, "Objeto proximo no sensor A");
      detectouAnterior = true;
    } 
    else if (pertoB && !detectouAnterior) {
      mqttClient.publish(TOPIC_ILUM, "Objeto proximo no sensor B");
      detectouAnterior = true;
    } 
    else if (!pertoA && !pertoB) {
      detectouAnterior = false;
    }
  }
}
