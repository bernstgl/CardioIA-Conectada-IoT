// main.ino (Arduino - ESP32)
// PARTE 1 – Edge Computing com resiliência offline e sincronização quando online
// Sensores: DHT22 (temp/umid) + MPU6050 (aceleração) via I2C
// Armazenamento local: SPIFFS (volátil no Wokwi, persistente no hardware real)
// "Nuvem simulada": Serial.println (quando "Wi-Fi" = ligado)
// Wi-Fi simulado: slide-switch no GPIO 4 (LOW=offline, HIGH=online)
// LED de status: GPIO 2 (aceso = online)

#include <Arduino.h>
#include <Wire.h>
#include "FS.h"
#include "SPIFFS.h"
#include "DHTesp.h"

/////////////////////// Configurações de pinos e sensores ///////////////////////
#define PIN_DHT      15          // Pino do DHT22
#define PIN_WIFI_SW  4           // Slide-switch que simula a conexão Wi-Fi
#define PIN_LED      2           // LED (onboard em muitas placas ESP32)

DHTesp dht;
const int I2C_SDA = 21;
const int I2C_SCL = 22;
const uint8_t MPU_ADDR = 0x68;   // Endereço padrão do MPU6050

/////////////////////// Coleta e bufferização ///////////////////////
const char* DATA_FILE = "/buffer.csv"; // Arquivo de armazenamento local
const uint32_t SAMPLE_INTERVAL_MS = 2000; // Intervalo de amostragem
const size_t   MAX_FILE_BYTES = 200 * 1024; // ~200 KB (estratégia limite)
                                           // (ajuste conforme o "modelo de negócio")
unsigned long lastSample = 0;

/////////////////////// Funções utilitárias ///////////////////////
bool mountSPIFFS() {
  if (!SPIFFS.begin(true)) {
    Serial.println("[ERRO] Falha ao montar SPIFFS");
    return false;
  }
  return true;
}

bool isOnline() {
  pinMode(PIN_WIFI_SW, INPUT_PULLDOWN);
  return digitalRead(PIN_WIFI_SW) == HIGH;
}

void setLed(bool on) {
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, on ? HIGH : LOW);
}

// Inicializa o MPU6050 em modo ativo (sai do sleep)
bool initMPU6050() {
  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B); // PWR_MGMT_1
  Wire.write(0);    // seta para 0 => wkae up
  return Wire.endTransmission() == 0;
}

// Leitura simples dos registradores de aceleração (brutos)
// Conversão para "g" aproximada usando sensibilidade default (±2g => 16384 LSB/g)
bool readAccel(float &ax, float &ay, float &az) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B); // ACCEL_XOUT_H
  if (Wire.endTransmission(false) != 0) return false;

  Wire.requestFrom((int)MPU_ADDR, 6, true);
  if (Wire.available() < 6) return false;

  int16_t AcX = (Wire.read() << 8) | Wire.read();
  int16_t AcY = (Wire.read() << 8) | Wire.read();
  int16_t AcZ = (Wire.read() << 8) | Wire.read();

  ax = AcX / 16384.0f;
  ay = AcY / 16384.0f;
  az = AcZ / 16384.0f;
  return true;
}

// Apende uma linha CSV ao arquivo local
void appendLineToFile(const String &line) {
  // Estratégia de limite: se o arquivo passou do tamanho estabelecido,
  // reinicia (simples e previsível em cenários de borda).
  File f = SPIFFS.open(DATA_FILE, FILE_APPEND);
  if (!f) {
    // Se não deu certo em APPEND (arquivo inexistente), tenta WRITE para criar
    f = SPIFFS.open(DATA_FILE, FILE_WRITE);
  }
  if (!f) {
    Serial.println("[ERRO] Não foi possível abrir o arquivo para escrita.");
    return;
  }

  // Check size and enforce cap
  if (f.size() > MAX_FILE_BYTES) {
    f.close();
    SPIFFS.remove(DATA_FILE);
    File nf = SPIFFS.open(DATA_FILE, FILE_WRITE);
    if (!nf) { Serial.println("[ERRO] Não foi possível recriar o arquivo."); return; }
    nf.println(line);
    nf.close();
  } else {
    f.println(line);
    f.close();
  }
}

// Lê todo o arquivo e envia linhas para a "nuvem simulada" (Serial), depois apaga
void flushFileToCloud() {
  if (!SPIFFS.exists(DATA_FILE)) return;
  File f = SPIFFS.open(DATA_FILE, FILE_READ);
  if (!f) {
    Serial.println("[ERRO] Falha ao abrir o arquivo para leitura.");
    return;
  }

  Serial.println("=== SINCRONIZACAO_INICIO ===");
  while (f.available()) {
    String line = f.readStringUntil('\n');
    line.trim();
    if (line.length()) {
      // Aqui seria a publicação MQTT/HTTP. Para a PARTE 1, fazemos Serial.println:
      Serial.println(line);
    }
  }
  Serial.println("=== SINCRONIZACAO_FIM ===");
  f.close();

  // Apaga o arquivo após sincronizar
  if (!SPIFFS.remove(DATA_FILE)) {
    Serial.println("[ERRO] Não foi possível remover o arquivo após sincronização.");
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);

  // LED
  pinMode(PIN_LED, OUTPUT);
  setLed(false);

  // DHT
  dht.setup(PIN_DHT, DHTesp::DHT22);

  // SPIFFS
  mountSPIFFS();

  // I2C / MPU
  if (!initMPU6050()) {
    Serial.println("[AVISO] MPU6050 não respondeu no endereço 0x68 (ok no Wokwi).");
  }

  Serial.println("Sistema iniciado. OFFLINE=coleta local | ONLINE=sincroniza e limpa buffer.");
}

void loop() {
  // Atualiza LED conforme "conexão"
  bool online = isOnline();
  setLed(online);

  // Se online, primeiro escoa o que ficou pendente no arquivo
  if (online) {
    flushFileToCloud();
  }

  // Coleta periódica
  unsigned long now = millis();
  if (now - lastSample >= SAMPLE_INTERVAL_MS) {
    lastSample = now;

    // Leitura DHT22
    TempAndHumidity th = dht.getTempAndHumidity();
    float tempC = th.temperature;
    float umid  = th.humidity;

    // Leitura MPU6050 (aceleração)
    float ax=0, ay=0, az=0;
    bool okA = readAccel(ax, ay, az);

    // Timestamp e linha CSV (ISO-like sem TZ + valores)
    // Campos: ts,tempC,umid,ax,ay,az,online
    String ts = String(millis()); // (em produção: use RTC/epoch)
    String line = ts + "," + String(tempC,2) + "," + String(umid,2) + "," +
                  (okA ? String(ax,3) : "NaN") + "," +
                  (okA ? String(ay,3) : "NaN") + "," +
                  (okA ? String(az,3) : "NaN") + "," +
                  (online ? "1" : "0");

    if (online) {
      // “Nuvem simulada”: Serial.println (na PARTE 2 você troca por MQTT)
      Serial.println(line);
    } else {
      // Resiliência offline: grava no SPIFFS
      appendLineToFile(line);
    }
  }
}
