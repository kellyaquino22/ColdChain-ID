# ColdChain-ID

**Autor:** Kelly Aquino · **Orientador:** Herbert Rocha  
**Instituição:** ECAI — Universidade Federal de Roraima (UFRR)  
**Curso:** Especialização em Computação Aplicada na Indústria 4.0

---

## Arquitetura Distribuída Edge-Server para Inventário em Tempo Real com RFID e Técnicas de Localização por Proximidade

O **ColdChain-ID** é um sistema de inventário inteligente desenvolvido para automatizar a rastreabilidade da cadeia do frio na indústria de sorvetes. A solução integra RFID passivo de 13,56 MHz com IoT, usando ESP32 como nó de borda, comunicação MQTT via HiveMQ, criptografia AES-128 e uma Máquina de Estados Finitos (FSM) com verificação formal. O sistema foi validado em simulador (Wokwi) e em protótipo físico funcional com três setores industriais simulados.

> **TCC:** Monografia de Pós-graduação Lato Sensu apresentada à Especialização em Computação Aplicada na Indústria 4.0 — UFRR, Abril de 2026.

https://github.com/user-attachments/assets/9eda46c9-2eb8-4ed6-bba1-e45db7d708da

---

## Sumário

1. [Problema e Motivação](#1-problema-e-motivação)
2. [Arquitetura do Sistema](#2-arquitetura-do-sistema)
3. [Lógica de Localização e FSM](#3-lógica-de-localização-e-fsm)
4. [Tecnologias e Materiais](#4-tecnologias-e-materiais)
5. [Segurança e Confiabilidade](#5-segurança-e-confiabilidade)
6. [Verificação Formal (Z3 Solver)](#6-verificação-formal-z3-solver)
7. [Monitoramento de Gargalos Logísticos](#7-monitoramento-de-gargalos-logísticos)
8. [Atualização OTA](#8-atualização-ota-over-the-air)
9. [Persistência e Auditoria de Dados](#9-persistência-e-auditoria-de-dados)
10. [Testes e Resultados Empíricos](#10-testes-e-resultados-empíricos)
11. [Viabilidade Financeira](#11-viabilidade-financeira)
12. [Status de Desenvolvimento (TRL)](#12-status-de-desenvolvimento-trl)
13. [Como Instalar e Executar](#13-como-instalar-e-executar)
14. [Referências](#14-referências)

---

## 1. Problema e Motivação

O gerenciamento de estoques em ambientes de cadeia do frio — especialmente na indústria de sorvetes em regiões de clima extremo como Roraima — enfrenta um desafio central: a dependência de processos manuais de inventário, ineficientes e propensos a erros. Sistemas tradicionais como o código de barras exigem manuseio unitário e contato visual, o que é inviável em câmaras frias a -20°C.

A ausência de visibilidade em tempo real sobre a movimentação dos produtos dificulta o controle de perdas por quebra térmica e compromete a precisão do estoque. O ColdChain-ID resolve esse problema com rastreamento automático, sem intervenção humana, usando RFID passivo em pontos estratégicos da planta.

---

## 2. Arquitetura do Sistema

O sistema opera em três camadas:

- **Camada de Borda (Edge):** ESP32 + leitores PN532 instalados como portais nos setores de **Produção**, **Câmara Fria** e **Expedição**.
- **Camada de Gateway/Middleware:** Node-RED orquestra os dados, aplica regras de negócio, gera dashboards e persiste logs.
- **Camada de Servidor:** MongoDB armazena as coleções `LEITURAS` e `ALERTAS`; HiveMQ Cloud gerencia a mensageria MQTT.

<img width="2816" height="1536" alt="DIAGRAMA" src="https://github.com/user-attachments/assets/5846cce6-d38f-4ba5-a6cc-88ebfa248c9f" />

**Fluxo de dados:** Tags RFID passivas (13,56 MHz) → ESP32 (leitura SPI via PN532) → Payload JSON cifrado com AES-128 CBC → MQTT/TLS para HiveMQ Cloud → Node-RED (descriptografia + validação + timestamp) → MongoDB + CSV + Dashboard.

---

## 3. Lógica de Localização e FSM

### 3.1 Técnicas de Localização

A posição de cada malote é determinada pela convergência de três métodos:

1. **Localização por Proximidade:** a posição do ativo é associada ao nó de borda que captura o sinal RFID — simples e de baixo custo.
2. **Estimativa por Centróide:** quando uma etiqueta é detectada simultaneamente por múltiplos leitores, calcula-se a média aritmética das coordenadas para refinar a posição exata.
3. **Máquina de Estados Finitos (FSM):** formaliza o fluxo operacional e valida as transições entre setores, atuando como um dispositivo **Poka-Yoke digital**.

### 3.2 Estados da FSM

O firmware opera com cinco estados:

```
AGUARDANDO_TAG  →  VALIDANDO_SEQUENCIA  →  SEQUENCIA_OK  →  PUBLICANDO
                                        ↘  SEQUENCIA_ERRO (dispara alerta)
```

| Estado | Descrição |
|---|---|
| `AGUARDANDO_TAG` | Estado inicial. Aguarda leitura do PN532 com debounce de 5.000 ms. |
| `VALIDANDO_SEQUENCIA` | Verifica se o setor anterior da tag corresponde ao permitido (cache RAM). |
| `SEQUENCIA_OK` | Monta o payload JSON para envio. |
| `SEQUENCIA_ERRO` | Publica alerta no tópico `coldchain/alerta`. |
| `PUBLICANDO` | Criptografa e envia via MQTT. Se offline, salva em buffer LittleFS. |

A regra de sequência válida é: **Produção → Câmara Fria → Expedição**. Qualquer desvio aciona `SEQUENCIA_ERRO` imediatamente.

<img width="1408" height="768" alt="FSM_Kelly" src="https://github.com/user-attachments/assets/3d49a7bb-5baa-40b1-8d81-687210184335" />

### 3.3 Cache de Status das Tags (RAM)

Para validar sequências sem depender de banco de dados remoto, cada ESP32 mantém em RAM um cache com até 20 tags (`struct TagStatus`). O cache é atualizado via MQTT quando outros setores registram uma tag no tópico `coldchain/status`, garantindo que cada nó conheça o histórico recente sem banco de dados local.

---

## 4. Tecnologias e Materiais

### Hardware

| Componente | Especificação |
|---|---|
| Microcontrolador | ESP32 DEVKITV1 (Wi-Fi/Bluetooth nativos, criptografia por hardware) |
| Leitor RFID | PN532 V3 (13,56 MHz, interface SPI) |
| Tags | RFID Passivas Mifare 13,56 MHz (HF) |
| Alimentação | Fonte 5V |

<img width="2142" height="2016" alt="ESQUEMA" src="https://github.com/user-attachments/assets/400710c4-d185-47bf-84dd-151a27128690" />

### Software e Serviços

| Camada | Tecnologia |
|---|---|
| Firmware | Arduino IDE 2.3.8 · Linguagem C++ |
| Simulação | Wokwi (validação de pinagem SPI e FSM) |
| Mensageria | HiveMQ Cloud (MQTT Broker) |
| Middleware | Node-RED 3.x + Dashboard 2.0 |
| Banco de dados | MongoDB + MongoDB Compass |
| Auditoria | Python + Paho-MQTT (sniffer passivo) |
| Verificação Formal | Z3 Solver (Microsoft) |

### Bibliotecas do Firmware

- `Adafruit_PN532` — interfaceamento com o leitor RFID
- `PubSubClient` — comunicação MQTT
- `mbedtls/aes.h` — criptografia AES-128 CBC (nativa ESP32)
- `LittleFS` — buffer offline em flash
- `ArduinoJson` — serialização/deserialização JSON
- `ArduinoOTA` — atualização remota de firmware
- `WiFiClientSecure` — TLS/SSL para MQTT seguro (porta 8883)
- `time.h` — sincronização NTP

---

## 5. Segurança e Confiabilidade

### A. Criptografia AES-128 CBC + PKCS7

Todos os payloads são cifrados antes de serem publicados no broker. O processo aplica padding PKCS#7 (múltiplo de 16 bytes), cifra com `mbedTLS` (nativa do ESP32) e codifica em Base64 para transporte como string. O Node-RED descriptografa na camada de middleware antes da persistência.

### B. QoS 1 (Quality of Service)

Todas as publicações MQTT críticas usam **QoS 1**, garantindo entrega com confirmação. Mensagens são reenviadas até o recebimento pelo broker em caso de falha de rede.

### C. Last Will and Testament (LWT)

Se o ESP32 desconectar abruptamente (falha de energia ou rede), o broker publica automaticamente um alerta de **OFFLINE** no tópico `coldchain/status`. O Dashboard reflete esse estado em tempo real, evitando interpretações equivocadas no monitoramento.

### D. Buffer Offline (LittleFS)

Quando a conexão Wi-Fi ou NTP estiver indisponível, o dado não é descartado. É salvo em `/buffer.txt` no sistema de arquivos Flash. Ao reestabelecer conexão, `processBuffer()` relê o arquivo, substitui timestamps `PENDENTE_NTP` pelo horário atual e reenvia cada registro ao broker.

### E. Proteção de Credenciais

Todas as senhas e tokens (Wi-Fi, HiveMQ, MongoDB) são separados em `config.h`, protegidos pelo `.gitignore`. Nenhuma credencial está exposta no repositório.

### F. Debounce de Software (5.000 ms)

Implementado via `millis()` (não bloqueante), elimina registros redundantes por leituras repetidas da mesma tag em curto intervalo.

---

## 6. Verificação Formal (Z3 Solver)

A FSM foi verificada matematicamente usando o motor de prova de teoremas **Z3 Solver** (Microsoft), garantindo que o sistema **nunca publique dados de inventário quando a sequência for inválida**.

### Proposições Atômicas

| Símbolo | Significado |
|---|---|
| T | Tag detectada com intervalo de debounce válido |
| V | Sequência do setor é válida (setor anterior == permitido) |
| P | Payload foi processado e publicado |
| A | Alerta de erro de sequência foi emitido |
| Eₙ | Sistema está no estado n (1 a 5) |

### Formalização das Transições

```
(E₁ ∧ T)   ⟹ E₂   — tag válida detectada → vai para validação
(E₂ ∧ V)   ⟹ E₃   — sequência correta → estado OK
(E₂ ∧ ¬V)  ⟹ E₄   — sequência incorreta → estado Erro
E₃         ⟹ E₅   — sequência OK → transição imediata para publicação
(E₅ ∧ P)   ⟹ E₁   — publicado/buffered → retorna ao início
(E₄ ∧ A)   ⟹ E₁   — alerta emitido → retorna ao início
```

### Invariante de Segurança

A propriedade inserida foi: **E₃ ⟹ (E₂ ∧ V)**

- Sem a invariante: resultado **SAT** (inconsistência detectada — E₃ sem dependência do estado anterior).
- Com a invariante: resultado **UNSAT** — comprova que é matematicamente impossível publicar um dado de inventário com sequência inválida.

**Prova por Contradição:** Para chegar em E₅ (publicação), o sistema obrigatoriamente passa por E₃. Para chegar em E₃, V deve ser verdadeiro. Logo, ¬V só pode levar a E₄, e não existe transição de E₄ para E₅. **O sistema é logicamente seguro.**

---

## 7. Monitoramento de Gargalos Logísticos

Foi implementado no Node-RED um fluxo de **Alerta de Gargalo** para monitoramento temporal da cadeia do frio. O fluxo rastreia o tempo de permanência das etiquetas entre os setores **Câmara Fria** e **Expedição**, usando contexto global para registrar entradas e saídas.

**Regra de negócio:** se o tempo de permanência exceder **2 horas**, o sistema dispara alertas simultâneos via:
- Tópico MQTT dedicado
- Dashboard Node-RED (visualmente destacado)
- **E-mail automático via SMTP** para gestores responsáveis

Esse mecanismo garante que gestores recebam alertas críticos mesmo sem monitorar ativamente o dashboard, reduzindo o tempo de resposta a eventos logísticos e prevenindo perdas por quebra térmica.

---

## 8. Atualização OTA (Over-the-Air)

O firmware inclui o protocolo **ArduinoOTA**, permitindo atualização remota via Wi-Fi — essencial para dispositivos instalados em câmaras frias a -20°C, onde o acesso físico implica riscos operacionais e térmicos.

**Segurança do OTA:** autenticação por senha + identificação única por hostname do nó. O progresso da atualização é monitorado em tempo real via Serial.

```cpp
ArduinoOTA.setHostname(hostName);
ArduinoOTA.setPassword("admin123"); // Recomenda-se trocar em produção
ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
  Serial.printf("Progresso: %u%%\r", (progress / (total / 100)));
});
```

---

## 9. Persistência e Auditoria de Dados

O sistema usa uma arquitetura de **validação tripla** para garantir rastreabilidade completa:

| Camada | Tecnologia | Conteúdo |
|---|---|---|
| NoSQL | MongoDB — coleção `LEITURAS` | Registros válidos: UID da tag, setor, timestamp (pós-descriptografia AES pelo Node-RED) |
| NoSQL | MongoDB — coleção `ALERTAS` | Exceções de `SEQUENCIA_ERRO`: UID, setor atual, setor anterior, mensagem de erro |
| Arquivo Plano | CSV via Node-RED (`logs.csv`) | Log incremental (modo append) — auditoria primária contínua |
| Flash Local | LittleFS no ESP32 | Buffer temporário offline — sincronizado automaticamente ao reconectar |
| Auditoria Externa | Python + Paho-MQTT | Sniffer passivo via TLS/SSL — captura e classifica eventos independentemente do orquestrador |

A triangulação entre MongoDB, CSV e o auditor Python confirma a confiabilidade total da rastreabilidade para auditorias industriais e análise forense de falhas.

**Gerenciamento:** MongoDB Compass (interface local). Timestamps são normalizados pelo Node-RED como autoridade de tempo, corrigindo automaticamente inconsistências do NTP embarcado.

<img width="2559" height="1283" alt="FLUXOS NODE-RED" src="https://github.com/user-attachments/assets/ef8ea4f7-6a7b-44b3-a3d2-9b72d7cc2027" />

---

## 10. Testes e Resultados Empíricos

### A. Simulação Virtual (Wokwi)

Antes da montagem física, o sistema foi modelado no simulador Wokwi para:
- Validar a pinagem e conectividade SPI entre ESP32 e PN532
- Testar transições da FSM com etiquetas simuladas
- Confirmar a correta formação de objetos JSON para MQTT

### B. Protótipo Físico

| Métrica | Resultado | Relevância |
|---|---|---|
| Latência fim-a-fim | 2 a 3 segundos | Atende requisitos de tempo real da Indústria 4.0 |
| Alcance de Leitura | ~2 cm | Atenuação por água/gordura do sorvete em 13,56 MHz |
| Debounce de Software | 5.000 ms | Elimina registros redundantes no banco |
| Resiliência de Rede | Buffer LittleFS | Sem perda de dados durante oscilações Wi-Fi |
| Segurança | AES-128 CBC + PKCS7 | Confidencialidade industrial confirmada |
| Manutenção | OTA via Wi-Fi | Atualizações sem acesso físico (até -20°C) |

### C. Verificação Formal (Z3 Solver)

Resultado **UNSAT** — confirma imunidade matemática a bypass de sequência (detalhes na [Seção 6](#6-verificação-formal-z3-solver)).

### D. Auditoria Independente (Python)

O auditor externo capturou simultaneamente eventos `DADO_INVENTARIO` e `ALERTA_SEQUENCIA` em tempo real via HiveMQ Cloud, triangulando com MongoDB e CSV para confirmar integridade total da rastreabilidade.

---

## 11. Viabilidade Financeira

Estimativa para implantação de **4 portais de leitura** (Produção, Etiquetagem, Câmara Fria, Expedição):

| Item | Descrição | Qtd. | Un. (R$) | Total (R$) |
|---|---|---|---|---|
| Hardware | ESP32 com Wi-Fi/Bluetooth | 4 | 60,00 | 240,00 |
| Leitores | Módulo RFID PN532 V3 | 4 | 85,00 | 340,00 |
| Etiquetas | Tags RFID Passivas 13,56 MHz (lote 500 un.) | 1 | 750,00 | 750,00 |
| Serviços | Programação Node-RED/MongoDB/HiveMQ/Arduino | 1 | 4.500,00 | 4.500,00 |
| Infraestrutura | Fontes e cabeamento de rede | 4 | 80,00 | 320,00 |
| APF | Desenvolvimento de software (25 Pontos de Função) | 25 | 180,00 | 4.500,00 |
| **TOTAL** | | | | **R$ 10.650,00** |

*Preços de hardware baseados em MakerHero (www.makerhero.com). Fonte: elaborada pela autora (2026).*

Baseado em Rao (2025), sistemas IoT similares apresentam redução de 14,2% no tempo de trânsito e 11,7% nas perdas de estoque, com mitigação de 79% nas variações térmicas em cadeias do frio. Para ativos sensíveis como sorvetes, o **payback estimado é de aproximadamente 9,7 meses**.

---

## 12. Status de Desenvolvimento (TRL)

O ColdChain-ID encontra-se em nível de **MVP funcional**, próximo ao TRL 4–5:

| TRL | Descrição | Evidência no ColdChain-ID |
|---|---|---|
| TRL 4 | Validação em laboratório | Leitura RFID, FSM, MQTT, AES-128 e Dashboard testados em hardware real com métricas documentadas |
| TRL 5 | Protótipo operacional validado | 3 setores simulados, verificação formal Z3 (UNSAT), auditoria Python independente |
| TRL 6–7 *(próximo passo)* | Piloto industrial | Encapsulamento IP66/IP67, tags para -20°C, integração ERP, testes em ambiente industrial real |

### Demandas de Desenvolvimento Remanescentes

- **Robustez industrial (IP66/IP67):** encapsulamento dos módulos ESP32 e PN532 contra condensação e umidade extrema.
- **Tags certificadas para baixa temperatura:** etiquetas com encapsulamento para operação abaixo de -20°C, ampliando o alcance de leitura em produtos congelados.
- **Inteligência preditiva (IA/FEFO):** algoritmos de análise de validade de lotes por histórico térmico (First-Expired, First-Out).
- **Integração ERP:** conexão com sistemas corporativos via nuvem para gestão unificada.
- **Antenas com polarização circular:** eliminação de zonas mortas causadas por reflexões metálicas no ambiente industrial.

---

## 13. Como Instalar e Executar

### Passo 1 — Firmware (ESP32)

1. Abra a pasta `/src` na Arduino IDE 2.x.
2. Renomeie `config_example.h` para `config.h`.
3. Preencha `config.h` com suas credenciais de Wi-Fi e HiveMQ Cloud.
4. Instale as bibliotecas necessárias na IDE:
   - `WiFiClientSecure`, `PubSubClient`, `Wire`, `Adafruit_PN532`
   - `time.h`, `LittleFS`, `ArduinoJson`, `ArduinoOTA`
   - `mbedtls` (nativa do ESP32 — não requer instalação manual)
5. Compile e faça o upload para cada ESP32, ajustando `SETOR` e `ETAPA_ANTERIOR_PERMITIDA` em cada arquivo conforme o setor físico.

### Passo 2 — Node-RED

1. Importe o arquivo JSON localizado em `/flows`.
2. Configure as credenciais do HiveMQ nos nós de entrada MQTT.
3. Configure a string de conexão MongoDB no nó de saída.
4. O nó de alerta SMTP requer configuração das credenciais de e-mail para notificações de gargalo.

### Passo 3 — MongoDB

1. Instale o MongoDB Community Server em [mongodb.com](https://www.mongodb.com).
2. Instale o MongoDB Compass para visualização.
3. Conecte em `mongodb://localhost:27017`.
4. Crie o banco `ColdChainDB` com as coleções `inventario`, `LEITURAS` e `ALERTAS`.
5. Verifique que a string de conexão no Node-RED aponta para seu IP local ou `127.0.0.1`.

### Passo 4 — Auditoria Python (opcional)

O script em `/audit/auditor.py` usa `paho-mqtt` para monitorar o broker de forma independente:

```bash
pip install paho-mqtt
python auditor.py
```

---

## 14. Referências

- Ballou, R. H. (2006). *Gerenciamento da Cadeia de Suprimentos: Logística Empresarial*. Porto Alegre: Bookman.
- Ferdousmou, J. et al. (2024). IoT-Enabled RFID in Supply Chain Management. *Journal of Computer and Communications*, v. 12, n. 11. DOI: [10.4236/jcc.2024.1211015](https://doi.org/10.4236/jcc.2024.1211015)
- Wu, W. et al. (2023). Internet of Everything and Digital Twin for Cold Chain Logistics. *Journal of Industrial Information Integration*, v. 33. DOI: [10.1016/j.jii.2023.100443](https://doi.org/10.1016/j.jii.2023.100443)
- Rao, P. (2026). Real-time visibility: IoT revolutionizes inventory in transit tracking. *European Journal of Computer Science and Information Technology*, v. 13, n. 23. DOI: [10.37745/ejcsit.2013](https://doi.org/10.37745/ejcsit.2013)
- Sousa, L., Rocha, H. et al. (2023). INEXT: A Computer System for Indoor Object Location using RFID. *SBESC*. DOI: [10.5753/sbesc_estendido.2023.235021](https://doi.org/10.5753/sbesc_estendido.2023.235021)
- Hopcroft, J. E., Motwani, R., Ullman, J. D. (2002). *Introdução à teoria de autômatos, linguagens e computação*. Porto Alegre: Bookman.
- Masoudi, B., Razi, N. & Rezazadeh, J. (2025). IoT-Enabled Indoor Real-Time Tracking Using UWB. *Computers*, v. 14, n. 12. DOI: [10.3390/computers14120510](https://doi.org/10.3390/computers14120510)
- Yan, B. & Lee, D. (2009). Application of RFID in cold chain temperature monitoring. *IEEE CCCM*. DOI: [10.1109/CCCM.2009.5270408](https://doi.org/10.1109/CCCM.2009.5270408)

---

## Licença

Este projeto é distribuído sob a Licença MIT. Veja o arquivo `LICENSE` para mais detalhes.

---

**Desenvolvido por Kelly Aquino** · UFRR, Boa Vista – Roraima, 2026
