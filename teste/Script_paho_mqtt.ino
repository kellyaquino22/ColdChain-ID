import paho.mqtt.client as mqtt
import time
import ssl

// --- CONFIGURAÇÕES DO BROKER (Baseadas no seu TCC) ---
MQTT_SERVER = "ENDERECO_DO_BROKER_HIVEMQ"
MQTT_USER   = "USUARIO_MQTT"
MQTT_PASS   = "SENHA_MQTT"
MQTT_PORT   = "xxxx"

// --- FUNÇÃO DE LOG ---
def on_message(client, userdata, msg):
    timestamp_auditor = time.strftime('%Y-%m-%d %H:%M:%S')
    topico = msg.topic
    payload = msg.payload.decode()

    //# Identifica o tipo de evento para o relatório científico
    tipo_evento = "DADO_INVENTARIO" if "rfid" in topico else "ALERTA_SEQUENCIA"
    
    log_entry = f"[{timestamp_auditor}] | TIPO: {tipo_evento} | TOPICO: {topico} | CONTEUDO: {payload}\n"
    
    //# Salva no arquivo para o anexo do TCC
    with open("auditoria_tcc_kelly.txt", "a", encoding="utf-8") as f:
        f.write(log_entry)
    
    print(f"✔ Evento de {tipo_evento} registrado com sucesso!")

//# --- CONFIGURAÇÃO DO CLIENTE ---
client = mqtt.Client()
client.username_pw_set(MQTT_USER, MQTT_PASS)

//# Configuração de Segurança (Necessária para HiveMQ Cloud)
client.tls_set(cert_reqs=ssl.CERT_REQUIRED)

client.on_message = on_message

//# --- CONEXÃO E EXECUÇÃO ---
print(f"Conectando ao Auditor ColdChainID em {MQTT_SERVER}...")
try:
    client.connect(MQTT_SERVER, MQTT_PORT)
    client.subscribe("coldchain/#") # Escuta dados, status e alertas simultaneamente
    print("Auditor Online! Aguardando eventos do ESP32 e Node-RED...")
    client.loop_forever()
except Exception as e:
    print(f"Erro na conexão: {e}")
