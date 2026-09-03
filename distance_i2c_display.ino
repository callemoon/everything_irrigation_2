#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "soc/usb_serial_jtag_reg.h"
#include "soc/soc.h"

// ========================================
// PINOUT - Waveshare ESP32-C5-Zero
// ========================================

#define LED_PIN     4
#define TRIG_PIN    6
#define SDA_PIN     7
#define SCL_PIN     8
#define MOSFET_PIN  13
#define ECHO_PIN    14

// ========================================
// OLED
// ========================================

#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 32
#define OLED_RESET    -1

Adafruit_SSD1306 display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire,
  OLED_RESET
);


// ========================================
// Stäng av USB på GPIO13/14
// ========================================

void disableUSB() {
  // GPIO13 = USB D-
  // GPIO14 = USB D+
  // Släpp USB-pads så de kan användas som GPIO.

  CLEAR_PERI_REG_MASK(
    USB_SERIAL_JTAG_CONF0_REG,
    USB_SERIAL_JTAG_USB_PAD_ENABLE
  );
}


// ========================================
// Mät avstånd
// ========================================

float measureDistance() {

  // Slå på strömmen till HC-SR04
  digitalWrite(MOSFET_PIN, HIGH);

  // LED visar att vi mäter
  digitalWrite(LED_PIN, HIGH);

  // Vänta 50 ms på att HC-SR04 ska starta
  delay(150);

  // -----------------------------
  // Triggerpuls
  // -----------------------------

  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(5);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);

  digitalWrite(TRIG_PIN, LOW);

  // -----------------------------
  // Läs ECHO
  // -----------------------------

  unsigned long duration = pulseIn(
    ECHO_PIN,
    HIGH,
    30000
  );

  // Mätningen är klar
  digitalWrite(LED_PIN, LOW);

  // Stäng av HC-SR04
  digitalWrite(MOSFET_PIN, LOW);

  // Ingen ECHO-signal
  if (duration == 0) {
    return -1;
  }

  // Omvandla mikrosekunder till cm
  return duration / 58.0;
}


// ========================================
// SETUP
// ========================================

void setup() {

  // ----------------------------------------
  // Stäng av USB-funktionen på GPIO13/14
  // ----------------------------------------

  disableUSB();

  // ----------------------------------------
  // GPIO
  // ----------------------------------------

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  pinMode(MOSFET_PIN, OUTPUT);
  digitalWrite(MOSFET_PIN, LOW);

  pinMode(TRIG_PIN, OUTPUT);
  digitalWrite(TRIG_PIN, LOW);

  pinMode(ECHO_PIN, INPUT);

  // ----------------------------------------
  // I2C
  // ----------------------------------------

  Wire.begin(
    SDA_PIN,
    SCL_PIN
  );

  // ----------------------------------------
  // OLED
  // ----------------------------------------

  if (!display.begin(
        SSD1306_SWITCHCAPVCC,
        0x3C
      )) {

    // OLED hittades inte
    while (true) {
      delay(100);
    }
  }

  // ----------------------------------------
  // Startskärm
  // ----------------------------------------

  display.clearDisplay();

  display.setTextColor(
    SSD1306_WHITE
  );

  display.setTextSize(2);
  display.setCursor(0, 0);

  display.println("ESP32-C5");

  display.setTextSize(1);
  display.setCursor(0, 20);

  display.println("Avstandsmatare");

  display.display();

  delay(1000);
}


// ========================================
// LOOP
// ========================================

void loop() {

  // Mät en gång
  float distance = measureDistance();

  // ----------------------------------------
  // OLED
  // ----------------------------------------

  display.clearDisplay();

  display.setTextColor(
    SSD1306_WHITE
  );

  display.setTextSize(1);
  display.setCursor(0, 0);

  display.println("Avstand:");

  display.setTextSize(2);
  display.setCursor(0, 12);

  if (distance < 0) {

    display.println("TIMEOUT");

  } else {

    display.print(distance, 1);
    display.println(" cm");
  }

  display.display();

  // ----------------------------------------
  // Vänta till nästa mätning
  // ----------------------------------------

  delay(1000);
}