Para o esquema de ligação (pinout) entre o ESP32 DEVKITV1 e o módulo RFID RC522 (ou o PN532 via SPI), 
a pinagem padrão no ESP32 utiliza os pinos do barramento VSPI.
Aqui está a tabela de conexões recomendada para garantir que o seu sensor NFC MODULE V3 seja lido corretamente
pelo microcontrolador:

Conexões (Padrão SPI)

Sensor RFID (Pin) -> ESP32 DEVKITV1 (GPIO)	-> Função

3.3V	 -> 3V3	-> Alimentação

RST ->	GPIO 22	-> Reset

GND	-> GND	-> Terra

MISO ->	GPIO 19	-> Master In Slave Out

MOSI ->	GPIO 23	-> Master Out Slave In

SCK	-> GPIO 18	-> Serial Clock

SDA (SS)	-> GPIO 21	-> Slave Select / Chip Select



Observações Importantes para o seu Hardware:
•	Nível Lógico: O ESP32 e a maioria dos módulos RFID vermelhos (NFC V3) operam em 3.3V. Evite alimentar o sensor com 5V para não danificar os pinos de dados do microcontrolador.
•	Antena: Ao montar a estrutura física, mantenha o sensor afastado de superfícies metálicas grandes, que podem causar interferência na leitura das tags.
•	Bibliotecas: Se estiver usando o Arduino IDE, a biblioteca MFRC522 de Github Community funciona muito bem com essa pinagem, bastando definir os pinos SS_PIN como 21 e RST_PIN como 22 no seu código.

