# ColdChain-ID
ARQUITETURA DISTRIBUÍDA EDGE-SERVER PARA INVENTÁRIO EM TEMPO REAL COM RFID E TÉCNICAS DE LOCALIZAÇÃO POR PROXIMIDADE

O ColdChain-ID é uma solução de inventário inteligente desenvolvida para otimizar a gestão da cadeia do frio na indústria de alimentos, especificamente no setor de sorvetes. O sistema utiliza a convergência entre a tecnologia de Identificação por Radiofrequência (RFID) passiva de 13,56 MHz e a Internet das Coisas (IoT) para realizar o monitoramento e rastreamento de ativos em tempo real.
Através de uma arquitetura distribuída (camadas edge, gateway e servidor), o sistema automatiza a coleta de dados, eliminando erros do manuseio manual e garantindo a integridade térmica e logística dos produtos desde a produção até a expedição.

[![Vídeo de Demonstração](https://img.shields.io/badge/VÍDEO-DEMONSTRAÇÃO-red?style=for-the-badge&logo=youtube)](LINK_DO_SEU_VIDEO_CURTO_AQUI)

Este projeto foi desenvolvido como Trabalho de Conclusão de Curso (TCC) para a Especialização em Engenharia de Computação Aplicada à Indústria (ECAI) na UFRR. O sistema implementa uma solução de Internet das Coisas (IoT) para rastrear e monitorar a movimentação de produtos (potes de sorvete) em tempo real dentro de um ambiente fabril com câmera fria.


## 1. Arquitetura do Sistema

O diagrama abaixo ilustra a arquitetura completa da solução, mostrando o fluxo de dados dos sensores de radiofrequência (RFID) até o banco de dados e a interface de visualização.
DIAGRAMA DE ARQUITETURA: RFID PARA MONGODB VIA ESP32 E MQTT

![Fluxo do Projeto](Image.png)

==========================================================

[ SENSORES ]          [ ESP32 ]          [ MQTT BROKER ]       [ NODE-RED ]         [ DASHBOARD ]
+----------+        +-----------+        +-------------+      +------------+       +-------------+
| RFID/NFC |        |  ESP32    |        |             |      | Logic Flow |       |  Node-RED   |
|  Module  |  I2C   | DEVKIT V1 |  WiFi  |   HiveMQ    | WiFi |            |  HTTP |  Dashboard  |
|   (V3)   |------->|           |------->|  (Broker)   |----->|  MongoDB   |------>| (Interface) |
+----------+        |  Coleta   |        |             |      |    Out     |       |             |
                    +-----------+        +-------------+      +------------+       +-------------+
                          |                     |                    |                    |
                  (Sensor de RF)        (Pub/Sub Protocol)    (Processamento)      (Visualização)

                  
## 2. Tecnologias e Materiais Utilizados

### Hardware
* Microcontrolador **ESP32 DEVKITV1**
* Leitor RFID **PN532 (Módulo V3)**
* Cartões/Etiquetas (Tags) RFID (Mifare 13.56MHz)

### Software e Serviços
* **Arduino IDE 2.x** (Firmware do ESP32)
* **HiveMQ Cloud** (MQTT Broker)
* **Node-RED 3.x** com Dashboard 2.0 (Motor de Fluxo e Visualização)
* **MongoDB Atlas** (Banco de Dados NoSQL Cloud)

## 3. Como Instalar e Executar

Este repositório está organizado para guiar a replicação do ambiente experimental. Siga os passos:

### Passo 3.1: Configuração do Firmware (ESP32)
1.  Abra a pasta `/src` na Arduino IDE.
2.  Renomeie o arquivo `config_example.h` para `config.h`.
3.  Abra `config.h` e insira suas credenciais de Wi-Fi e HiveMQ Cloud nos locais indicados. **NUNCA suba o arquivo `config.h` real para o GitHub.**
4.  Instale as bibliotecas necessárias na IDE: `WiFiClientSecure`, `PubSubClient`, `Wire`, `Adafruit_PN532`, `time.h`, e `LittleFS`.
5.  Compile e faça o upload para o ESP32.

### Passo 3.2: Configuração do Node-RED
1.  Importe o arquivo JSON localizado na pasta `/flows`.
2.  Configure as credenciais do HiveMQ nos nós de entrada MQTT.
3.  Configure a string de conexão do MongoDB Atlas no nó de saída MongoDB.


## 4. Resultados e Análise Técnica

Este projeto implementa boas práticas de **cibersegurança e confiabilidade industrial**, conforme as orientações da banca:

### A. Confiabilidade e QoS 1
Para garantir que as leituras das Tags RFID não se percam em caso de oscilação do Wi-Fi da fábrica, todas as publicações MQTT críticas utilizam o nível **QoS 1 (Quality of Service)**.

### B. Monitoramento de Dispositivos (LWT)
O sistema implementa o recurso **Last Will and Testament (LWT)** do protocolo MQTT. Se o ESP32 desconectar bruscamente (falha de energia ou rede), o Dashboard exibe automaticamente um alerta de **OFFLINE** no tópico `coldchain/status`.

### C. Segurança de Credenciais
Todas as senhas e tokens de acesso (Wi-Fi, HiveMQ, MongoDB) foram separadas em arquivos de configuração locais (`config.h`), protegidos pelo `.gitignore`, evitando a exposição de dados sensíveis na internet.

### D. Buffer Offline (LittleFS)
Se a conexão com o broker cair, o ESP32 armazena as leituras no sistema de arquivos local (`LittleFS`) e sincroniza automaticamente assim que a conexão for restabelecida.


## 5. Licença
Este projeto é distribuído sob a Licença MIT. Veja o arquivo `LICENSE` para mais detalhes.


**Desenvolvido por Kelly Aquino**
