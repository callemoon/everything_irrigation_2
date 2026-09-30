#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Skärminställningar
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1  // Delas ofta med ESP32 reset
#define SCREEN_ADDRESS 0x3C // Vanligaste I2C-adressen (ibland 0x3D)

// Ange dina I2C-pinnar för C3
#define SDA_PIN 0
#define SCL_PIN 1

#define LED_BUILTIN 4  // Ändra om din LED sitter på annan GPIO

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const char* ssid = "ESP32-C3-AP";
const char* password = "Lösenord123";

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  // Starta I2C med anpassade pinnar
  Wire.begin(SDA_PIN, SCL_PIN);

  // Starta OLED-skärmen
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("Kunde inte hitta SSD1306 OLED-skärmen!"));
  } else {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("ESP32-C3 Wi-Fi Test");
    display.println("Ansluter...");
    display.display();
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  // Vänta på anslutning med visuell indikering på skärmen
  int dots = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    dots = (dots + 1) % 4;
    
    display.clearDisplay();
    display.setCursor(0, 10);
    display.setTextSize(1);
    display.print("Ansluter till AP");
    for(int i = 0; i < dots; i++) display.print(".");
    display.display();
  }

  Serial.println("\nAnsluten!");
}

void loop() {
  // Blinkar med LED (100 ms)
  digitalWrite(LED_BUILTIN, HIGH);
  delay(100);
  digitalWrite(LED_BUILTIN, LOW);

  display.clearDisplay();

  if (WiFi.status() == WL_CONNECTED) {
    int32_t rssi = WiFi.RSSI();

    // 1. Rubrik
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("AP: ");
    display.println(ssid);

    // 2. RSSI-värde i stor text
    display.setTextSize(2);
    display.setCursor(0, 18);
    display.print(rssi);
    display.setTextSize(1);
    display.print(" dBm");

    // 3. Texttolkning av signalstyrkan
    display.setCursor(0, 42);
    if (rssi >= -50) {
      display.println("Utmarkt signal");
    } else if (rssi >= -65) {
      display.println("God signal");
    } else if (rssi >= -75) {
      display.println("Medelstark signal");
    } else {
      display.println("Svag signal");
    }

    // 4. Enkel signalstapel längst ner
    int barWidth = map(constrain(rssi, -90, -30), -90, -30, 0, 128);
    display.fillRect(0, 56, barWidth, 8, SSD1306_WHITE);

    // Skriv ut till Serial också
    Serial.print("RSSI: ");
    Serial.print(rssi);
    Serial.println(" dBm");

  } else {
    display.setTextSize(1);
    display.setCursor(0, 20);
    display.println("Tappade anslutning!");
  }

  display.display(); // Uppdatera skärmen med ny data

  delay(900); // Totalt 1 sekund per cykel
}
