// =============================================================================
// ColdChainID — ESP32 | SETOR: Expedição (Etapa 3 de 3)
// Arquitetura: Máquina de Estados Finitos (FSM)
//
// Sequência obrigatória: Produção → Câmara Fria → Expedição
// Este setor só aceita tags cuja etapa anterior é "Câmara Fria".
// Após registrar com sucesso, o ciclo desta tag é considerado completo.
//
// Estados da FSM:
//   AGUARDANDO_TAG      → fica em espera até o PN532 detectar uma tag
//   VALIDANDO_SEQUENCIA → consulta o cache e decide se a sequência está correta
//   SEQUENCIA_OK        → monta o JSON e prepara para publicação
//   SEQUENCIA_ERRO      → publica alerta e descarta o dado
//   PUBLICANDO          → criptografa, publica e marca o ciclo como completo
// =============================================================================

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_PN532.h>
#include "time.h"
#include "LittleFS.h"
#include <ArduinoJson.h>
#include "mbedtls/aes.h"
#include "base64.h"

// --- Configurações de Wi-Fi e MQTT ---
const char* ssid = "NOME_DO_SEU_WIFI";
const char* password = "SENHA_DO_WIFI";
const char* mqtt_server = "ENDERECO_DO_BROKER_HIVEMQ"; 
const char* mqtt_user = "USUARIO_MQTT"; 
const char* mqtt_pass = "SENHA_MQTT";

// --- Configurações de Tempo (NTP) ---
const char* ntpServer          = "pool.ntp.org";
const long  gmtOffset_sec      = -10800;
const int   daylightOffset_sec = 0;

// --- Pinos do PN532 ---
#define SDA_PIN     21
#define SCL_PIN     22
#define PN532_IRQ    2
#define PN532_RESET  3

// --- Identificador do Setor ---
const char* SETOR = "Expedição";

// --- Regra de sequência: etapa que deve ter ocorrido ANTES desta ---
const char* ETAPA_ANTERIOR_PERMITIDA = "Câmara Fria";

// --- Tópicos MQTT ---
const char* TOPICO_DADOS  = "coldchain/rfid";
const char* TOPICO_STATUS = "coldchain/status";
const char* TOPICO_ALERTA = "coldchain/alerta";

// --- Chaves AES-128 ---
const char* chave_aes = "XXXXXXXXXXXXXXX";
const char* iv_aes    = "XXXXXXXXXXXXXXX";

// =============================================================================
// DEFINIÇÃO DOS ESTADOS DA FSM
// =============================================================================
enum EstadoFSM {
  AGUARDANDO_TAG,
  VALIDANDO_SEQUENCIA,
  SEQUENCIA_OK,
  SEQUENCIA_ERRO,
  PUBLICANDO
};

EstadoFSM estadoAtual = AGUARDANDO_TAG;

// =============================================================================
// VARIÁVEIS DE CONTEXTO DA FSM
// =============================================================================
String tagUID            = "";
String tagTimestamp      = "";
String tagUltimoSetor    = "";
String jsonParaPublicar  = "";

// =============================================================================
// CACHE DE STATUS DAS TAGS
// =============================================================================
#define MAX_TAGS_CACHE 20
struct TagStatus {
  String uid;
  String ultimoSetor;
};
TagStatus cacheStatus[MAX_TAGS_CACHE];
int totalTagsCache = 0;

String getUltimoSetor(String uid) {
  for (int i = 0; i < totalTagsCache; i++) {
    if (cacheStatus[i].uid == uid) return cacheStatus[i].ultimoSetor;
  }
  return "nenhuma";
}

void setUltimoSetor(String uid, String setor) {
  for (int i = 0; i < totalTagsCache; i++) {
    if (cacheStatus[i].uid == uid) {
      cacheStatus[i].ultimoSetor = setor;
      return;
    }
  }
  if (totalTagsCache < MAX_TAGS_CACHE) {
    cacheStatus[totalTagsCache].uid = uid;
    cacheStatus[totalTagsCache].ultimoSetor = setor;
    totalTagsCache++;
  }
}

Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET);
WiFiClientSecure espClient;
PubSubClient client(espClient);

unsigned long lastReadTime = 0;
const long interval = 1000;
bool ntpSincronizado = false;
bool wifiEstavaDesconectado = false; // Bug 2: rastreia perda de WiFi para chamar processBuffer ao reconectar

// =============================================================================
// CALLBACK MQTT
// =============================================================================
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String topico   = String(topic);
  String mensagem = "";
  for (unsigned int i = 0; i < length; i++) mensagem += (char)payload[i];

  if (topico == TOPICO_STATUS) {
    StaticJsonDocument<200> doc;
    if (!deserializeJson(doc, mensagem)) {
      String uid      = doc["id"].as<String>();
      String ultSetor = doc["ultimo_setor"].as<String>();
      setUltimoSetor(uid, ultSetor);
      Serial.println("[STATUS] Cache atualizado: " + uid + " -> " + ultSetor);
    }
  }
}

// =============================================================================
// CRIPTOGRAFIA AES-128 CBC
// =============================================================================
String criptografar(String payload) {
  int record_len = payload.length();
  int pad_len    = 16 - (record_len % 16);
  int total_len  = record_len + pad_len;

  uint8_t input[total_len];
  uint8_t output[total_len];

  memcpy(input, payload.c_str(), record_len);
  for (int i = 0; i < pad_len; i++) input[record_len + i] = pad_len;

  mbedtls_aes_context aes;
  mbedtls_aes_init(&aes);
  mbedtls_aes_setkey_enc(&aes, (const unsigned char*)chave_aes, 128);

  uint8_t iv[16];
  memcpy(iv, iv_aes, 16);

  mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_ENCRYPT, total_len, iv, input, output);
  mbedtls_aes_free(&aes);

  return base64::encode(output, total_len);
}

bool verificarNTP() {
  struct tm timeinfo;
  if (getLocalTime(&timeinfo, 100)) {
    if (timeinfo.tm_year > 120) ntpSincronizado = true;
  }
  return ntpSincronizado;
}

String getTimestamp() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo, 100)) return "";
  char buf[30];
  strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
  return String(buf);
}

void saveToBuffer(String data) {
  File file = LittleFS.open("/buffer.txt", FILE_APPEND);
  if (file) { file.println(data); file.close(); }
}

void processBuffer() {
  if (!LittleFS.exists("/buffer.txt")) return;
  File fileRead = LittleFS.open("/buffer.txt", FILE_READ);
  if (!fileRead) return;

  String linhas[50];
  int total = 0;
  while (fileRead.available() && total < 50) {
    String linha = fileRead.readStringUntil('\n');
    linha.trim();
    if (linha.length() > 0) linhas[total++] = linha;
  }
  fileRead.close();

  if (total == 0) { LittleFS.remove("/buffer.txt"); return; }

  bool temFalha = false;
  File fileEscrita = LittleFS.open("/buffer_temp.txt", FILE_WRITE);

  for (int i = 0; i < total; i++) {
    String p = linhas[i];
    if (p.indexOf("PENDENTE_NTP") >= 0) {
      String hora = getTimestamp();
      if (hora.length() > 0) p.replace("PENDENTE_NTP", hora);
    }
    String enc = criptografar(p);
    if (!temFalha && client.connected() && client.publish(TOPICO_DADOS, enc.c_str(), true)) {
      Serial.println("Buffer sincronizado.");
      // Bug 1: atualiza status da tag apos publicar, igual ao fluxo normal
      StaticJsonDocument<200> docBuf;
      if (!deserializeJson(docBuf, p)) {
        String uidBuf = docBuf["id"].as<String>();
        String tsBuf  = docBuf["data_hora"].as<String>();
        publicarStatus(uidBuf, tsBuf);
      }
    } else {
      temFalha = true;
      if (fileEscrita) fileEscrita.println(p);
    }
  }
  if (fileEscrita) fileEscrita.close();
  LittleFS.remove("/buffer.txt");
  if (temFalha) LittleFS.rename("/buffer_temp.txt", "/buffer.txt");
  else LittleFS.remove("/buffer_temp.txt");
}

String buildJsonPayload(String uidStr, String timestamp) {
  StaticJsonDocument<200> doc;
  doc["id"]        = uidStr;
  doc["setor"]     = SETOR;
  doc["data_hora"] = timestamp;
  char buffer[200];
  serializeJson(doc, buffer);
  return String(buffer);
}

void publicarAlerta(String uidStr, String ultimoSetor, String timestamp) {
  StaticJsonDocument<300> doc;
  doc["id"]           = uidStr;
  doc["setor_atual"]  = SETOR;
  doc["ultimo_setor"] = ultimoSetor;
  doc["data_hora"]    = timestamp;
  doc["mensagem"]     = "ERRO: Sequencia incorreta! Tag lida fora de ordem.";
  char buffer[300];
  serializeJson(doc, buffer);
  client.publish(TOPICO_ALERTA, buffer, false);

  Serial.println("==========================================");
  Serial.println("  [ERRO] SEQUENCIA INCORRETA!");
  Serial.println("  TAG     : " + uidStr);
  Serial.println("  SETOR   : " + String(SETOR));
  Serial.println("  ULTIMO  : " + ultimoSetor);
  Serial.println("  ESPERADO: " + String(ETAPA_ANTERIOR_PERMITIDA));
  Serial.println("==========================================");
}

// Após Expedição, publica "Expedição" como último setor.
// O ciclo está completo — a tag pode ser reutilizada em nova produção
// somente depois que o status for manualmente resetado para "nenhuma".
void publicarStatus(String uidStr, String timestamp) {
  StaticJsonDocument<200> doc;
  doc["id"]           = uidStr;
  doc["ultimo_setor"] = SETOR;
  doc["data_hora"]    = timestamp;
  char buffer[200];
  serializeJson(doc, buffer);
  client.publish(TOPICO_STATUS, buffer, true);
  setUltimoSetor(uidStr, SETOR);
  Serial.println("[PUBLICANDO] Ciclo completo para a tag: " + uidStr);
}

void reconnect() {
  Serial.print("Conectando ao HiveMQ...");
  if (client.connect("ESP32_ColdChain_Expedicao", mqtt_user, mqtt_pass)) {
    Serial.println("Conectado!");
    client.subscribe(TOPICO_STATUS);
    // Bug 3: aguarda NTP sincronizar antes de processar o buffer,
    // para garantir que timestamps PENDENTE_NTP sejam substituidos corretamente
    for (int i = 0; i < 20 && !verificarNTP(); i++) delay(500);
    processBuffer();
  } else {
    Serial.println("Falha. Tentara em 5s.");
  }
}

// =============================================================================
// SETUP
// =============================================================================
void setup() {
  Serial.begin(115200);

  if (!LittleFS.begin(true)) Serial.println("Erro ao montar LittleFS");

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\nWi-Fi Conectado!");

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  Serial.print("Aguardando NTP");
  for (int i = 0; i < 10; i++) {
    if (verificarNTP()) { Serial.println("\nNTP Sincronizado!"); break; }
    delay(500); Serial.print(".");
  }
  if (!ntpSincronizado) Serial.println("\nNTP sem resposta no boot.");

  espClient.setInsecure();
  client.setServer(mqtt_server, 8883);
  client.setCallback(mqttCallback);

  Wire.begin(SDA_PIN, SCL_PIN);
  nfc.begin();
  if (!nfc.getFirmwareVersion()) { Serial.println("PN532 nao encontrado!"); while (1); }
  nfc.SAMConfig();

  estadoAtual = AGUARDANDO_TAG;
  Serial.println("=== SETOR: Expedicao (Etapa 3/3) | FSM iniciada ===");
  Serial.println("Estado inicial: AGUARDANDO_TAG");
}

// =============================================================================
// LOOP — Máquina de Estados Finitos (FSM)
//
// Diagrama de transições:
//
//        +------------------+
//        |  AGUARDANDO_TAG  | <─────────────────────────+
//        +------------------+                           |
//               | tag detectada                         |
//               v                                       |
//   +------------------------+                          |
//   |  VALIDANDO_SEQUENCIA   |                          |
//   +------------------------+                          |
//        |           |                                  |
//   seq. OK      seq. ERRADA                            |
//        |           |                                  |
//        v           v                                  |
//  +----------+  +----------------+                     |
//  |SEQUENCIA_|  | SEQUENCIA_ERRO |-----> AGUARDANDO ───+
//  |    OK    |  +----------------+
//  +----------+
//       |
//       v
//  +------------+
//  | PUBLICANDO |-----> AGUARDANDO_TAG ─────────────────+
//  +------------+    (ciclo completo)
// =============================================================================
void loop() {
  // Tarefas contínuas — rodam em TODOS os estados
  if (!client.connected()) {
    static unsigned long lastReconnect = 0;
    if (millis() - lastReconnect > 5000) {
      lastReconnect = millis();
      reconnect();
    }
  }
  client.loop();
  if (!ntpSincronizado) verificarNTP();

  // Bug 2: detecta reconexao silenciosa do WiFi e processa o buffer pendente
  if (WiFi.status() != WL_CONNECTED) {
    wifiEstavaDesconectado = true;
  } else if (wifiEstavaDesconectado && client.connected()) {
    wifiEstavaDesconectado = false;
    for (int i = 0; i < 20 && !verificarNTP(); i++) delay(500);
    processBuffer();
  }

  // ============================================================
  // FSM — switch principal
  // ============================================================
  switch (estadoAtual) {

    // ----------------------------------------------------------
    case AGUARDANDO_TAG:
    // Fica em escuta até o PN532 detectar uma tag.
    // Transição: → VALIDANDO_SEQUENCIA
    // ----------------------------------------------------------
    {
      uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };
      uint8_t uidLength;
      uint8_t success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 50);

      if (success && (millis() - lastReadTime > interval)) {
        lastReadTime = millis();

        tagUID = "";
        for (uint8_t i = 0; i < uidLength; i++) {
          if (uid[i] < 0x10) tagUID += "0";
          tagUID += String(uid[i], HEX);
        }

        tagTimestamp = getTimestamp();
        if (tagTimestamp.length() == 0) {
          tagTimestamp = "PENDENTE_NTP";
          Serial.println("Aviso: NTP nao sincronizado.");
        }

        Serial.println("\n[AGUARDANDO_TAG] Tag detectada: " + tagUID);
        estadoAtual = VALIDANDO_SEQUENCIA;
      }
      break;
    }

    // ----------------------------------------------------------
    case VALIDANDO_SEQUENCIA:
    // Verifica se a etapa anterior desta tag é a esperada.
    // Transição: → SEQUENCIA_OK ou → SEQUENCIA_ERRO
    // ----------------------------------------------------------
    {
      tagUltimoSetor = getUltimoSetor(tagUID);

      Serial.println("[VALIDANDO_SEQUENCIA] Ultimo setor: " + tagUltimoSetor);
      Serial.println("[VALIDANDO_SEQUENCIA] Esperado    : " + String(ETAPA_ANTERIOR_PERMITIDA));

      if (tagUltimoSetor == String(ETAPA_ANTERIOR_PERMITIDA)) {
        estadoAtual = SEQUENCIA_OK;
      } else {
        estadoAtual = SEQUENCIA_ERRO;
      }
      break;
    }

    // ----------------------------------------------------------
    case SEQUENCIA_OK:
    // Monta o JSON com os dados da leitura.
    // Transição: → PUBLICANDO
    // ----------------------------------------------------------
    {
      Serial.println("[SEQUENCIA_OK] Sequencia valida. Preparando dado...");
      jsonParaPublicar = buildJsonPayload(tagUID, tagTimestamp);
      estadoAtual = PUBLICANDO;
      break;
    }

    // ----------------------------------------------------------
    case SEQUENCIA_ERRO:
    // Publica alerta e descarta o dado.
    // Transição: → AGUARDANDO_TAG
    // ----------------------------------------------------------
    {
      Serial.println("[SEQUENCIA_ERRO] Sequencia invalida. Emitindo alerta...");
      publicarAlerta(tagUID, tagUltimoSetor, tagTimestamp);
      estadoAtual = AGUARDANDO_TAG;
      break;
    }

    // ----------------------------------------------------------
    case PUBLICANDO:
    // Criptografa e publica. Se offline/NTP pendente, salva no buffer.
    // Marca o ciclo da tag como completo (último setor = Expedição).
    // Transição: → AGUARDANDO_TAG
    // ----------------------------------------------------------
    {
      Serial.println("[PUBLICANDO] Enviando dado para o broker...");
      bool timestampValido = (tagTimestamp != "PENDENTE_NTP");

      if (timestampValido && WiFi.status() == WL_CONNECTED && client.connected()) {
        String jsonCriptografado = criptografar(jsonParaPublicar);
        if (client.publish(TOPICO_DADOS, jsonCriptografado.c_str(), true)) {
          Serial.println("[PUBLICANDO] Dado original:      " + jsonParaPublicar);
          Serial.println("[PUBLICANDO] Dado criptografado: " + jsonCriptografado);
          publicarStatus(tagUID, tagTimestamp); // Ciclo completo
        } else {
          Serial.println("[PUBLICANDO] Falha ao publicar. Salvando no buffer...");
          saveToBuffer(jsonParaPublicar);
        }
      } else {
        Serial.println("[PUBLICANDO] Offline/NTP pendente. Salvando no buffer...");
        saveToBuffer(jsonParaPublicar);
      }

      estadoAtual = AGUARDANDO_TAG;
      break;
    }

  } // fim do switch — FSM
}
