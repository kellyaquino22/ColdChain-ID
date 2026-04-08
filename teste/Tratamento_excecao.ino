case SEQUENCIA_ERRO:
{
  Serial.println("[SEQUENCIA_ERRO] Sequencia invalida. Emitindo alerta...");
  // Chama a função que publica no tópico "coldchain/alerta" 
  publicarAlerta(tagUID, tagUltimoSetor, tagTimestamp); 
  
  // Retorna ao estado inicial sem processar o dado como "leitura válida"
  estadoAtual = AGUARDANDO_TAG;
  break;
}