case VALIDANDO_SEQUENCIA:
{
  tagUltimoSetor = getUltimoSetor(tagUID); // Busca no cache o último local da tag 
  
  // Verifica se o último setor registrado coincide com o esperado para este dispositivo 
  if (tagUltimoSetor == String(ETAPA_ANTERIOR_PERMITIDA)) {
    estadoAtual = SEQUENCIA_OK; // Segue o fluxo normal 
  } else {
    // SE OS SETORES NÃO COINCIDIREM, OCORRE A QUEBRA DE SEQUÊNCIA 
    Serial.println("Transição: VALIDANDO_SEQUENCIA -> SEQUENCIA_ERRO");
    estadoAtual = SEQUENCIA_ERRO; 
  }
  break;
}