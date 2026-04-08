Para o esquema de ligação (pinout) entre o ESP32 DEVKITV1 e o módulo RFID RC522 (ou o PN532 via SPI), 
a pinagem padrão no ESP32 utiliza os pinos do barramento VSPI.
Aqui está a tabela de conexões recomendada para garantir que o seu sensor NFC MODULE V3 seja lido corretamente
pelo microcontrolador:

<img width="2142" height="2016" alt="ESQUEMA 1" src="https://github.com/user-attachments/assets/e57e3808-9548-446c-be4c-9048bf7d815d" />


<img width="2816" height="1536" alt="ESQUEMA PINAGEM" src="https://github.com/user-attachments/assets/1dc12ad4-5d55-487b-90aa-592f70f11d9a" />



Observações Importantes para o seu Hardware:

•	Nível Lógico: O ESP32 e a maioria dos módulos RFID vermelhos (NFC V3) operam em 3.3V. 
Evite alimentar o sensor com 5V para não danificar os pinos de dados do microcontrolador.

•	Antena: Ao montar a estrutura física, mantenha o sensor afastado de superfícies metálicas grandes, 
que podem causar interferência na leitura das tags.

•	Bibliotecas: Se estiver usando o Arduino IDE, a biblioteca MFRC522 de Github Community funciona 
muito bem com essa pinagem, bastando definir os pinos SS_PIN como 21 e RST_PIN como 22 no seu código.

