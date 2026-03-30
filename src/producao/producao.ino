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

// --- Pinos do PN532 ---
#define SDA_PIN 21
#define SCL_PIN 22
#define PN532_IRQ   2
#define PN532_RESET 3

Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET);
WiFiClientSecure espClient;
PubSubClient client(espClient);

unsigned long lastReadTime = 0;
const long interval = 1000; 

// --- Funções do LittleFS (Nova Adição) ---
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
      if (client.publish("coldchain/rfid", pendingData.c_str())) {
        Serial.println("Sincronizado: " + pendingData);
      } else {
        file.close();
        return; // Para se a conexão cair de novo
      }
    }
  }
  file.close();
  LittleFS.remove("/buffer.txt"); // Limpa o buffer após sucesso
}

String getTimestamp() {
  struct tm timeinfo;
  // O timeout de 100ms evita que o código "congele" tentando buscar a hora offline
  if(!getLocalTime(&timeinfo, 100)){ 
    return "0000-00-00 00:00:00 (Offline)";
  }
  char timeStringBuff[30];
  strftime(timeStringBuff, sizeof(timeStringBuff), "%Y-%m-%d %H:%M:%S", &timeinfo);
  return String(timeStringBuff);
}

void setup() {
  Serial.begin(115200);
  
  // Inicializa LittleFS
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
  if (client.connect("ESP32_ColdChain_Produção", mqtt_user, mqtt_pass)) {
    Serial.println("Conectado!");
    processBuffer(); // Tenta enviar dados salvos ao reconectar
  } else {
    Serial.println("Falha na conexão. Tentará novamente em 5s.");
  }
}

void loop() {
  // Verifica conexão de forma não-bloqueante
  if (!client.connected()) {
    static unsigned long lastReconnectAttempt = 0;
    if (millis() - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = millis();
      reconnect(); // Tenta reconectar mas sai logo em seguida
    }
  } else {
    client.loop();
  }

  uint8_t success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };
  uint8_t uidLength;

  // Timeout de 50ms para permitir leitura offline sem travar
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
    jsonPayload += "\"setor\":\"Produção\",";
    jsonPayload += "\"data_hora\":\"" + currentTime + "\"";
    jsonPayload += "}";
    
    // MUDANÇA SOLICITADA: Lógica estrita para diferenciar Online de Offline
   if(WiFi.status() == WL_CONNECTED && client.connected() && client.publish("coldchain/rfid", jsonPayload.c_str())) {
      Serial.println("Enviado Online: " + jsonPayload);
    } else {
      // Se o Wi-Fi desconectar fisicamente, ele entra aqui instantaneamente
      Serial.println("MQTT Offline. Salvando no Flash (LittleFS)...");
      saveToBuffer(jsonPayload);
    }
  }
}
