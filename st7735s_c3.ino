#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>

// ==========================================
// PINNAR ANPASSADE FÖR DIN KLOCKRENA SOCKEL
// ==========================================
#define TFT_SCLK    0
#define TFT_MOSI    1
#define TFT_RST     2     // OBS: Strapping pin – hanteras försiktigt nedan
#define TFT_DC      3
#define TFT_CS      4
#define PIN_LED     4

class ST7735_Custom : public Adafruit_ST7735 {
  public:
    ST7735_Custom(SPIClass *spiClass, int8_t cs, int8_t dc, int8_t rst)
      : Adafruit_ST7735(spiClass, cs, dc, rst) {}

    void setRealOffset(int8_t x, int8_t y) {
      _xstart = x;
      _ystart = y;
    }
};

SPIClass spiDisplay(FSPI);
ST7735_Custom tft = ST7735_Custom(&spiDisplay, TFT_CS, TFT_DC, TFT_RST);

void setup() {
  // 1. Sätt LED
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, HIGH);

  // 2. VÄNTA med GPIO 2 så att ESP32-C3 hinner boota säkert utan krasch
  delay(300);

  // 3. Konfigurera styrpinnar
  pinMode(TFT_CS, OUTPUT);
  pinMode(TFT_DC, OUTPUT);
  pinMode(TFT_RST, OUTPUT);
  
  digitalWrite(TFT_CS, HIGH);
  
  // Gör en manuell, kontrollerad reset-puls
  digitalWrite(TFT_RST, HIGH);
  delay(10);
  digitalWrite(TFT_RST, LOW);
  delay(20);
  digitalWrite(TFT_RST, HIGH);
  delay(150);

  // 4. Starta SPI på GPIO 0 & 1
  spiDisplay.begin(TFT_SCLK, -1, TFT_MOSI, -1);
  spiDisplay.setFrequency(8000000);

  // 5. Initiera skärmen
  tft.initR(INITR_BLACKTAB);

  tft.sendCommand(ST77XX_SWRESET);
  delay(150);
  tft.sendCommand(ST77XX_SLPOUT);
  delay(150);

  // 16-bit färg
  tft.sendCommand(0x3A);
  tft.spiWrite(0x05);
  delay(10);

  tft.sendCommand(ST77XX_DISPON);
  delay(100);

  // 6. Layout
  tft.setRotation(1);
  tft.setRealOffset(1, 2);

  tft.fillScreen(ST77XX_BLACK);
  tft.drawRect(0, 0, 160, 128, ST77XX_RED);

  tft.setTextColor(ST77XX_GREEN);
  tft.setTextSize(2);
  tft.setCursor(15, 25);
  tft.println("ESP32-C3");

  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);
  tft.setCursor(15, 60);
  tft.println("1.8\" 160x128 ST7735S");
  tft.setCursor(15, 75);
  tft.println("Status: Sockel OK!");
}

void loop() {
  digitalWrite(PIN_LED, HIGH);
  delay(500);
  digitalWrite(PIN_LED, LOW);
  delay(500);
}
