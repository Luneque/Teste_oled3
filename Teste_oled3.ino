#include <ESP8266WiFi.h>
#include <Wire.h>
#include <U8g2lib.h>

// Credenciais Wi-Fi conforme especificação
const char* WIFI_SSID = "S729-1";
const char* WIFI_PASS = "25102026";

// Configuração do Wemos OLED Shield (64x48, SSD1306, I2C)
// D1 Mini default I2C pins: D1 (SCL / GPIO5), D2 (SDA / GPIO4)
// Endereço I2C típico: 0x3C
U8G2_SSD1306_64X48_ER_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

// Buffer circular de histórico de RSSI (largura da tela = 64 pixels)
#define GRAPH_WIDTH 64
#define GRAPH_Y_TOP 16
#define GRAPH_Y_BOTTOM 47
#define RSSI_MIN -95
#define RSSI_MAX -35

int8_t rssiHistory[GRAPH_WIDTH];
unsigned long lastSampleTime = 0;
const unsigned long SAMPLE_INTERVAL_MS = 500; // Amostragem a cada 500ms

void setupDisplay() {
  // Inicialização com os pinos I2C corretos do Wemos D1 Mini
  Wire.begin(D2, D1); // SDA = D2 (GPIO4), SCL = D1 (GPIO5)
  u8g2.begin();
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_profont10_tr);
  u8g2.drawStr(2, 16, "Wi-Fi RSSI");
  u8g2.drawStr(2, 30, "Iniciando..");
  u8g2.sendBuffer();
}

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  Serial.println();
  Serial.print("Conectando a: ");
  Serial.println(WIFI_SSID);

  int dotCount = 0;
  while (WiFi.status() != WL_CONNECTED && dotCount < 25) {
    delay(400);
    dotCount++;
    Serial.print(".");

    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_profont10_tr);
    u8g2.drawStr(2, 14, "Conectando");
    u8g2.setCursor(2, 28);
    u8g2.print(WIFI_SSID);

    // Barra simples de progresso
    int barWidth = (dotCount * (GRAPH_WIDTH - 4)) / 25;
    u8g2.drawFrame(2, 36, GRAPH_WIDTH - 4, 8);
    u8g2.drawBox(4, 38, barWidth, 4);
    u8g2.sendBuffer();
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWi-Fi Conectado!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("RSSI inicial: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");

    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_profont10_tr);
    u8g2.drawStr(6, 20, "Conectado!");
    u8g2.setCursor(4, 34);
    u8g2.print(WiFi.localIP().toString().c_str());
    u8g2.sendBuffer();
    delay(1000);
  } else {
    Serial.println("\nTempo limite de conexao. Tentando em segundo plano...");
  }
}

// Converte dBm para coordenada Y da tela (16 a 47)
int rssiToY(int8_t rssi) {
  if (rssi == 0) return GRAPH_Y_BOTTOM; // sem sinal ou desconhecido
  int clamped = constrain((int)rssi, RSSI_MIN, RSSI_MAX);
  // Mapeia [RSSI_MIN, RSSI_MAX] -> [GRAPH_Y_BOTTOM, GRAPH_Y_TOP]
  return map(clamped, RSSI_MIN, RSSI_MAX, GRAPH_Y_BOTTOM, GRAPH_Y_TOP);
}

void renderScreen(int8_t currentRssi, bool isConnected) {
  u8g2.clearBuffer();

  // Cabeçalho de Status / Valor Instantâneo
  u8g2.setFont(u8g2_font_6x10_tr);
  if (isConnected) {
    u8g2.setCursor(2, 10);
    u8g2.print(currentRssi);
    u8g2.print("dBm");

    // Indicador gráfico minimalista de qualidade no canto superior direito
    int bars = 1;
    if (currentRssi >= -60) bars = 4;
    else if (currentRssi >= -70) bars = 3;
    else if (currentRssi >= -80) bars = 2;

    for (int i = 0; i < 4; i++) {
      int barHeight = (i + 1) * 2;
      if (i < bars) {
        u8g2.drawVLine(53 + (i * 2), 10 - barHeight, barHeight);
      }
    }
  } else {
    u8g2.drawStr(2, 10, "OFFLINE");
  }

  // Linha separadora do topo
  u8g2.drawHLine(0, 13, GRAPH_WIDTH);

  // Linhas guia de referência (dBm) pontilhadas
  // Linha de -50 dBm
  int y50 = rssiToY(-50);
  for (int x = 0; x < GRAPH_WIDTH; x += 4) {
    u8g2.drawPixel(x, y50);
  }
  // Linha de -75 dBm
  int y75 = rssiToY(-75);
  for (int x = 2; x < GRAPH_WIDTH; x += 4) {
    u8g2.drawPixel(x, y75);
  }

  // Desenha o gráfico de histórico
  for (int x = 0; x < GRAPH_WIDTH - 1; x++) {
    int8_t r1 = rssiHistory[x];
    int8_t r2 = rssiHistory[x + 1];

    if (r1 != 0 && r2 != 0) {
      int y1 = rssiToY(r1);
      int y2 = rssiToY(r2);
      u8g2.drawLine(x, y1, x + 1, y2);
    } else if (r2 != 0) {
      int y2 = rssiToY(r2);
      u8g2.drawPixel(x + 1, y2);
    }
  }

  // Ponto atual destacado no fim do gráfico
  if (isConnected) {
    int curY = rssiToY(currentRssi);
    u8g2.drawDisc(GRAPH_WIDTH - 2, curY, 1);
  }

  u8g2.sendBuffer();
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\n\n--- Wemos D1 Mini: Monitor de RSSI Wi-Fi com U8g2 ---");

  for (int i = 0; i < GRAPH_WIDTH; i++) {
    rssiHistory[i] = 0;
  }

  setupDisplay();
  connectWiFi();
}

void loop() {
  unsigned long now = millis();

  if (now - lastSampleTime >= SAMPLE_INTERVAL_MS) {
    lastSampleTime = now;

    bool connected = (WiFi.status() == WL_CONNECTED);
    int8_t currentRssi = 0;

    if (connected) {
      currentRssi = WiFi.RSSI();
      Serial.printf("[RSSI] %d dBm | Canal: %d | IP: %s\n",
                    currentRssi,
                    WiFi.channel(),
                    WiFi.localIP().toString().c_str());
    } else {
      Serial.println("[Wi-Fi] Desconectado. Tentando reconectar...");
      WiFi.reconnect();
    }

    // Desloca o buffer para a esquerda (scroll horizontal contínuo)
    for (int i = 0; i < GRAPH_WIDTH - 1; i++) {
      rssiHistory[i] = rssiHistory[i + 1];
    }
    rssiHistory[GRAPH_WIDTH - 1] = connected ? currentRssi : 0;

    // Renderiza a interface no display
    renderScreen(currentRssi, connected);
  }

  // Permite que o ESP8266 execute tarefas de background do Wi-Fi
  yield();
}
