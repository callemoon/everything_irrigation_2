#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <BH1750.h>

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
// I2C
// ==============================
#define SDA_PIN 20
#define SCL_PIN 21

BH1750 lightMeter;

// Skicka data var 20:e sekund
const unsigned long interval = 20000;
unsigned long previousMillis = 0;


void setup()
{
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("ESP32-C3 + BH1750 + ThingSpeak");

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
  // Anslut till WiFi
  // ============================
  WiFi.begin(ssid, password);

  Serial.print("Ansluter till WiFi");

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi ansluten!");
  Serial.print("IP-adress: ");
  Serial.println(WiFi.localIP());
}


void loop()
{
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval)
  {
    previousMillis = currentMillis;

    // ==========================
    // Läs ljussensor
    // ==========================
    float lux = lightMeter.readLightLevel();

    Serial.print("Ljus: ");
    Serial.print(lux);
    Serial.println(" lux");


    // ==========================
    // Kontrollera WiFi
    // ==========================
    if (WiFi.status() == WL_CONNECTED)
    {
      HTTPClient http;

      // Bygg URL
      String url = String(thingspeakURL) + String(lux, 2);

      Serial.print("Skickar till ThingSpeak: ");
      Serial.println(url);

      // Starta HTTP
      http.begin(url);

      // GET
      int httpResponseCode = http.GET();

      // Kontrollera svar
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
      Serial.println("WiFi inte anslutet!");
    }
  }
}