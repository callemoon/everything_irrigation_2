#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

// Pinnar för ESP32-C3
#define TFT_SCLK 0
#define TFT_MOSI 1
#define TFT_RST  2
#define TFT_DC   3
#define TFT_CS   4
#define PIN_LED  5

// Färgdefinitioner (RGB565)
#define COLOR_BG        0x0821  // Mörkblå/svart bakgrund
#define COLOR_PANEL     0x18C5  // Mörkgrå panel
#define COLOR_BORDER    0x3218  // Subtil ramfärg
#define COLOR_CYAN      0x07FF  // Stark cyan
#define COLOR_ORANGE    0xFCE0  // Neon orange
#define COLOR_GREEN     0x07E0  // Stark grön
#define COLOR_RED       0xF800  // Röd
#define COLOR_WHITE     0xFFFF  // Vit
#define COLOR_GRAY      0x7BEF  // Grå text

SPIClass spiDisplay(FSPI);
Adafruit_ST7789 tft = Adafruit_ST7789(&spiDisplay, TFT_CS, TFT_DC, TFT_RST);

// Variabler för animering och graf
int graphPoints[40];
int graphIndex = 0;
float cpuUsage = 18.0;
unsigned long lastUpdate = 0;

// Hjälpfunktion för att rita en cirkulär mätare
void drawGauge(int x, int y, int radius, int valPercent, uint16_t color) {
  tft.drawCircle(x, y, radius, COLOR_BORDER);
  tft.drawCircle(x, y, radius - 1, COLOR_BORDER);
  
  // Rita båge utifrån procent (förenklad cirkelritning)
  int numDots = map(valPercent, 0, 100, 0, 16);
  for (int i = 0; i < 16; i++) {
    float angle = (i * 22.5 - 210) * 3.14159 / 180.0;
    int px = x + cos(angle) * (radius - 3);
    int py = y + sin(angle) * (radius - 3);
    
    if (i < numDots) {
      tft.fillCircle(px, py, 2, color);
    } else {
      tft.fillCircle(px, py, 1, COLOR_BORDER);
    }
  }
}

// Rita den statiska layouten en gång för att undvika flimmer
void drawStaticUI() {
  tft.fillScreen(COLOR_BG);

  // 1. Rubrik-panel (Titel)
  tft.fillRoundRect(8, 8, 154, 38, 6, COLOR_PANEL);
  tft.drawRoundRect(8, 8, 154, 38, 6, COLOR_CYAN);
  
  tft.setTextSize(2);
  tft.setTextColor(COLOR_CYAN);
  tft.setCursor(20, 19);
  tft.print("SYS STATUS");

  // Under-rubrik
  tft.setTextSize(1);
  tft.setTextColor(COLOR_ORANGE);
  tft.setCursor(20, 52);
  tft.print("CORE: ESP32-C3");

  // 2. CPU-Sektion
  tft.drawFastHLine(10, 68, 150, COLOR_BORDER);
  
  // 3. Minnesmätare (Progress bar ram)
  tft.setTextColor(COLOR_WHITE);
  tft.setCursor(12, 142);
  tft.print("MEM FREE: 245 KB");
  
  tft.drawRoundRect(10, 156, 150, 16, 4, COLOR_BORDER);

  // 4. Graf-sektion
  tft.setTextColor(COLOR_GRAY);
  tft.setCursor(12, 185);
  tft.print("UPTIME & LOAD");
  
  tft.drawRect(10, 200, 150, 80, COLOR_BORDER);
  
  // 5. Bottenfält (Footer)
  tft.fillRect(0, 302, 170, 18, COLOR_PANEL);
  tft.setTextColor(COLOR_GRAY);
  tft.setTextSize(1);
  tft.setCursor(6, 307);
  tft.print("RES:170x320  GPIO:OK");
}

void setup() {
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, HIGH);

  delay(200);

  // Starta SPI och display (Stående läge: 170 bred, 320 hög)
  spiDisplay.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);
  tft.init(170, 320);
  tft.setSPISpeed(80000000); // 40 MHz
  tft.setRotation(0); // 0 = Stående läge (170x320)

  drawStaticUI();

  // Fyll graf-array med startvärden
  for (int i = 0; i < 40; i++) {
    graphPoints[i] = random(10, 60);
  }
}

void loop() {
  // Uppdatera grafiken var 300:e millisekund
  if (millis() - lastUpdate > 300) {
    lastUpdate = millis();

    // --- Simulera nya data ---
    cpuUsage = random(15, 85);
    
    // Skifta grafpunkter åt vänster
    for (int i = 0; i < 39; i++) {
      graphPoints[i] = graphPoints[i + 1];
    }
    graphPoints[39] = map(cpuUsage, 0, 100, 70, 10); // inverterad för Y-axeln

    // --- Uppdatera CPU Cirkelmätare & Text ---
    drawGauge(42, 105, 24, (int)cpuUsage, COLOR_CYAN);
    
    // Rensa endast textområdet för CPU för att undvika flimmer
    tft.fillRect(75, 85, 85, 40, COLOR_BG);
    tft.setTextColor(COLOR_GRAY);
    tft.setTextSize(1);
    tft.setCursor(78, 88);
    tft.print("CPU USAGE:");
    
    tft.setTextColor(COLOR_WHITE);
    tft.setTextSize(2);
    tft.setCursor(78, 102);
    tft.print((int)cpuUsage);
    tft.print("%");

    // --- Uppdatera Minnesstapeln (Progress bar) ---
    int barWidth = map((int)cpuUsage, 0, 100, 2, 146);
    tft.fillRect(12, 158, 146, 12, COLOR_BG); // Rensa insidan
    tft.fillRoundRect(12, 158, barWidth, 12, 2, cpuUsage > 70 ? COLOR_RED : COLOR_GREEN);

    // --- Uppdatera Grafen ---
    tft.fillRect(11, 201, 148, 78, COLOR_BG); // Rensa insidan av grafen
    
    // Rita grid-linjer i bakgrunden av grafen
    for (int y = 220; y < 280; y += 20) {
      for (int x = 12; x < 158; x += 8) {
        tft.drawPixel(x, y, COLOR_BORDER);
      }
    }

    // Rita linjegrafen
    for (int i = 0; i < 39; i++) {
      int x1 = 12 + (i * 3.7);
      int y1 = 200 + graphPoints[i];
      int x2 = 12 + ((i + 1) * 3.7);
      int y2 = 200 + graphPoints[i + 1];
      tft.drawLine(x1, y1, x2, y2, COLOR_CYAN);
    }

    // Blinka med status-LED
    //digitalWrite(PIN_LED, !digitalRead(PIN_LED));
  }
}
