#include <WiFi.h>
#include <PubSubClient.h>

WiFiClient client;
PubSubClient mqtt(client);

const String BrokerURL = "test.mosquitto.org";
const int BrokerPort = 1883;

const String SSID = "FIESC_IOT_EDU";
const String PASS = "8120gv08";
const String BrokerUser = "";
const String BrokerPass = "";

void setup() {
  Serial.begin(115200);
  Serial.println("Conectando ao WiFi");
  WiFi.begin(SSID,PASS);
  while(WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(200);
  }
  Serial.println("\nConectado com Sucesso");
  
  Serial.println("conectando ao Broker...");
  mqtt.setServer(BrokerURL.c_str(),BrokerPort);
  String BoardID = "S1";
  BoardID += String(random(0xffff),HEX);
  mqtt.connect(BoardID.c_str(),BrokerUser.c_str(),BrokerPass.c_str());
  while(!mqtt.connected()){
    Serial.print(".");
    delay(200);

  }
}


void loop() {


}