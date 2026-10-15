#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// OLED-pinnar (justera vid behov)
#define OLED_SDA 8
#define OLED_SCL 9

// ADC-ingång
#define ADC_PIN 3

// SSD1306 I2C-adress
#define OLED_ADDR 0x3C

// Lastmotstånd
const float LOAD_RESISTANCE = 2.0f;

// Om du använder spänningsdelare:
// 1.0 = ingen spänningsdelare
// 2.0 = 10k + 10k
const float VOLTAGE_MULTIPLIER = 1.0f;

Adafruit_SSD1306 display(
    SCREEN_WIDTH,
    SCREEN_HEIGHT,
    &Wire,
    -1);

float consumed_mAh = 0.0f;

unsigned long startTime;
unsigned long lastUpdate;

void setup()
{
    Serial.begin(115200);

    Wire.begin(OLED_SDA, OLED_SCL);

    if (!display.begin(
            SSD1306_SWITCHCAPVCC,
            OLED_ADDR))
    {
        Serial.println("OLED init failed");
        while (1);
    }

    analogReadResolution(12);

    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);

    startTime = millis();
    lastUpdate = millis();
}

void loop()
{
    unsigned long now = millis();

    // Läs ADC
    uint16_t raw = analogRead(ADC_PIN);

    // ADC -> Volt
    float adcVoltage =
        ((float)raw * 3.3f) / 4095.0f;

    float batteryVoltage =
        adcVoltage * VOLTAGE_MULTIPLIER;

    // Beräkna ström genom lasten
    float current_mA =
        (batteryVoltage / LOAD_RESISTANCE) * 1000.0f;

    // Integrera mAh
    float elapsedHours =
        (now - lastUpdate) / 3600000.0f;

    consumed_mAh +=
        current_mA * elapsedHours;

    lastUpdate = now;

    unsigned long elapsedSeconds =
        (now - startTime) / 1000;

    // OLED
    display.clearDisplay();

    display.setTextSize(2);
    display.setCursor(0, 0);
    display.print(batteryVoltage, 2);
    display.println("V");

    display.setTextSize(1);

    display.setCursor(0, 25);
    display.print("I:");
    display.print(current_mA, 0);
    display.println("mA");

    display.setCursor(0, 38);
    display.print("Used:");
    display.print(consumed_mAh, 0);
    display.println("mAh");

    display.setCursor(0, 51);
    display.print("Time:");
    display.print(elapsedSeconds);
    display.print("s");

    display.display();

    // Seriell utskrift
    Serial.print("V=");
    Serial.print(batteryVoltage, 3);

    Serial.print("  I=");
    Serial.print(current_mA, 0);

    Serial.print("mA  Used=");
    Serial.print(consumed_mAh, 1);

    Serial.print("mAh  Time=");
    Serial.print(elapsedSeconds);

    Serial.println("s");

    delay(500);
}
