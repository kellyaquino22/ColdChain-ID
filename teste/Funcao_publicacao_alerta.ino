void publicarAlerta(String uidStr, String ultimoSetor, String timestamp) {
  StaticJsonDocument<300> doc;
  doc["id"]           = uidStr;
  doc["setor_atual"]  = SETOR;
  doc["ultimo_setor"] = ultimoSetor;
  doc["data_hora"]    = timestamp;
  doc["mensagem"]     = "ERRO: Sequencia incorreta! Tag lida fora de ordem."; // Mensagem de auditoria 

  char buffer[300];
  serializeJson(doc, buffer);
  client.publish(TOPICO_ALERTA, buffer, false); // Envia para o tópico de alertas 
}