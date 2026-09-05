// NOTE!!! This code has been tested on an esp32 c3 supermini, check pin config if using another board!

#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <BH1750.h>
#include "esp_sleep.h"

// ==============================
// WiFi
// ==============================
const char* ssid = "";
const char* password = "";

// ==============================
// ThingSpeak
// ==============================
const char* thingspeakURL =
  "https://api.thingspeak.com/update?api_key=";

// ==============================
// GPIO
// ==============================
#define BATTERY_PIN 1
#define LED_PIN 0

// ==============================
// Batterispänningsdelare
// 47k från batteri till GPIO1
// 10k från GPIO1 till GND
// ==============================
const float R1 = 47000.0;
const float R2 = 10000.0;

// ==============================
// I2C
// ==============================
#define SDA_PIN 20
#define SCL_PIN 21

BH1750 lightMeter;

// ==============================
// Deep sleep
// ==============================
const uint64_t SLEEP_TIME_US = 30ULL * 1000000ULL;


// ==========================================
// Läs batterispänning
// ==========================================
float readBatteryVoltage()
{
  // LED PÅ under mätningen
  digitalWrite(LED_PIN, HIGH);

  delay(5);

  // Läs ADC flera gånger
  const int samples = 10;
  uint32_t sum = 0;

  for (int i = 0; i < samples; i++)
  {
    sum += analogRead(BATTERY_PIN);
    delay(2);
  }

  float adcValue = sum / (float)samples;

  // Omvandla ADC-värdet till spänning
  float adcVoltage = (adcValue / 4095.0) * 3.3;

  // Räkna tillbaka batterispänningen
  float batteryVoltage =
    adcVoltage * ((R1 + R2) / R2);

  // LED AV direkt efter mätningen
  digitalWrite(LED_PIN, LOW);

  return batteryVoltage;
}


// ==========================================
// Deep sleep
// ==========================================
void goToDeepSleep()
{
  Serial.println("Går till deep sleep...");
  Serial.flush();

  esp_sleep_enable_timer_wakeup(SLEEP_TIME_US);

  esp_deep_sleep_start();
}


void setup()
{
  Serial.begin(115200);
  delay(500);

  Serial.println();
  Serial.println("ESP32-C3 + BH1750 + Batteri + ThingSpeak");

  // ============================
  // GPIO
  // ============================
  pinMode(BATTERY_PIN, INPUT);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  analogReadResolution(12);

  // ============================
  // Starta I2C
  // ============================
  Wire.begin(SDA_PIN, SCL_PIN);

  // ============================
  // Starta BH1750
  // ============================
  if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE))
  {
    Serial.println("BH1750 startad!");
  }
  else
  {
    Serial.println("FEL: BH1750 hittades inte!");

    while (1)
    {
      delay(1000);
    }
  }

  // ============================
  // Läs batterispänning
  // ============================
  float batteryVoltage = readBatteryVoltage();

  Serial.print("Batteri: ");
  Serial.print(batteryVoltage, 2);
  Serial.println(" V");

  // ============================
  // Läs ljus
  // ============================
  float lux = lightMeter.readLightLevel();

  Serial.print("Ljus: ");
  Serial.print(lux);
  Serial.println(" lux");

  // ============================
  // Anslut till WiFi
  // ============================
  WiFi.begin(ssid, password);

  Serial.print("Ansluter till WiFi");

  unsigned long startTime = millis();

  while (WiFi.status() != WL_CONNECTED &&
         millis() - startTime < 10000)
  {
    delay(250);
    Serial.print(".");
  }

  Serial.println();

  // ============================
  // Skicka till ThingSpeak
  // ============================
  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("WiFi ansluten!");

    HTTPClient http;

    String url = String(thingspeakURL) +
                 "&field1=" + String(lux, 2) +
                 "&field2=" + String(batteryVoltage, 2);

    Serial.print("Skickar till ThingSpeak: ");
    Serial.println(url);

    http.begin(url);

    int httpResponseCode = http.GET();

    if (httpResponseCode > 0)
    {
      Serial.print("HTTP svar: ");
      Serial.println(httpResponseCode);

      String response = http.getString();

      Serial.print("ThingSpeak svar: ");
      Serial.println(response);
    }
    else
    {
      Serial.print("HTTP-fel: ");
      Serial.println(httpResponseCode);
    }

    http.end();
  }
  else
  {
    Serial.println("WiFi kunde inte anslutas!");
  }

  // ============================
  // Stäng av WiFi
  // ============================
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);

  // ============================
  // Sov i 30 sekunder
  // ============================
  goToDeepSleep();
}


void loop()
{
  // Används inte.
  // ESP32 startar om från setup()
  // efter deep sleep.
}
