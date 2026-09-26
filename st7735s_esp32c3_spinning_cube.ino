#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>

#define TFT_SCLK  0
#define TFT_MOSI  1
#define TFT_RST   2
#define TFT_DC    3
#define TFT_CS    4

class ST7735_Custom : public Adafruit_ST7735 {
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

// Kubens hörn
float cube[8][3] =
{
    {-1, -1, -1},
    { 1, -1, -1},
    { 1,  1, -1},
    {-1,  1, -1},
    {-1, -1,  1},
    { 1, -1,  1},
    { 1,  1,  1},
    {-1,  1,  1}
};

const uint8_t edges[12][2] =
{
    {0,1},{1,2},{2,3},{3,0},
    {4,5},{5,6},{6,7},{7,4},
    {0,4},{1,5},{2,6},{3,7}
};

float angleX = 0;
float angleY = 0;
float angleZ = 0;

void setup()
{
    spiDisplay.begin(TFT_SCLK, -1, TFT_MOSI, -1);
    spiDisplay.setFrequency(80000000);

    tft.initR(INITR_BLACKTAB);
    tft.setRotation(1);
    tft.setRealOffset(1, 2);

    tft.fillScreen(ST77XX_BLACK);
}

void loop()
{
    canvas.fillScreen(ST77XX_BLACK);

    int sx[8];
    int sy[8];

    for (int i = 0; i < 8; i++)
    {
        float x = cube[i][0];
        float y = cube[i][1];
        float z = cube[i][2];

        // Rotation X
        float y1 = y * cos(angleX) - z * sin(angleX);
        float z1 = y * sin(angleX) + z * cos(angleX);

        // Rotation Y
        float x2 = x * cos(angleY) + z1 * sin(angleY);
        float z2 =-x * sin(angleY) + z1 * cos(angleY);

        // Rotation Z
        float x3 = x2 * cos(angleZ) - y1 * sin(angleZ);
        float y3 = x2 * sin(angleZ) + y1 * cos(angleZ);

        // Perspektiv
        float distance = 6.0f;
        float scale = 120.0f;

        float factor = scale / (z2 + distance);

        sx[i] = (int)(x3 * factor + 80);
        sy[i] = (int)(y3 * factor + 64);
    }

    // Rita alla kanter
    for (int i = 0; i < 12; i++)
    {
        canvas.drawLine(
            sx[edges[i][0]],
            sy[edges[i][0]],
            sx[edges[i][1]],
            sy[edges[i][1]],
            ST77XX_GREEN);
    }

    // Hörnpunkter
    for (int i = 0; i < 8; i++)
    {
        canvas.fillCircle(
            sx[i],
            sy[i],
            2,
            ST77XX_RED);
    }

    tft.drawRGBBitmap(
        0,
        0,
        canvas.getBuffer(),
        160,
        128);

    angleX += 0.03f;
    angleY += 0.02f;
    angleZ += 0.01f;
}
