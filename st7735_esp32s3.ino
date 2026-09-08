#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>

// ==========================================
// KORREKTA PINNAR ENLIGT WAVESHARE PINOUTS
// ==========================================
#define PIN_LED     5     // C5 GP4  -> S3 GP5
#define TFT_SCLK    8     // C5 GP7  -> S3 GP8
#define TFT_MOSI    9     // C5 GP8  -> S3 GP9
#define TFT_CS      43    // C5 GP11 -> S3 TX (GPIO 43)
#define TFT_DC      44    // C5 GP12 -> S3 RX (GPIO 44)
#define TFT_RST    -1     // Hårdvarubunden till 3.3V

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
  // 1. Status-LED
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LOW);

  // 2. Tvinga GPIO 43 och 44 till vanliga utgångar (koppla loss från UART0)
  pinMode(TFT_CS, OUTPUT);
  pinMode(TFT_DC, OUTPUT);
  digitalWrite(TFT_CS, HIGH);

  // 3. SPI på 8 MHz
  spiDisplay.begin(TFT_SCLK, -1, TFT_MOSI, -1);
  spiDisplay.setFrequency(8000000);

  // 4. Initiera display
  tft.initR(INITR_BLACKTAB);

  tft.sendCommand(ST77XX_SWRESET);
  delay(150);
  tft.sendCommand(ST77XX_SLPOUT);
  delay(150);

  // Tvinga 16-bit färg (RGB565)
  tft.sendCommand(0x3A);
  tft.spiWrite(0x05);
  delay(10);

  tft.sendCommand(ST77XX_DISPON);
  delay(100);

  // 5. Rotation och offset
  tft.setRotation(1);
  tft.setRealOffset(1, 2);

  // 6. Rita layout
  tft.fillScreen(ST77XX_BLACK);
  tft.drawRect(0, 0, 160, 128, ST77XX_RED);

  tft.setTextColor(ST77XX_GREEN);
  tft.setTextSize(2);
  tft.setCursor(15, 25);
  tft.println("ESP32-S3");

  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);
  tft.setCursor(15, 60);
  tft.println("1.8\" 160x128 ST7735S");
  tft.setCursor(15, 75);
  tft.println("Status: 100% rätt sockel!");
}

void loop() {
  digitalWrite(PIN_LED, HIGH);
  delay(500);
  digitalWrite(PIN_LED, LOW);
  delay(500);
}