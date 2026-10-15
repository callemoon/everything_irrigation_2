#include <Wire.h>
#include <U8g2lib.h>
#include <WiFi.h>
#include <HTTPClient.h>

// --- WiFi & ThingSpeak Inställningar ---
const char* ssid = "";        // Byt till ditt WiFi-namn
const char* password = ""; // Byt till ditt WiFi-lösenord

const char* server = "http://api.thingspeak.com/update";
const char* writeAPIKey = "";   // Byt till din ThingSpeak Write API Key

unsigned long lastThingSpeakUpdate = 0;
const unsigned long thingSpeakInterval = 60000; // 60 sekunder (1 minut) i millisekunder

// Skapa objekt för 1.3" I2C OLED med SH1106-drivrutin (128x64)
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

// ADC-ingång
#define ADC_PIN 3

// Lastmotstånd
const float LOAD_RESISTANCE = 2.0f;
const float VOLTAGE_MULTIPLIER = 1.0f;

float consumed_mAh = 0.0f;
unsigned long startTime;
unsigned long lastUpdate;

void setup() {
    Serial.begin(115200);

    // Initiera I2C på pinnar 8 och 9
    Wire.begin(8, 9);
    
    // Starta skärmen
    u8g2.begin();

    analogReadResolution(12);

    // Anslut till WiFi
    WiFi.begin(ssid, password);
    Serial.print("Ansluter till WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi anslutet!");

    startTime = millis();
    lastUpdate = millis();
}

void loop() {
    unsigned long now = millis();

    // Läs ADC
    uint16_t raw = analogRead(ADC_PIN);
    float adcVoltage = ((float)raw * 3.3f) / 4095.0f;
    float batteryVoltage = adcVoltage * VOLTAGE_MULTIPLIER;

    // Beräkna ström
    float current_mA = (batteryVoltage / LOAD_RESISTANCE) * 1000.0f;

    // Integrera mAh
    float elapsedHours = (now - lastUpdate) / 3600000.0f;
    consumed_mAh += current_mA * elapsedHours;
    lastUpdate = now;

    unsigned long elapsedSeconds = (now - startTime) / 1000;

    // --- Skicka till ThingSpeak via HTTP en gång i minuten ---
    if (now - lastThingSpeakUpdate >= thingSpeakInterval) {
        lastThingSpeakUpdate = now;

        if (WiFi.status() == WL_CONNECTED) {
            HTTPClient http;
            
            // Bygg URL:en med API-nyckel och fält (field1 = spänning, field2 = ström, field3 = mAh)
            String url = String(server) + "?api_key=" + writeAPIKey + 
                         "&field1=" + String(batteryVoltage, 3) + 
                         "&field2=" + String(current_mA, 0) + 
                         "&field3=" + String(consumed_mAh, 1);

            http.begin(url);
            int httpResponseCode = http.GET();

            if (httpResponseCode > 0) {
                Serial.println("Data skickad till ThingSpeak! Svarskod: " + String(httpResponseCode));
            } else {
                Serial.println("Fel vid skickning, HTTP-felkod: " + String(httpResponseCode));
            }
            http.end();
        } else {
            Serial.println("WiFi frånkopplat, kan inte skicka till ThingSpeak.");
        }
    }

    // Rita på OLED med U8g2
    u8g2.firstPage();
    do {
        u8g2.setFont(u8g2_font_fub20_tr); 
        u8g2.setCursor(0, 22);
        u8g2.print(batteryVoltage, 2);
        u8g2.print("V");

        u8g2.setFont(u8g2_font_ncenB08_tr);
        
        u8g2.setCursor(0, 38);
        u8g2.print("I: ");
        u8g2.print(current_mA, 0);
        u8g2.print("mA");

        u8g2.setCursor(0, 50);
        u8g2.print("Used: ");
        u8g2.print(consumed_mAh, 0);
        u8g2.print("mAh");

        u8g2.setCursor(0, 62);
        u8g2.print("Time: ");
        u8g2.print(elapsedSeconds);
        u8g2.print("s");
        
    } while (u8g2.nextPage());

    // Seriell utskrift
    Serial.print("V=");
    Serial.print(batteryVoltage, 3);
    Serial.print(" I=");
    Serial.print(current_mA, 0);
    Serial.print("mA Used=");
    Serial.print(consumed_mAh, 1);
    Serial.print("mAh Time=");
    Serial.print(elapsedSeconds);
    Serial.println("s");

    delay(500);
}
