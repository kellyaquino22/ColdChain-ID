switch (estadoAtual) {

    case AGUARDANDO_TAG:
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
        Serial.println("Transição: AGUARDANDO_TAG -> VALIDANDO_SEQUENCIA");
        estadoAtual = VALIDANDO_SEQUENCIA;
      }
      break;
    }