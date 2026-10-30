# Medidor de Intensidade de Wi-Fi (RSSI) com OLED 64x48

Projeto desenvolvido para a placa **Wemos D1 Mini (ESP8266EX)** integrada ao **Wemos OLED Shield (SSD1306 64x48 I2C)**, utilizando a biblioteca gráfica [`U8g2`](https://github.com/olikraus/u8g2).

## 📋 Descrição

O dispositivo conecta-se a uma rede Wi-Fi configurada, faz a amostragem periódica do sinal recebido (RSSI em dBm) e renderiza em tempo real um gráfico com a oscilação do sinal ao longo do tempo, além de exibir o valor instantâneo em dBm e um medidor gráfico de intensidade.

## 🛠️ Hardware

* **Placa:** Wemos D1 Mini (ESP8266EX)
* **Display:** Wemos OLED Shield (SSD1306 64x48)
* **Comunicação:** I2C
  * `SCL` ➔ Pino D1 (GPIO 5)
  * `SDA` ➔ Pino D2 (GPIO 4)
  * Endereço I2C: `0x3C`

## 📁 Estrutura do Projeto

* [`Teste_oled3.ino`](Teste_oled3.ino): Código-fonte do firmware em C++ para Arduino/ESP8266.
* [`docs/FSD.md`](docs/FSD.md): Documento de Especificação Funcional (FSD) detalhado.
* [`Teste_oled3.md`](Teste_oled3.md): Especificação inicial do projeto.

## 🚀 Como Compilar e Gravar

Utilizando o `arduino-cli`:

```bash
# Compilar
arduino-cli compile --fqbn esp8266:esp8266:d1_mini .

# Gravar (ajuste a porta COM conforme sua máquina)
arduino-cli upload -p COM15 --fqbn esp8266:esp8266:d1_mini .
```
