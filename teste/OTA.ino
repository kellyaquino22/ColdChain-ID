// =============================================================================
// FUNÇÃO DE CONFIGURAÇÃO OTA
// =============================================================================
void setupOTA() {
  ArduinoOTA.setHostname(hostName);    // Nome que aparecerá no seu Arduino IDE
  ArduinoOTA.setPassword("admin123");  // Segurança: senha para autorizar o upload
  
  ArduinoOTA.onStart([]() {
    Serial.println("Iniciando atualização OTA...");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nAtualização concluída!");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progresso: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Erro [%u]: ", error);
  });
  ArduinoOTA.begin();
}