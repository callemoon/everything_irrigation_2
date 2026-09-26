#include <Arduino.h>
#include <SPI.h>
#include <WiFi.h>
#include <time.h>

#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>

#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>

// =====================================================
// TFT PINS
// =====================================================

#define TFT_SCLK    0
#define TFT_MOSI    1
#define TFT_RST     2
#define TFT_DC      3
#define TFT_CS      4

#define PIN_LED     10

// =====================================================
// WIFI
// =====================================================

const char* ssid     = "";
const char* password = "";

// =====================================================
// DISPLAY
// =====================================================

class ST7735_Custom : public Adafruit_ST7735
{
public:
    ST7735_Custom(SPIClass *spiClass,
                  int8_t cs,
                  int8_t dc,
                  int8_t rst)
        : Adafruit_ST7735(spiClass, cs, dc, rst)
    {
    }

    void setRealOffset(int8_t x, int8_t y)
    {
        _xstart = x;
        _ystart = y;
    }
};

SPIClass spiDisplay(FSPI);
ST7735_Custom tft(&spiDisplay, TFT_CS, TFT_DC, TFT_RST);

// Full framebuffer 160x128x16-bit ≈ 40 kB
GFXcanvas16 canvas(160, 128);

int lastSecond = -1;

// =====================================================

void drawWifiBars(int rssi)
{
    int bars = 0;

    if (rssi > -90) bars = 1;
    if (rssi > -80) bars = 2;
    if (rssi > -70) bars = 3;
    if (rssi > -60) bars = 4;

    uint16_t color;

    if (rssi > -60)
        color = ST77XX_GREEN;
    else if (rssi > -75)
        color = ST77XX_YELLOW;
    else
        color = ST77XX_RED;

    for (int i = 0; i < 4; i++)
    {
        int x = 135 + (i * 5);
        int h = 3 + (i * 3);

        if (i < bars)
        {
            canvas.fillRect(
                x,
                16 - h,
                3,
                h,
                color);
        }
        else
        {
            canvas.drawRect(
                x,
                16 - h,
                3,
                h,
                ST77XX_WHITE);
        }
    }
}

// =====================================================

void drawCenteredText(
    const char* txt,
    const GFXfont* font,
    uint16_t color,
    int y)
{
    canvas.setFont(font);
    canvas.setTextColor(color);

    int16_t x1, y1;
    uint16_t w, h;

    canvas.getTextBounds(
        txt,
        0,
        0,
        &x1,
        &y1,
        &w,
        &h);

    int x = (160 - w) / 2;

    canvas.setCursor(x, y);
    canvas.print(txt);
}

// =====================================================

void renderScreen(
    const char* timeString,
    const char* dateString)
{
    canvas.fillScreen(ST77XX_BLACK);

    drawWifiBars(WiFi.RSSI());

    drawCenteredText(
        timeString,
        &FreeSansBold18pt7b,
        ST77XX_GREEN,
        58);

    drawCenteredText(
        dateString,
        &FreeSansBold9pt7b,
        ST77XX_CYAN,
        92);

    tft.drawRGBBitmap(
        0,
        0,
        canvas.getBuffer(),
        160,
        128);
}

// =====================================================

void connectWifiAndTime()
{
    canvas.fillScreen(ST77XX_BLACK);

    canvas.setTextColor(ST77XX_WHITE);
    canvas.setTextSize(1);

    canvas.setCursor(10, 40);
    canvas.print("Connecting WiFi...");

    tft.drawRGBBitmap(
        0,
        0,
        canvas.getBuffer(),
        160,
        128);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
    }

    configTzTime(
        "CET-1CEST,M3.5.0/02,M10.5.0/03",
        "se.pool.ntp.org",
        "pool.ntp.org");

    struct tm timeinfo;

    while (!getLocalTime(&timeinfo))
    {
        delay(500);
    }
}

// =====================================================

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

    spiDisplay.begin(
        TFT_SCLK,
        -1,
        TFT_MOSI,
        -1);

    spiDisplay.setFrequency(80000000);

    tft.initR(INITR_BLACKTAB);

    tft.setRotation(1);
    tft.setRealOffset(1, 2);

    connectWifiAndTime();
}

// =====================================================

void loop()
{
    struct tm timeinfo;

    if (getLocalTime(&timeinfo))
    {
        if (timeinfo.tm_sec != lastSecond)
        {
            lastSecond = timeinfo.tm_sec;

            char timeString[16];
            char dateString[20];

            strftime(
                timeString,
                sizeof(timeString),
                "%H:%M:%S",
                &timeinfo);

            strftime(
                dateString,
                sizeof(dateString),
                "%Y-%m-%d",
                &timeinfo);

            renderScreen(
                timeString,
                dateString);

            digitalWrite(
                PIN_LED,
                !digitalRead(PIN_LED));
        }
    }

    delay(20);
}
