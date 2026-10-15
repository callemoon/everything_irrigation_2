#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define OLED_SDA 8    // ändra vid behov
#define OLED_SCL 9    // ändra vid behov

#define ADC_PIN 3     // välj valfri ADC-ingång

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

unsigned long startTime;

void setup()
{
    Serial.begin(115200);

    Wire.begin(OLED_SDA, OLED_SCL);

    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
    {
        Serial.println("OLED init failed");
        while (1);
    }

    analogReadResolution(12);

    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);

    startTime = millis();
}

void loop()
{
    int raw = analogRead(ADC_PIN);

    // ESP32-S3 ADC referens ~3.3 V
    float voltage = (raw * 3.3f) / 4095.0f;

    unsigned long seconds = (millis() - startTime) / 1000;

    display.clearDisplay();

    display.setTextSize(2);
    display.setCursor(0, 0);
    display.print(voltage, 3);
    display.println(" V");

    display.setTextSize(1);
    display.setCursor(0, 35);
    display.print("Time: ");
    display.print(seconds);
    display.println(" s");

    display.display();

    Serial.print("ADC=");
    Serial.print(raw);
    Serial.print("  Voltage=");
    Serial.print(voltage, 3);
    Serial.print(" V  Time=");
    Serial.print(seconds);
    Serial.println(" s");

    delay(500);
}
