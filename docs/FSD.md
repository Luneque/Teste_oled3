# Functional Specification Document (FSD)
## Medidor de Intensidade de Sinal Wi-Fi com Wemos D1 Mini e OLED Shield

### 1. Document Control
* **Title:** Functional Specification Document - Medidor de Intensidade de Wi-Fi em Tempo Real
* **Status:** Approved / In Implementation
* **Target Version:** 1.0.0
* **Author:** Antigravity AI & Engenharia de Firmware
* **Date:** 2026-10-30

---

### 2. Executive Summary & Objectives
* **High-Level Description:** Desenvolvimento de firmware embarcado em C++/Arduino para o microcontrolador ESP8266 (placa Wemos D1 Mini) integrado com um Shield OLED SSD1306 (resolução 64x48 pixels via barramento I2C). O sistema conecta-se a um Ponto de Acesso Wi-Fi pré-configurado, monitora periodicamente a potência recebida do sinal de rádio (RSSI em dBm) e plota dinamicamente a oscilação histórica desse sinal em forma de gráfico temporal contínuo no display, utilizando a biblioteca gráfica `U8g2`.
* **Problem Statement:** A verificação empírica de cobertura e flutuação de sinal Wi-Fi em ambientes residenciais ou industriais requer dispositivos portáteis dedicados e responsivos, capazes de fornecer feedback visual imediato sobre variações locais de recepção.
* **Out of Scope:**
  * Modo captive portal / Wi-Fi Manager para configuração dinâmica de SSID/senha via navegador web (nesta versão 1.0.0 as credenciais são fixas conforme especificado).
  * Envio de telemetria para nuvem ou servidores MQTT/HTTP remotos.
  * Armazenamento persistente em memória Flash SPIFFS/LittleFS de histórico além do buffer circular de tela.

---

### 3. User Personas & User Journeys
* **Primary Persona:** Técnico de Campo, Desenvolvedor IoT ou Usuário Final avaliando cobertura de sinal Wi-Fi.
* **User Journey:**
  1. O usuário acopla o Wemos OLED Shield aos barramentos do Wemos D1 Mini e energiza o microcontrolador (via cabo micro-USB ou fonte externa).
  2. A tela OLED exibe tela de inicialização com mensagem de status de conexão (`Connecting to S729-1...`).
  3. Assim que a conexão for estabelecida com sucesso, a interface comuta para a tela principal de monitoramento:
     * **Cabeçalho:** Exibe o valor numérico instantâneo de RSSI (ex: `-52 dBm`) e status de link.
     * **Área Central/Inferior (Gráfico):** Um gráfico de linhas/histórico horizontal de 64 pontos (resolução total da largura da tela) que desliza da direita para a esquerda com amostragens periódicas de RSSI.
  4. O usuário caminha pelo ambiente e visualiza instantaneamente vales (sinal degradando) e picos (sinal forte) no gráfico.

---

### 4. Functional Requirements

| ID | Módulo / Componente | Requisito / Descrição | Prioridade |
| :--- | :--- | :--- | :--- |
| **FR-01** | **Wi-Fi Subsystem** | O dispositivo DEVE conectar-se automaticamente à rede SSID `S729-1` com chave WPA2 `25102026` em modo Station (`WIFI_STA`). | P0 |
| **FR-02** | **Wi-Fi Subsystem** | O sistema DEVE reconectar-se automaticamente caso haja perda de link com o Access Point e sinalizar visualmente no display. | P0 |
| **FR-03** | **RSSI Sampling** | O firmware DEVE coletar o RSSI (`WiFi.RSSI()`) em intervalos regulares de 500 ms sem bloqueios impeditivos (`non-blocking` via `millis()`). | P0 |
| **FR-04** | **OLED Display (U8g2)** | O display DEVE ser inicializado com o construtor adequado para SSD1306 64x48 I2C no Wemos D1 Mini (pinos D1 = SCL/GPIO5, D2 = SDA/GPIO4, I2C address `0x3C`). | P0 |
| **FR-05** | **Graph Rendering** | O display DEVE manter um buffer circular de histórico com 64 amostras (uma por coluna de pixel X) e plotar a evolução temporal normalizada entre -100 dBm (mínimo, base do gráfico) e -30 dBm (máximo, topo da área de plotagem). | P0 |
| **FR-06** | **Telemetry Header** | O terço superior do display (Y: 0 a 14) DEVE exibir o valor numérico em dBm e um indicador de conexão. | P1 |
| **FR-07** | **Serial Logging** | O microcontrolador DEVE emitir logs detalhados via UART Serial (115200 bps) para fins de depuração e auditoria de teste em bancada. | P1 |

---

### 5. Data Flow & Interface Specifications

#### 5.1 Parâmetros de Hardware e Pinagem (Wemos D1 Mini + Wemos OLED Shield)
* **MCU:** ESP8266EX (Wemos D1 Mini, 80/160MHz, 4MB Flash).
* **Display Controller:** SSD1306 64x48 pixels OLED monocromático.
* **Barramento I2C:**
  * `SCL` -> Pino D1 (GPIO 5)
  * `SDA` -> Pino D2 (GPIO 4)
  * Endereço I2C: `0x3C` (padrão do shield Wemos OLED).
* **Construtor U8g2:** `U8G2_SSD1306_64X48_ER_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);`

#### 5.2 Estrutura de Dados
* Buffer de amostras: `int8_t rssiHistory[64]` inicializado com valor nulo / fora de escala.
* Faixa de mapeamento de RSSI para gráfico:
  * $RSSI_{min} = -95 \text{ dBm}$ (mapeado para $Y_{base} = 47$)
  * $RSSI_{max} = -35 \text{ dBm}$ (mapeado para $Y_{topo} = 16$)
  * Altura da viewport do gráfico: 32 pixels ($Y \in [16, 47]$).

#### 5.3 Máquina de Estados do Firmware
1. `STATE_BOOT`: Inicialização de GPIO, Serial (115200) e Display OLED. Exibição de logo/splash screen.
2. `STATE_CONNECTING`: Tentativa de conexão Wi-Fi com animação de progresso no OLED.
3. `STATE_MONITORING`: Conectado. Coleta periódica de RSSI (500 ms), atualização do buffer circular e redesenho do gráfico e métricas.
4. `STATE_DISCONNECTED`: Caso `WiFi.status() != WL_CONNECTED`, exibe aviso visual e tenta restabelecer a conexão sem travar o loop.

---

### 6. Non-Functional Requirements
* **Performance:** Taxa de amostragem de 500 ms; ciclo de renderização U8g2 inferior a 30 ms por frame (buffer completo em RAM `_F_`).
* **Estabilidade:** Uso estrito de chamadas assíncronas/não bloqueantes (`millis()`), garantindo alimentação do software watchdog (`yield()` / `ESP.wdtFeed()`).
* **Consumo de Memória:** RAM requerida pelo buffer de tela (64x48 = 384 bytes) e variáveis < 2 KB, perfeitamente compatível com a SRAM de 80 KB do ESP8266.

---

### 7. Acceptance Criteria (Critérios de Aceitação)
* **AC-01:** Dado que o Wemos D1 Mini é iniciado, quando tenta se associar à rede `S729-1`, então deve estabelecer conexão Wi-Fi e obter endereço IP.
* **AC-02:** Dado que o Wi-Fi está conectado, quando o valor de RSSI for lido, então o display OLED de 64x48 deve exibir a medição numérica atual (ex: `-65dBm`).
* **AC-03:** Dado que novas leituras ocorrem no tempo, quando cada amostra chega, então o gráfico deve deslocar-se uma coluna para a esquerda e desenhar o novo ponto na extremidade direita com altura correspondente à intensidade relativa em dBm.
* **AC-04:** Dado que o dispositivo é submetido ao teste prático conectado na porta serial `COM15`, então o upload do firmware é concluído com sucesso e o monitor serial e display confirmam o funcionamento.
