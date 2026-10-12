
#include <Arduino.h>
#include <SPI.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <lvgl.h>

// =====================================================
// WIFI
// =====================================================
const char* WIFI_SSID = "";
const char* WIFI_PASS = "";

// =====================================================
// THINGSPEAK-KANALER
// Rubriker, kanal-ID, fält, API-nyckel och enhet
// ändras här. UI:t använder automatiskt namnen.
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
  {"sovrum", 631325, 1, "IA9TGDTR2351DC1I", "C",   1},
  {"overvaning",  2011583, 1, "EL96SBAO527BKNP3", "C",  1},
  {"ute",         2013865, 1, "PLWLFP0HG4TE9BNZ", "C",  1},
  {"kok",        2014046, 1, "68ODS47UMTXTWR9K", "C", 1}
};

constexpr int NUM_CHANNELS =
  sizeof(channels) / sizeof(channels[0]);

// =====================================================
// ESP32-C3 / ST7789
// Samma pinnar som i din fungerande kod
// =====================================================
#define TFT_SCLK 0
#define TFT_MOSI 1
#define TFT_RST  2
#define TFT_DC   3
#define TFT_CS   4
#define PIN_LED  5

// Widescreen efter rotation
constexpr int SCREEN_W = 320;
constexpr int SCREEN_H = 170;

#define COLOR_BG       0x0821
#define COLOR_PANEL    0x18C5
#define COLOR_BORDER   0x3218
#define COLOR_CYAN     0x07FF
#define COLOR_ORANGE   0xFCE0
#define COLOR_GREEN    0x07E0
#define COLOR_RED      0xF800
#define COLOR_WHITE    0xFFFF
#define COLOR_GRAY     0x7BEF

SPIClass spiDisplay(FSPI);
Adafruit_ST7789 tft(
  &spiDisplay, TFT_CS, TFT_DC, TFT_RST
);

// LVGL partial buffer: 40 rader RGB565
static uint16_t drawBuffer[SCREEN_W * 40];

// LVGL-objekt för värden och rubriker
static lv_obj_t *valueLabels[NUM_CHANNELS];
static lv_obj_t *titleLabels[NUM_CHANNELS];
static lv_obj_t *statusLabel;

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

  // LVGL RGB565 till Adafruit ST7789
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
// GEMENSAM STIL
// =====================================================
static void stylePanel(lv_obj_t *obj) {
  lv_obj_set_style_bg_color(
    obj, lv_color_hex(COLOR_PANEL), 0
  );
  lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
  lv_obj_set_style_border_color(
    obj, lv_color_hex(COLOR_BORDER), 0
  );
  lv_obj_set_style_border_width(obj, 1, 0);
  lv_obj_set_style_radius(obj, 9, 0);
  lv_obj_set_style_pad_all(obj, 7, 0);
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
  lv_obj_set_style_text_color(
    label, lv_color_hex(color), 0
  );
  lv_obj_set_style_text_font(label, font, 0);
  return label;
}

// =====================================================
// SKAPA DASHBOARD
// =====================================================
static void createUI() {
  lv_obj_t *screen = lv_screen_active();

  lv_obj_set_style_bg_color(
    screen, lv_color_hex(COLOR_BG), 0
  );
  lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
  lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

  // STOR PANEL TILL VÄNSTER
  lv_obj_t *mainCard = lv_obj_create(screen);
  lv_obj_set_pos(mainCard, 8, 8);
  lv_obj_set_size(mainCard, 184, 154);
  stylePanel(mainCard);

  lv_obj_set_style_border_color(
    mainCard, lv_color_hex(COLOR_CYAN), 0
  );

  titleLabels[0] = makeLabel(
    mainCard, channels[0].name,
    COLOR_CYAN, &lv_font_montserrat_48
  );
  lv_obj_set_width(titleLabels[0], 164);
  lv_label_set_long_mode(
    titleLabels[0], LV_LABEL_LONG_DOT
  );
  lv_obj_align(
    titleLabels[0], LV_ALIGN_TOP_LEFT, 1, 2
  );

  valueLabels[0] = makeLabel(
    mainCard, "--.-",
    COLOR_WHITE, &lv_font_montserrat_14
  );

  // En större font kan aktiveras i lv_conf.h
  // och användas här, exempelvis montserrat_32.
  lv_obj_set_style_text_font(
    valueLabels[0], &lv_font_montserrat_14, 0
  );
  lv_obj_align(
    valueLabels[0], LV_ALIGN_CENTER, 0, 3
  );

  lv_obj_t *unitLabel = makeLabel(
    mainCard, channels[0].unit,
    COLOR_ORANGE, &lv_font_montserrat_14
  );
  lv_obj_align(
    unitLabel, LV_ALIGN_BOTTOM_LEFT, 1, -1
  );

  // TRE SMÅ KORT TILL HÖGER
  const int cardX = 201;
  const int cardW = 111;
  const int cardH = 48;
  const int cardY[] = {8, 61, 114};

  const uint32_t titleColors[] = {
    COLOR_CYAN, COLOR_ORANGE, COLOR_GREEN
  };

  for (int i = 1; i < NUM_CHANNELS; i++) {
    lv_obj_t *card = lv_obj_create(screen);

    lv_obj_set_pos(card, cardX, cardY[i - 1]);
    lv_obj_set_size(card, cardW, cardH);
    stylePanel(card);

    // Rubriken hämtas från channels[]
    titleLabels[i] = makeLabel(
      card, channels[i].name,
      titleColors[i - 1], &lv_font_montserrat_14
    );

    lv_obj_set_width(titleLabels[i], cardW - 20);
    lv_label_set_long_mode(
      titleLabels[i], LV_LABEL_LONG_DOT
    );
    lv_obj_align(
      titleLabels[i], LV_ALIGN_TOP_LEFT, 0, -1
    );

    valueLabels[i] = makeLabel(
      card, "--",
      COLOR_WHITE, &lv_font_montserrat_14
    );
    lv_obj_align(
      valueLabels[i], LV_ALIGN_BOTTOM_LEFT, 0, 0
    );
  }

  statusLabel = makeLabel(
    screen, "STARTING...",
    COLOR_GRAY, &lv_font_montserrat_14
  );
  lv_obj_set_pos(statusLabel, 12, 145);
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

  // För test. Använd certifikatverifiering i produktion.
  client.setInsecure();

  HTTPClient http;
  http.setConnectTimeout(5000);
  http.setTimeout(5000);

  Serial.printf("[HTTP] Hämtar %s\n", ch.name);

  if (!http.begin(client, url)) {
    Serial.println("[HTTP] begin misslyckades");
    return false;
  }

  int code = http.GET();
  bool success = false;

  Serial.printf("[HTTP] Status: %d\n", code);

  if (code == HTTP_CODE_OK) {
    String payload = http.getString();
    payload.trim();

    Serial.printf(
      "[HTTP] Svar: '%s'\n", payload.c_str()
    );

    char *endPtr = nullptr;
    float value = strtof(payload.c_str(), &endPtr);

    if (payload.length() > 0 &&
        endPtr != payload.c_str() &&
        *endPtr == '\0') {

      char number[24];
      char formatted[40];

      snprintf(
        number, sizeof(number),
        ch.decimals ? "%.1f" : "%.0f",
        value
      );

      snprintf(
        formatted, sizeof(formatted),
        "%s %s", number, ch.unit
      );

      lv_label_set_text(
        valueLabels[index], formatted
      );

      Serial.printf(
        "[OK] %s = %s\n", ch.name, formatted
      );

      success = true;
    } else {
      Serial.println("[DATA] Ogiltigt numeriskt svar");
    }
  } else if (code < 0) {
    Serial.printf(
      "[HTTP] Fel: %s\n",
      http.errorToString(code).c_str()
    );
  }

  http.end();
  return success;
}

// =====================================================
// SETUP
// =====================================================
void setup() {
  Serial.begin(115200);
  delay(300);

  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, HIGH);

  // Samma initiering som i din fungerande skärmkod
  spiDisplay.begin(
    TFT_SCLK, -1, TFT_MOSI, TFT_CS
  );

  tft.init(170, 320);
  tft.setSPISpeed(40000000);
  tft.setRotation(1);
  tft.fillScreen(COLOR_BG);

  // LVGL 9
  lv_init();
  lv_tick_set_cb(getTick);

  lv_display_t *display = lv_display_create(
    SCREEN_W, SCREEN_H
  );

  lv_display_set_color_format(
    display, LV_COLOR_FORMAT_RGB565
  );

  lv_display_set_flush_cb(display, displayFlush);

  lv_display_set_buffers(
    display,
    drawBuffer,
    nullptr,
    sizeof(drawBuffer),
    LV_DISPLAY_RENDER_MODE_PARTIAL
  );

  createUI();

  // Rita gränssnittet före nätverksanslutning
  for (int i = 0; i < 5; i++) {
    lv_timer_handler();
    delay(10);
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  Serial.println("[WiFi] Ansluter...");

  uint32_t start = millis();

  while (WiFi.status() != WL_CONNECTED &&
         millis() - start < 15000) {
    lv_timer_handler();
    delay(10);
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("[WiFi] IP: ");
    Serial.println(WiFi.localIP());

    lv_label_set_text(
      statusLabel, "WIFI CONNECTED"
    );

    for (int i = 0; i < NUM_CHANNELS; i++) {
      fetchChannel(i);
      lv_timer_handler();
    }
  } else {
    Serial.println("[WiFi] Anslutning misslyckades");
    lv_label_set_text(statusLabel, "WIFI FAILED");
  }
}

// =====================================================
// LOOP
// =====================================================
void loop() {
  lv_timer_handler();

  // En kanal var 5:e sekund:
  // varje kanal uppdateras var 20:e sekund.
  static uint32_t lastFetch = 0;
  static int nextChannel = 0;

  if (millis() - lastFetch >= 5000) {
    lastFetch = millis();

    if (WiFi.status() != WL_CONNECTED) {
      lv_label_set_text(
        statusLabel, "WIFI RECONNECTING"
      );
      WiFi.reconnect();
    } else {
      bool ok = fetchChannel(nextChannel);

      lv_label_set_text(
        statusLabel,
        ok ? "DATA UPDATED" : "DATA ERROR"
      );

      nextChannel =
        (nextChannel + 1) % NUM_CHANNELS;
    }
  }

  delay(5);
}
