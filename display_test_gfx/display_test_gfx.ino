#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_GC9A01A.h>

// Pinos para o XIAO ESP32C6
#define TFT_CS    2
#define TFT_DC    1

// Instância do display (sem SPI customizado)
Adafruit_GC9A01A tft(TFT_CS, TFT_DC);

void setup() {
  // Inicializa o display
  tft.begin();
  tft.setRotation(0);
  tft.fillScreen(GC9A01A_BLACK);

  // Desenha algo simples
  tft.setTextColor(GC9A01A_WHITE);
  tft.setTextSize(2);
  tft.setCursor(20, 100);
  tft.println("GC9A01A OK");

  tft.drawCircle(120, 120, 60, GC9A01A_RED);
}

void loop() {
  // Nada aqui
}
