#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "soc/soc.h"
#include "soc/usb_serial_jtag_reg.h"

// =========================
// PINOUT
// =========================

#define LED_PIN       4
#define BATTERY_PIN   3   // GPIO3 = ADC1_CH2
#define TRIG_PIN      6
#define SDA_PIN       7
#define SCL_PIN       8
#define MOSFET_PIN   13
#define ECHO_PIN     14

// =========================
// OLED
// =========================

#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 32
#define OLED_RESET    -1
#define OLED_ADDRESS  0x3C

Adafruit_SSD1306 display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire,
  OLED_RESET
);

// =========================
// USB GPIO13/14
// =========================

void disableUSB()
{
  CLEAR_PERI_REG_MASK(
    USB_SERIAL_JTAG_CONF0_REG,
    USB_SERIAL_JTAG_USB_PAD_ENABLE
  );
}

// =========================
// BATTERI
// =========================

void readBattery(
  int &raw,
  uint32_t &millivolts,
  float &batteryVoltage
)
{
  uint32_t rawSum = 0;
  uint32_t mvSum = 0;

  for (int i = 0; i < 32; i++)
  {
    rawSum += analogRead(BATTERY_PIN);
    mvSum += analogReadMilliVolts(BATTERY_PIN);

    delay(2);
  }

  raw = rawSum / 32;
  millivolts = mvSum / 32;

  // 47k / 10k
  // Vbat = Vadc × 5.7
  batteryVoltage =
    (millivolts / 1000.0) * 5.7;
}

// =========================
// HC-SR04
// =========================

float measureDistance()
{
  // Slå på HC-SR04
  digitalWrite(MOSFET_PIN, HIGH);

  // Viktigt: minst 150 ms uppstartstid
  delay(150);

  digitalWrite(LED_PIN, HIGH);

  // Trigger
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(5);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);

  digitalWrite(TRIG_PIN, LOW);

  // Echo
  unsigned long duration =
    pulseIn(ECHO_PIN, HIGH, 30000);

  digitalWrite(LED_PIN, LOW);

  // Stäng av HC-SR04
  digitalWrite(MOSFET_PIN, LOW);

  if (duration == 0)
    return -1;

  return duration / 58.0;
}

// =========================
// SETUP
// =========================

void setup()
{
  // Frigör GPIO13 och GPIO14 från USB
  disableUSB();

  // LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // MOSFET
  pinMode(MOSFET_PIN, OUTPUT);
  digitalWrite(MOSFET_PIN, LOW);

  // TRIG
  pinMode(TRIG_PIN, OUTPUT);
  digitalWrite(TRIG_PIN, LOW);

  // ECHO
  pinMode(ECHO_PIN, INPUT);

  // ADC
  analogReadResolution(12);

  analogSetPinAttenuation(
    BATTERY_PIN,
    ADC_11db
  );

  // OLED
  Wire.begin(SDA_PIN, SCL_PIN);

  if (!display.begin(
        SSD1306_SWITCHCAPVCC,
        OLED_ADDRESS))
  {
    while (true)
    {
      delay(100);
    }
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  display.setCursor(0, 0);
  display.println("ESP32-C5 ADC TEST");

  display.setCursor(0, 16);
  display.println("Startar...");

  display.display();

  delay(1000);
}

// =========================
// LOOP
// =========================

void loop()
{
  // Läs båda ADC-värdena
  int raw;
  uint32_t millivolts;
  float batteryVoltage;

  readBattery(
    raw,
    millivolts,
    batteryVoltage
  );

  // Mät avstånd
  float distance =
    measureDistance();

  // =========================
  // OLED
  // =========================

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Rad 1: RAW
  display.setCursor(0, 0);

  display.print("RAW: ");
  display.print(raw);

  // Rad 2: kalibrerat ADC-värde
  display.setCursor(0, 8);

  display.print("ADC: ");
  display.print(millivolts);
  display.print(" mV");

  // Rad 3: batteri
  display.setCursor(0, 16);

  display.print("BAT: ");
  display.print(batteryVoltage, 2);
  display.print(" V");

  // Rad 4: avstånd
  display.setCursor(0, 24);

  display.print("DIST: ");

  if (distance < 0)
  {
    display.print("ERROR");
  }
  else
  {
    display.print(distance, 1);
    display.print(" cm");
  }

  display.display();

  // Mät en gång per sekund
  delay(1000);
}
