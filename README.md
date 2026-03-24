# ColdChain-ID
ARQUITETURA DISTRIBUÍDA EDGE-SERVER PARA INVENTÁRIO EM TEMPO REAL COM RFID E TÉCNICAS DE LOCALIZAÇÃO POR PROXIMIDADE

O ColdChain-ID é uma solução de inventário inteligente desenvolvida para otimizar a gestão da cadeia do frio na indústria de alimentos, especificamente no setor de sorvetes. O sistema utiliza a convergência entre a tecnologia de Identificação por Radiofrequência (RFID) passiva de 13,56 MHz e a Internet das Coisas (IoT) para realizar o monitoramento e rastreamento de ativos em tempo real.
Através de uma arquitetura distribuída (camadas edge, gateway e servidor), o sistema automatiza a coleta de dados, eliminando erros do manuseio manual e garantindo a integridade térmica e logística dos produtos desde a produção até a expedição.

Demonstração do Projeto
(Link do projeto funcionando)


Como Reproduzir o Ambiente Experimental
A avaliação experimental deste projeto foi dividida em duas fases principais: simulação virtual e prototipagem física.
1. Simulação Virtual (Wokwi)
Antes da montagem física, valide a lógica do sistema utilizando o simulador Wokwi.
•	Hardware Simulado: ESP32 e Leitor PN532.
•	Objetivo: Validar a pinagem SPI, a lógica da Máquina de Estados Finitos (FSM) e a geração de objetos JSON.
•	Pinagem:
o	ESP32 (VSPI) ↔ PN532
o	SCK (D18), MISO (D19), MOSI (D23), SS (D5).
2. Infraestrutura de Software
Para processar os dados capturados pelos nós de borda (Edge), você precisará configurar:
•	Broker MQTT: Utilize o HiveMQ Cloud para o gerenciamento de mensagens assíncronas.
•	Orquestração (Node-RED): Importe o fluxo do Node-RED para gerenciar as mensagens MQTT e o Dashboard interativo.
•	Banco de Dados: Configure o MongoDB localmente para persistência e auditoria dos registros de movimentação.
3. Montagem do Protótipo Físico
Siga a Tabela de Hardware abaixo para replicar a maquete funcional:
Item	Descrição	Qtd.
Microcontrolador	ESP32 com Wi-Fi nativo	4
Leitor RFID	Módulo PN532 V3 (Interface SPI)	4
Tags	Etiquetas RFID Passivas 13,56 MHz	Lote
Alimentação	Fontes Externas 5V	4



Arquitetura do Sistema
O sistema opera em uma estrutura distribuída de três camadas:
1.	Camada de Percepção (Edge): ESP32 + PN532 realizam a leitura das tags e enviam dados via protocolo MQTT.
2.	Camada de Rede (Gateway): Broker HiveMQ orquestra a comunicação via rede local (WLAN).
3.	Camada de Aplicação (Server): Node-RED processa a lógica de localização por proximidade e centróide, atualizando o Dashboard e o MongoDB.


Resultados Esperados
•	Precisão de Inventário: Até 98,7% com automação.
•	Redução de Custos: Diminuição média de 14,2% no tempo de trânsito e 11,7% nas perdas por perecibilidade.
•	Viabilidade: Sistema de baixo custo (aprox. R$ 10.650,00) ideal para PMEs.

Licença e Autor
Projeto desenvolvido por Kelly Silva de Aquino como requisito para o grau de especialista em Computação Aplicada na Indústria 4.0 pela UFRR

