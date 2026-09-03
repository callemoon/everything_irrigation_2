#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define OLED_RESET    -1
#define OLED_ADDRESS  0x3C

// ESP32-C3
#define ADC_PIN 0       // GPIO0
#define SDA_PIN 8       // GPIO8
#define SCL_PIN 7       // GPIO7

Adafruit_SSD1306 display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire,
  OLED_RESET
);

void setup() {
  Serial.begin(115200);

  // I2C
  Wire.begin(SDA_PIN, SCL_PIN);

  // Starta OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println("Kunde inte hitta OLED!");
    while (1);
  }

  // ADC 12 bit
  analogReadResolution(12);

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(2);
  display.setCursor(15, 20);
  display.println("Startar...");

  display.display();

  delay(1000);
}

void loop() {

  // Läs ADC
  int adcValue = analogRead(ADC_PIN);

  // 1200 = torrt
  // 600  = blött
  int fuktighet = map(adcValue, 1200, 600, 0, 100);

  // Begränsa till 0-100 %
  fuktighet = constrain(fuktighet, 0, 100);


  // =========================
  // SERIAL MONITOR
  // =========================

  Serial.print("ADC: ");
  Serial.print(adcValue);

  Serial.print("   Fuktighet: ");
  Serial.print(fuktighet);
  Serial.println(" %");


  // =========================
  // OLED
  // =========================

  display.clearDisplay();

  // Rubrik
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("JORDFUKTIGHET");


  // Fuktighetsprocent
  display.setTextSize(3);

  if (fuktighet < 10) {
    display.setCursor(45, 14);
  }
  else if (fuktighet < 100) {
    display.setCursor(35, 14);
  }
  else {
    display.setCursor(25, 14);
  }

  display.print(fuktighet);
  display.print("%");


  // =========================
  // FUKTIGHETSSTAPEL
  // =========================

  int barX = 5;
  int barY = 43;
  int barWidth = 118;
  int barHeight = 12;

  // Ram
  display.drawRect(
    barX,
    barY,
    barWidth,
    barHeight,
    SSD1306_WHITE
  );

  // Beräkna hur mycket som ska fyllas
  int fillWidth = map(
    fuktighet,
    0,
    100,
    0,
    barWidth - 4
  );

  // Fyll stapeln
  if (fillWidth > 0) {
    display.fillRect(
      barX + 2,
      barY + 2,
      fillWidth,
      barHeight - 4,
      SSD1306_WHITE
    );
  }


  // =========================
  // ADC-VÄRDE
  // =========================

  display.setTextSize(1);
  display.setCursor(0, 58);

  display.print("ADC: ");
  display.print(adcValue);


  // Visa allt
  display.display();

  delay(500);
}