# FIAP - Faculdade de Informática e Administração Paulista

<p align="center">
<a href= "https://www.fiap.com.br/"><img src="assets/logo-fiap.png" alt="FIAP - Faculdade de Informática e Admnistração Paulista" border="0" width=40% height=40%></a>
</p>

<br>

# CardioIoT – Monitoramento Cardíaco com ESP32, MQTT e Dashboards

## Thiago Bernardes

## 👨‍🎓 Integrantes: 
- <a href="https://www.linkedin.com/company/">Thiago Lima Bernardes</a>

## 👩‍🏫 Professores:

### Coordenador(a)
- <a href="https://www.linkedin.com/in/profandregodoi/">André Godoi</a>


## 📜 Descrição

Este projeto integra conceitos de IoT aplicado à saúde, Edge/Fog Computing e Cloud para construir um sistema vestível (simulado) de monitoramento cardíaco. O fluxo completo é: captura → processamento local → transmissão MQTT → visualização → alerta.

Parte 1 – Edge Computing (Wokwi/ESP32):
Implementamos um firmware em ESP32 que lê dois sensores:

DHT22 (obrigatório) para temperatura e umidade;

Sensor de movimento (MPU6050) ou botão push (no Wokwi usamos o pushbutton para simular batimentos).

Para resiliência offline, o dispositivo armazena leituras em SPIFFS (observação: no simulador o SPIFFS é volátil, mas demonstra a lógica). Quando a “conexão” volta, as amostras acumuladas são enviadas e o buffer local é limpo. Na simulação, a “nuvem” é representada pelo Serial Monitor; em hardware real, o mesmo fluxo persiste, garantindo continuidade mesmo sem Internet.

Parte 2 – MQTT + Dashboards (Fog/Cloud):
Evoluímos o protótipo para enviar telemetria à nuvem via MQTT (ex.: HiveMQ Cloud, porta 8883/TLS). No Node-RED, montamos uma Dashboard com:

Gráfico em tempo real (line) de BPM (batimentos/minuto, calculados a partir de cliques no botão nos últimos 15s × 4);

Gauge de temperatura;

Indicador de alerta (texto/LED virtual) quando BPM > 120 ou Temp > 38 °C (limiares ajustáveis).

Opcionalmente, os dados podem ser persistidos em InfluxDB e consumidos no Grafana Cloud para análises históricas e painéis avançados. O resultado reforça boas práticas de IoT médico: baixa latência na borda, confiabilidade com buffer local, segurança com TLS, tópicos padronizados, observabilidade e visualização clara para tomada de decisão.


## 📁 Estrutura de pastas

Dentre os arquivos e pastas presentes na raiz do projeto, definem-se:

- <b>assets</b>: aqui estão os arquivos relacionados a elementos não-estruturados deste repositório, como imagens.

- <b>src</b>: Código fonte criado para o desenvolvimento do projeto.

- <b>Relatórios</b>: Relatórios obrigatórios das Partes 1 e 2 desta atividade

- <b>README.md</b>: arquivo que serve como guia e explicação geral sobre o projeto.

## 🔧 Como executar o código

Pré-requisitos

Wokwi (web) ou Arduino IDE + ESP32 core.

Bibliotecas Arduino (no Wokwi via Library Manager ou libraries.txt):

PubSubClient
DHT sensor library for ESPx


Node-RED (≥ 3.x) com node-red-dashboard instalado.

(Opcional) InfluxDB e Grafana Cloud.

PARTE 1 – Edge (ESP32 no Wokwi - https://wokwi.com/projects/445740340158254081)

Crie um projeto ESP32 no Wokwi e adicione o DHT22 e o MPU6050 ou pushbutton.

Cole o código da Parte 1 (https://wokwi.com/projects/445740340158254081) direto no Wokwi sketch.ino.

Adicione as libs indicadas acima.

Rode a simulação: com “Wi-Fi OFF”, as leituras são armazenadas (resiliência). Com “Wi-Fi ON”, ocorre o flush das amostras para a “nuvem simulada” (Serial) e limpeza do buffer.

Nota: no Wokwi o SPIFFS é volátil – em hardware real, os arquivos persistem.

PARTE 2 – MQTT + Dashboard (ESP32 + HiveMQ + Node-RED - https://wokwi.com/projects/445740340158254081)

Broker MQTT (HiveMQ Cloud):

Crie uma instância (host, porta 8883, usuário e senha).

No firmware (src/parte2/main.ino), ajuste: MQTT_HOST, MQTT_USER, MQTT_PASS.

Em testes, é possível usar WiFiClientSecure.setInsecure() (apenas demo). Em produção, valide certificados.

ESP32 (Wokwi):

Hardware mínimo: ESP32 + DHT22 + pushbutton (GPIO 5).

Instale as libs e rode. O dispositivo publica JSON em cardio/telemetry/<deviceId> a cada ~2s contendo { device, ts, temp, hum, bpm, tht, thb }.

Simule BPM clicando rapidamente no botão (~30 cliques em 15s ≈ 120 bpm). Ajuste temperatura no DHT para simular febre.

Node-RED (dashboard):

Instale node-red-dashboard.

Importe o flow JSON (em src/flow.json).

Abra o nó MQTT IN e configure o broker TLS (8883) com suas credenciais.

Deploy e acesse http://localhost:1880/ui.

Você verá: Gauge de temperatura, gráfico de BPM e alerta (OK/ALERTA + indicador). Limiares padrão: 38 °C e 120 bpm (alteráveis por comando MQTT em cardio/cmd/<deviceId>).

(Opcional) Persistência e Grafana Cloud

No Node-RED, instale node-red-contrib-influxdb.

Crie um bucket no InfluxDB e configure um InfluxDB Out (measurement vitals, fields temp, bpm, tag device).

No Grafana, adicione a fonte InfluxDB e crie painéis: Time series (BPM), Gauge (Temp) com thresholds.

Troubleshooting rápido

Erro PubSubClient.h: confirme as bibliotecas instaladas ou crie libraries.txt com as linhas acima.

Sem dados na Dashboard: confira o tópico (cardio/telemetry/#) e as credenciais do broker no nó MQTT.

TLS falhando: para teste, habilite setInsecure(); para produção, use CA válida.

BPM parado em 0: verifique o pino do botão (GPIO 5) e os cliques dentro da janela de 15s.


## 🗃 Histórico de lançamentos

* 1.0.0 - 23/10/2025

## 📋 Licença

<img style="height:22px!important;margin-left:3px;vertical-align:text-bottom;" src="https://mirrors.creativecommons.org/presskit/icons/cc.svg?ref=chooser-v1"><img style="height:22px!important;margin-left:3px;vertical-align:text-bottom;" src="https://mirrors.creativecommons.org/presskit/icons/by.svg?ref=chooser-v1"><p xmlns:cc="http://creativecommons.org/ns#" xmlns:dct="http://purl.org/dc/terms/"><a property="dct:title" rel="cc:attributionURL" href="https://github.com/agodoi/template">MODELO GIT FIAP</a> por <a rel="cc:attributionURL dct:creator" property="cc:attributionName" href="https://fiap.com.br">Fiap</a> está licenciado sobre <a href="http://creativecommons.org/licenses/by/4.0/?ref=chooser-v1" target="_blank" rel="license noopener noreferrer" style="display:inline-block;">Attribution 4.0 International</a>.</p>
