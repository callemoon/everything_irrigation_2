#include <Arduino.h>
#include <SPI.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <lvgl.h>
#include <time.h>

// =====================================================
// WIFI
// =====================================================
const char* WIFI_SSID = "";
const char* WIFI_PASS = "";

// =====================================================
// NTP / TID
// =====================================================
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;

// =====================================================
// THINGSPEAK-KANALER (Totalt 8 st)
// =====================================================
struct ChannelConfig {
  const char *name;
  uint32_t channelId;
  uint8_t field;
  const char *apiKey;
  const char *unit;
  uint8_t decimals;
};

ChannelConfig channels[] = {
  {"sovrum",                  631325,  1, "IA9TGDTR2351DC1I", "C",  1},
  {"overvaning",              2011583, 1, "EL96SBAO527BKNP3", "C",  1},
  {"ute",                     2013865, 1, "PLWLFP0HG4TE9BNZ", "C",  1},
  {"kok",                     2014046, 1, "68ODS47UMTXTWR9K", "C",  1},
  {"outdoor temp",            2019519, 1, "3HZ51EFUGWRX9DBQ", "C",  1}, // Ny (antar fält 1 för temp, ändra field om det är ett annat)
  {"water depth",             3422161, 1, "0SI22N5VUS0UEBUD", "cm", 0}, // Ny (satt till fält 1, justera vid behov)
  {"rain",                    3424144, 1, "7QXA4BYHS5MNVMKY", "mm", 1}, // Ny
  {"light",                   3482060, 1, "A9Z2KGTLFR5KEV1Z", "lx", 0}  // Ny
};

constexpr int NUM_CHANNELS = sizeof(channels) / sizeof(channels[0]);

// =====================================================
// ESP32-C3 / ST7789 & LJUSSTYRKA
// =====================================================
#define TFT_SCLK 0
#define TFT_MOSI 1
#define TFT_RST  2
#define TFT_DC   3
#define TFT_CS   4
#define PIN_LED  5

const int SCREEN_BRIGHTNESS = 130; 

constexpr int SCREEN_W = 320;
constexpr int SCREEN_H = 170;

// =====================================================
// FÄRGER
// =====================================================
#define COLOR_BG       0x0410
#define COLOR_PANEL    0x0A28
#define COLOR_BORDER   0x1926
#define COLOR_CYAN     0x3CE7
#define COLOR_ORANGE   0xCBE0
#define COLOR_GREEN    0x3CF4
#define COLOR_PURPLE   0x87FD
#define COLOR_BLUE     0x39BF
#define COLOR_PINK     0xF92A
#define COLOR_YELLOW   0xFFE0
#define COLOR_TURQUOISE 0x5FF7
#define COLOR_WHITE    0xCE79
#define COLOR_GRAY     0x7BEF

SPIClass spiDisplay(FSPI);

Adafruit_ST7789 tft(
  &spiDisplay, TFT_CS, TFT_DC, TFT_RST
);

static uint16_t drawBuffer[SCREEN_W * 40];

static lv_obj_t *valueLabels[2];
static lv_obj_t *titleLabels[2];
static lv_obj_t *statusLabel;
static lv_obj_t *clockLabel;
static lv_obj_t *lastSyncLabel;

String cachedValues[NUM_CHANNELS];
int currentPairIndex = 0; // Bläddrar mellan par (0 till 3 eftersom vi har 4 par totalt)

// =====================================================
// LVGL TICK
// =====================================================
static uint32_t getTick() {
  return millis();
}

// =====================================================
// SKÄRMUPPDATERING
// =====================================================
static void displayFlush(
  lv_display_t *display,
  const lv_area_t *area,
  uint8_t *pxMap
) {
  uint32_t width = area->x2 - area->x1 + 1;
  uint32_t height = area->y2 - area->y1 + 1;

  tft.drawRGBBitmap(
    area->x1,
    area->y1,
    reinterpret_cast<uint16_t *>(pxMap),
    width,
    height
  );

  lv_display_flush_ready(display);
}

// =====================================================
// GEMENSAM KORTSTIL
// =====================================================
static void stylePanel(lv_obj_t *obj) {
  lv_obj_set_style_bg_color(obj, lv_color_hex(COLOR_PANEL), 0);
  lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
  lv_obj_set_style_border_color(obj, lv_color_hex(COLOR_BORDER), 0);
  lv_obj_set_style_border_width(obj, 1, 0);
  lv_obj_set_style_radius(obj, 9, 0);
  lv_obj_set_style_pad_all(obj, 6, 0);
  lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
}

static lv_obj_t *makeLabel(
  lv_obj_t *parent,
  const char *text,
  uint32_t color,
  const lv_font_t *font
) {
  lv_obj_t *label = lv_label_create(parent);
  lv_label_set_text(label, text);
  lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
  lv_obj_set_style_text_font(label, font, 0);
  return label;
}

// =====================================================
// SKAPA DASHBOARD
// =====================================================
static void createUI() {
  lv_obj_t *screen = lv_screen_active();

  lv_obj_set_style_bg_color(screen, lv_color_hex(COLOR_BG), 0);
  lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
  lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

  // STORT KORT TILL VÄNSTER (Klocka)
  lv_obj_t *mainCard = lv_obj_create(screen);
  lv_obj_set_pos(mainCard, 8, 8);
  lv_obj_set_size(mainCard, 190, 154);
  stylePanel(mainCard);
  lv_obj_set_style_border_color(mainCard, lv_color_hex(COLOR_CYAN), 0);

  // Statusrad överst
  statusLabel = makeLabel(mainCard, "STARTING...", COLOR_GRAY, &lv_font_montserrat_14);
  lv_obj_set_pos(statusLabel, 6, 4);

  // Klocka (fast position, centrerad åt vänster)
  clockLabel = makeLabel(mainCard, "00:00", COLOR_WHITE, &lv_font_montserrat_48);
  lv_obj_set_style_transform_zoom(clockLabel, 320, 0);
  lv_obj_set_pos(clockLabel, 10, 42);

  // Senaste uppdateringstid längst ner
  lastSyncLabel = makeLabel(mainCard, "Senast: --:--:--", COLOR_GRAY, &lv_font_montserrat_14);
  lv_obj_set_pos(lastSyncLabel, 6, 128);

  // TVÅ STÖRRE KORT TILL HÖGER
  const int cardX = 206;
  const int cardW = 106;
  const int cardH = 72;
  const int cardY[] = {8, 90};

  const uint32_t titleColors[] = {COLOR_CYAN, COLOR_ORANGE};

  for (int i = 0; i < 2; i++) {
    lv_obj_t *card = lv_obj_create(screen);
    lv_obj_set_pos(card, cardX, cardY[i]);
    lv_obj_set_size(card, cardW, cardH);
    stylePanel(card);

    titleLabels[i] = makeLabel(
      card,
      "",
      titleColors[i],
      &lv_font_montserrat_14
    );
    lv_obj_align(titleLabels[i], LV_ALIGN_TOP_LEFT, 0, -2);

    valueLabels[i] = makeLabel(
      card,
      "--",
      COLOR_WHITE,
      &lv_font_montserrat_14
    );
    lv_obj_set_style_text_font(valueLabels[i], &lv_font_montserrat_24, 0);
    lv_obj_align(valueLabels[i], LV_ALIGN_CENTER, 0, 8);
  }
}

// =====================================================
// UPPDATERA HÖGRA SIDANS VISNING
// =====================================================
static void updateDisplayPair() {
  int firstIdx = currentPairIndex * 2;
  
  for (int i = 0; i < 2; i++) {
    int chIdx = firstIdx + i;
    if (chIdx < NUM_CHANNELS) {
      lv_label_set_text(titleLabels[i], channels[chIdx].name);
      lv_label_set_text(valueLabels[i], cachedValues[chIdx].c_str());
      
      // Unik färg per kanal
      uint32_t col = COLOR_CYAN;
      if (chIdx == 1) col = COLOR_ORANGE;
      else if (chIdx == 2) col = COLOR_GREEN;
      else if (chIdx == 3) col = COLOR_PURPLE;
      else if (chIdx == 4) col = COLOR_BLUE;
      else if (chIdx == 5) col = COLOR_PINK;
      else if (chIdx == 6) col = COLOR_YELLOW;
      else if (chIdx == 7) col = COLOR_TURQUOISE;

      lv_obj_set_style_text_color(titleLabels[i], lv_color_hex(col), 0);
    }
  }
}

// =====================================================
// UPPDATERA KLOCKA
// =====================================================
static void updateClock() {
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    char timeStr[6];
    snprintf(timeStr, sizeof(timeStr), "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
    lv_label_set_text(clockLabel, timeStr);
  }
}

// =====================================================
// HÄMTA DATA FRÅN THINGSPEAK
// =====================================================
static bool fetchChannel(int index) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WiFi] Inte ansluten");
    return false;
  }

  const ChannelConfig &ch = channels[index];

  String url =
    "https://api.thingspeak.com/channels/" +
    String(ch.channelId) +
    "/fields/" + String(ch.field) +
    "/last.txt?api_key=" + String(ch.apiKey);

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  http.setConnectTimeout(5000);
  http.setTimeout(5000);

  if (!http.begin(client, url)) return false;

  int code = http.GET();
  bool success = false;

  if (code == HTTP_CODE_OK) {
    String payload = http.getString();
    payload.trim();

    char *endPtr = nullptr;
    float value = strtof(payload.c_str(), &endPtr);

    if (payload.length() > 0 && endPtr != payload.c_str() && *endPtr == '\0') {
      char number[24];
      char formatted[40];

      snprintf(number, sizeof(number), ch.decimals ? "%.1f" : "%.0f", value);
      snprintf(formatted, sizeof(formatted), "%s %s", number, ch.unit);

      cachedValues[index] = String(formatted);
      success = true;
    }
  }

  http.end();

  if (success) {
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
      char timeStr[20];
      snprintf(timeStr, sizeof(timeStr), "Senast: %02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
      lv_label_set_text(lastSyncLabel, timeStr);
    }
  }

  return success;
}

// =====================================================
// SETUP
// =====================================================
void setup() {
  Serial.begin(115200);
  delay(300);

  pinMode(PIN_LED, OUTPUT);
  ledcAttach(PIN_LED, 5000, 8);
  ledcWrite(PIN_LED, SCREEN_BRIGHTNESS);

  spiDisplay.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);

  tft.init(170, 320);
  tft.setSPISpeed(40000000);
  tft.setRotation(3);
  tft.fillScreen(COLOR_BG);

  lv_init();
  lv_tick_set_cb(getTick);

  lv_display_t *display = lv_display_create(SCREEN_W, SCREEN_H);
  lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
  lv_display_set_flush_cb(display, displayFlush);
  lv_display_set_buffers(display, drawBuffer, nullptr, sizeof(drawBuffer), LV_DISPLAY_RENDER_MODE_PARTIAL);

  // Initiera cache med --
  for(int i=0; i<NUM_CHANNELS; i++) {
    cachedValues[i] = "--";
  }

  createUI();

  for (int i = 0; i < 5; i++) {
    lv_timer_handler();
    delay(10);
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    lv_timer_handler();
    delay(10);
  }

  if (WiFi.status() == WL_CONNECTED) {
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

    char wifiStr[24];
    snprintf(wifiStr, sizeof(wifiStr), "WiFi: %ld dBm", WiFi.RSSI());
    lv_label_set_text(statusLabel, wifiStr);

    for (int i = 0; i < NUM_CHANNELS; i++) {
      fetchChannel(i);
      lv_timer_handler();
    }
    updateDisplayPair();
  } else {
    lv_label_set_text(statusLabel, "WIFI FAILED");
  }
}

// =====================================================
// LOOP
// =====================================================
void loop() {
  lv_timer_handler();

  static uint32_t lastClockUpdate = 0;
  if (millis() - lastClockUpdate >= 1000) {
    lastClockUpdate = millis();
    updateClock();
  }

  // Byt par (totalt 4 par: 0-1, 2-3, 4-5, 6-7) var 15:e sekund
  static uint32_t lastPairSwitch = 0;
  if (millis() - lastPairSwitch >= 15000) {
    lastPairSwitch = millis();

    if (WiFi.status() == WL_CONNECTED) {
      char wifiStr[24];
      snprintf(wifiStr, sizeof(wifiStr), "WiFi: %ld dBm", WiFi.RSSI());
      lv_label_set_text(statusLabel, wifiStr);
    }

    currentPairIndex = (currentPairIndex + 1) % 4;
    updateDisplayPair();
  }

  // Hämta en kanal i taget var 15:e sekund (cyklar igenom alla 8 på 2 minuter)
  static uint32_t lastFetch = 0;
  static int nextChannelToFetch = 0;

  if (millis() - lastFetch >= 15000) {
    lastFetch = millis();
    if (WiFi.status() == WL_CONNECTED) {
      fetchChannel(nextChannelToFetch);
      nextChannelToFetch = (nextChannelToFetch + 1) % NUM_CHANNELS;
      updateDisplayPair();
    }
  }

  delay(5);
}
