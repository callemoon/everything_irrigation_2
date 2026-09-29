#include <Arduino.h>
#include <SPI.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>

#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <Fonts/FreeSansBold18pt7b.h>

#define TFT_SCLK 0
#define TFT_MOSI 1
#define TFT_RST  2
#define TFT_DC   3
#define TFT_CS   4
#define PIN_LED  10

#ifndef COLOR_ORANGE
#define COLOR_ORANGE 0xFD20
#endif

const char* ssid     = "";
const char* password = "";

// =====================================================
// THINGSPEAK
// =====================================================

#define THINGSPEAK_CHANNEL 2019519
#define THINGSPEAK_API_KEY ""

class ST7735_Custom : public Adafruit_ST7735
{
public:
    ST7735_Custom(SPIClass *spiClass, int8_t cs, int8_t dc, int8_t rst)
        : Adafruit_ST7735(spiClass, cs, dc, rst) {}

    void setRealOffset(int8_t x, int8_t y)
    {
        _xstart = x;
        _ystart = y;
    }
};

SPIClass spiDisplay(FSPI);
ST7735_Custom tft(&spiDisplay, TFT_CS, TFT_DC, TFT_RST);
GFXcanvas16 canvas(160, 128);

float outdoorTemp = 0.0f;
char measurementTime[16] = "--:--:--";
char measurementDate[16] = "0000-00-00";
unsigned long lastUpdate = 0;

void drawWifiBars(int rssi)
{
    int bars = 0;
    if (rssi > -90) bars = 1;
    if (rssi > -80) bars = 2;
    if (rssi > -70) bars = 3;
    if (rssi > -60) bars = 4;

    uint16_t color = ST77XX_RED;
    if (rssi > -60) color = ST77XX_GREEN;
    else if (rssi > -75) color = ST77XX_YELLOW;

    for (int i = 0; i < 4; i++)
    {
        int x = 135 + i * 5;
        int h = 3 + i * 3;

        if (i < bars)
            canvas.fillRect(x, 16 - h, 3, h, color);
        else
            canvas.drawRect(x, 16 - h, 3, h, ST77XX_WHITE);
    }
}

bool updateTemperature()
{
    HTTPClient http;

    String url =
        "https://api.thingspeak.com/channels/" +
        String(THINGSPEAK_CHANNEL) +
        "/feeds/last.json?api_key=" +
        String(THINGSPEAK_API_KEY);

    http.begin(url);
    int httpCode = http.GET();

    if (httpCode != HTTP_CODE_OK)
    {
        http.end();
        return false;
    }

    String payload = http.getString();
    http.end();

    DynamicJsonDocument doc(4096);

    if (deserializeJson(doc, payload))
        return false;

    outdoorTemp = String(doc["field1"]).toFloat();

    String created = doc["created_at"].as<String>();

    int year   = created.substring(0, 4).toInt();
    int month  = created.substring(5, 7).toInt();
    int day    = created.substring(8,10).toInt();
    int hour   = created.substring(11,13).toInt();
    int minute = created.substring(14,16).toInt();
    int second = created.substring(17,19).toInt();

    struct tm t = {};
    t.tm_year = year - 1900;
    t.tm_mon  = month - 1;
    t.tm_mday = day;
    t.tm_hour = hour;
    t.tm_min  = minute;
    t.tm_sec  = second;

    time_t epoch = mktime(&t);
    struct tm localTm;
    localtime_r(&epoch, &localTm);

    snprintf(measurementTime, sizeof(measurementTime), "%02d:%02d:%02d",
             localTm.tm_hour, localTm.tm_min, localTm.tm_sec);

    snprintf(measurementDate, sizeof(measurementDate), "%04d-%02d-%02d",
             localTm.tm_year + 1900,
             localTm.tm_mon + 1,
             localTm.tm_mday);

    return true;
}

void renderScreen()
{
    canvas.fillScreen(ST77XX_BLACK);

    drawWifiBars(WiFi.RSSI());

    canvas.fillRoundRect(8,20,6,88,2,ST77XX_RED);
    canvas.fillRoundRect(9,20,4,88,2,COLOR_ORANGE);
    canvas.fillRoundRect(10,20,2,88,2,ST77XX_YELLOW);

    canvas.setFont(NULL);
    canvas.setTextSize(1);
    canvas.setTextColor(ST77XX_WHITE);
    canvas.setCursor(55,12);
    canvas.print("OUTDOOR");

    char tempString[16];
    snprintf(tempString,sizeof(tempString),"%.1f",outdoorTemp);

    canvas.setFont(&FreeSansBold18pt7b);
    canvas.setTextColor(COLOR_ORANGE);

    int16_t x1,y1;
    uint16_t w,h;

    canvas.getTextBounds(tempString,0,0,&x1,&y1,&w,&h);

    int tempX = (160 - w) / 2 - 8;

    canvas.setCursor(tempX,62);
    canvas.print(tempString);

    canvas.drawCircle(tempX + w + 5, 39, 2, COLOR_ORANGE);

    canvas.setFont(NULL);
    canvas.setTextSize(1);
    canvas.setCursor(tempX + w + 10, 50);
    canvas.print("C");

    canvas.setTextColor(ST77XX_GREEN);
    canvas.setCursor(56,88);
    canvas.print(measurementTime);

    canvas.setTextColor(ST77XX_CYAN);
    canvas.setCursor(48,103);
    canvas.print(measurementDate);

    tft.drawRGBBitmap(0,0,canvas.getBuffer(),160,128);
}

void connectWifi()
{
    canvas.fillScreen(ST77XX_BLACK);
    canvas.setTextColor(ST77XX_WHITE);
    canvas.setTextSize(1);
    canvas.setCursor(10,40);
    canvas.print("Connecting WiFi...");

    tft.drawRGBBitmap(0,0,canvas.getBuffer(),160,128);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED)
        delay(500);

    configTzTime(
        "CET-1CEST,M3.5.0/02,M10.5.0/03",
        "pool.ntp.org");
}

void setup()
{
    Serial.begin(115200);

    pinMode(PIN_LED, OUTPUT);

    delay(300);

    pinMode(TFT_CS, OUTPUT);
    pinMode(TFT_DC, OUTPUT);
    pinMode(TFT_RST, OUTPUT);

    digitalWrite(TFT_CS, HIGH);

    digitalWrite(TFT_RST, HIGH);
    delay(10);
    digitalWrite(TFT_RST, LOW);
    delay(20);
    digitalWrite(TFT_RST, HIGH);
    delay(150);

    spiDisplay.begin(TFT_SCLK, -1, TFT_MOSI, -1);
    spiDisplay.setFrequency(80000000);

    tft.initR(INITR_BLACKTAB);
    tft.setRotation(1);
    tft.setRealOffset(1, 2);

    connectWifi();

    updateTemperature();
    renderScreen();
}

void loop()
{
    if ((lastUpdate == 0) || (millis() - lastUpdate > 60000UL))
    {
        lastUpdate = millis();

        if (updateTemperature())
        {
            renderScreen();
            digitalWrite(PIN_LED, !digitalRead(PIN_LED));
        }
    }

    delay(100);
}
