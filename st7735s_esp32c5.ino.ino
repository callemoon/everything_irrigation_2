#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>

#include "soc/usb_serial_jtag_reg.h"
#include "soc/soc.h"

// ==========================================
// PIN-MAPPNING (ESP32-C5)
// ==========================================
#define PIN_LED    4     // Statusdiod
#define TFT_SCLK   7     // SPI SCK (Klocka)
#define TFT_MOSI   8     // SPI MOSI (Data)
#define TFT_DC     12    // Data / Command
#define TFT_CS    -1     // Hårdvarubunden till GND
#define TFT_RST   -1     // Hårdvarubunden till 3.3V

// Subklass för att sätta hårdvaruoffseten direkt
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

  // 2. Frigör USB-padsen i hårdvaran
  CLEAR_PERI_REG_MASK(
    USB_SERIAL_JTAG_CONF0_REG,
    USB_SERIAL_JTAG_USB_PAD_ENABLE
  );

  // 3. SPI på 8 MHz
  spiDisplay.begin(TFT_SCLK, -1, TFT_MOSI, -1);
  spiDisplay.setFrequency(8000000);

  // 4. Initiera med BlackTab
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

  // 5. Rotation och verifierad pixel-offset
  tft.setRotation(1);
  tft.setRealOffset(1, 2);

  // 6. Rita layout
  tft.fillScreen(ST77XX_BLACK);

  // Perfekt inramning (160x128)
  tft.drawRect(0, 0, 160, 128, ST77XX_RED);

  tft.setTextColor(ST77XX_GREEN);
  tft.setTextSize(2);
  tft.setCursor(15, 25);
  tft.println("ESP32-C5");

  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);
  tft.setCursor(15, 60);
  tft.println("1.8\" 160x128 ST7735S");
  tft.setCursor(15, 75);
  tft.println("Status: Kalibrerad & klar!");
}

void loop() {
  digitalWrite(PIN_LED, HIGH);
  delay(500);
  digitalWrite(PIN_LED, LOW);
  delay(500);
}