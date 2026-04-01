Documentação de Análise de Pontos de Função (APF) - ColdChain-ID

Esta documentação detalha a mensuração do tamanho funcional do software ColdChain-ID, 
justificando o investimento em desenvolvimento de software conforme apresentado na Tabela 
de Custos do projeto.

1. Resumo da Contagem
-Quantidade de Pontos de Função (PF): 25 PF 
-Valor por Ponto de Função: R$ 180,00 
-Custo Total de Desenvolvimento: R$ 4.500,00 

2. Escopo Funcional (O que foi medido)
A contagem baseia-se nas funcionalidades entregues pelas camadas Edge (ESP32), Broker (HiveMQ) e Server (Node-RED/MongoDB).

A. Funções de Dados (Arquivos Lógicos Internos)
Cadastro de Itens/Lotes: Estrutura de dados no MongoDB para armazenar o UID da tag e metadados do produto.
Histórico de Movimentações: Log persistente de transições de estado e logs de eventos temporais.

B. Funções de Transação
-Entrada Externa (Identificação RFID): Captura e envio de carga útil JSON via protocolo MQTT.
-Saída Externa (Dashboard Interativo): Visualização em tempo real das tabelas de inventário e status dos setores.
-Consulta Externa (Relatórios de Auditoria): Recuperação de dados históricos do MongoDB para conferência de estoque.
-Interface Externa (Alertas e Notificações): Lógica de disparo de alertas via e-mail em caso de violação de zonas frias.

3. Justificativa de Valor Adicionado
O uso da métrica de Pontos de Função em vez de "horas trabalhadas" justifica-se pela complexidade da integração entre hardware e software:
1) Sincronização Temporal: Garantia de linearidade cronológica via redundância de timestamps (NTP + Node-RED).
2)Resiliência de Dados: Implementação de QoS 1 no MQTT e armazenamento temporário em sistema de arquivos LittleFS para evitar perda de dados em falhas de Wi-Fi.
3)Segurança: Segregação de credenciais e gestão dinâmica de variáveis de ambiente.

4. Referência de Cálculo:
O valor unitário do Ponto de Função (R$ 180,00) segue as práticas de mercado
 para desenvolvimento de sistemas embarcados e IoT em 2026, considerando a alta especialização necessária para a orquestração de protocolos industriais e bancos de dados NoSQL.
