#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_PN532.h>
#include "time.h"
#include "LittleFS.h"
#include "config.h" 


extern const char* ssid;
extern const char* password;
extern const char* mqtt_server;
extern const char* mqtt_user;
extern const char* mqtt_pass;

// --- Tópicos e Mensagens de Status ---
const char* topic_status = "coldchain/status";
const char* msg_online = "ONLINE";
const char* msg_offline = "OFFLINE";

// --- Configurações de Tempo (NTP) ---
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = -10800;
const int   daylightOffset_sec = 0; 

#define SDA_PIN 21
#define SCL_PIN 22
#define PN532_IRQ   2
#define PN532_RESET 3

Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET);
WiFiClientSecure espClient;
PubSubClient client(espClient);

unsigned long lastReadTime = 0;
const long interval = 1000;

void saveToBuffer(String data) {
  File file = LittleFS.open("/buffer.txt", FILE_APPEND);
  if (file) {
    file.println(data);
    file.close();
    Serial.println("Dados salvos no Buffer Offline.");
  }
}

void processBuffer() {
  if (!LittleFS.exists("/buffer.txt")) return;
  File file = LittleFS.open("/buffer.txt", FILE_READ);
  if (!file) return;

  Serial.println("Sincronizando Buffer...");
  String pendingData;
  while (file.available()) {
    pendingData = file.readStringUntil('\n');
    pendingData.trim();
    if (pendingData.length() > 0) {
      // QoS 1 aplicado para garantir entrega industrial [cite: 42, 69]
      if (client.publish("coldchain/rfid", pendingData.c_str(), true)) { 
        Serial.println("Sincronizado: " + pendingData);
      } else {
        file.close();
        return;
      }
    }
  }
  file.close();
  LittleFS.remove("/buffer.txt");
}

String getTimestamp() {
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo, 100)){ 
    return "offline_sync"; 
  }
  char timeStringBuff[30];
  strftime(timeStringBuff, sizeof(timeStringBuff), "%Y-%m-%d %H:%M:%S", &timeinfo);
  return String(timeStringBuff);
}

void setup() {
  Serial.begin(115200);
  if(!LittleFS.begin(true)){
    Serial.println("Erro ao montar LittleFS");
  }

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi Conectado!");

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  espClient.setInsecure();
  client.setServer(mqtt_server, 8883);
  Wire.begin(SDA_PIN, SCL_PIN);
  nfc.begin();
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    Serial.println("PN532 não encontrado!");
    while (1);
  }
  nfc.SAMConfig();
  Serial.println("Aguardando TAG...");
}

void reconnect() {
  Serial.print("Conectando ao HiveMQ...");
  // LWT configurado com QoS 1 para avisar se o dispositivo cair [cite: 74, 75]
  if (client.connect("ESP32_ColdChain_Camara_Fria", mqtt_user, mqtt_pass, topic_status, 1, true, msg_offline)) {
    Serial.println("Conectado!");
    client.publish(topic_status, msg_online, true); 
    processBuffer();
  } else {
    Serial.println("Falha na conexão. Tentará novamente em 5s.");
  }
}

void loop() {
  if (!client.connected()) {
    static unsigned long lastReconnectAttempt = 0;
    if (millis() - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = millis();
      reconnect();
    }
  } else {
    client.loop();
  }

  uint8_t success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };
  uint8_t uidLength;
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 50);

  if (success && (millis() - lastReadTime > interval)) {
    lastReadTime = millis(); 

    String uidStr = "";
    for (uint8_t i = 0; i < uidLength; i++) {
      if (uid[i] < 0x10) uidStr += "0";
      uidStr += String(uid[i], HEX);
    }

    String currentTime = getTimestamp();

    String jsonPayload = "{";
    jsonPayload += "\"id\":\"" + uidStr + "\",";
    jsonPayload += "\"setor\":\"Câmara Fria\",";
    jsonPayload += "\"data_hora\":\"" + currentTime + "\"";
    jsonPayload += "}";
    
    // Publicação com QoS 1 para evitar perda de dados no inventário [cite: 68, 69]
    if(WiFi.status() == WL_CONNECTED && client.connected() && client.publish("coldchain/rfid", jsonPayload.c_str(), true)) {
      Serial.println("Enviado Online (QoS 1): " + jsonPayload);
    } else {
      Serial.println("Falha no envio ou Offline. Salvando no Flash...");
      saveToBuffer(jsonPayload);
    }
  }
}
