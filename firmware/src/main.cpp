// InkCards firmware entry point for the Xteink X4/X3 (ESP32-C3), built on the
// FreeInk SDK. The boot sequence mirrors CrossPoint's: bring up serial, GPIO,
// SD storage and the e-ink display, provision fonts, then hand control to the
// InkCards application loop.
//
// This is the "device" PlatformIO environment. The portable engine it drives
// (firmware/lib/inkcards_core) is exercised by host unit tests in CI; the glue
// in this file and under src/ targets real hardware and is marked with
// TODO(hardware-test) wherever an on-device check is required. See
// docs/HARDWARE_TESTING.md.
#include <Arduino.h>
#include <BoardConfig.h>
#include <GfxRenderer.h>
#include <HalClock.h>
#include <HalDisplay.h>
#include <HalGPIO.h>
#include <HalStorage.h>
#include <Logging.h>

#include "InkCardsApp.h"
#include "platform/InkInput.h"
#include "ui/Theme.h"

using namespace inkcards;

// SDK-provided hardware singletons (declared by the FreeInk SDK headers, as in
// CrossPoint's main.cpp).
extern HalDisplay display;
extern HalGPIO gpio;

static GfxRenderer renderer(display);
static InkInput input(gpio);
static InkFonts fonts;

// Study configuration. TODO(hardware-test): surface these in an on-device
// settings screen; for now they are compile-time defaults.
static SessionConfig sessionConfig = {/*newCardsPerDay=*/20, /*maxReviews=*/0};

// Local timezone offset in seconds. TODO(hardware-test): read from the RTC /
// user setting. Defaulting to UTC keeps "day" well-defined until set.
static int32_t gTzOffsetSeconds = 0;

static uint32_t currentDay() {
  // HalClock provides Unix time when the RTC is set; fall back to millis-based
  // day 0 so the scheduler still runs on a device with no clock.
  // TODO(hardware-test): confirm HalClock's accessor on the installed SDK.
  uint64_t unix = HalClock::nowUnix();
  if (unix == 0) return 0;
  return dayNumber(unix, gTzOffsetSeconds);
}

// Provision the fonts the UI draws with. InkCards does not embed fonts: it
// loads CrossPoint-compatible .cpfont families from the SD card (/.fonts or
// /fonts), exactly as documented in docs/sd-card-fonts of CrossPoint, and
// assigns sizes to the four UI roles. A CJK-capable family (e.g. NotoSansSC)
// therefore renders Latin and CJK decks alike.
//
// TODO(hardware-test): wire this to the SD-card font system. The concrete
// SdCardFont loading + renderer.registerSdCardFont() calls depend on the
// FreeInk SDK's font API version; this function documents the intended mapping
// and must be completed against the installed SDK. Until then the renderer
// falls back to any default family the SDK registers at begin().
static void setupFonts() {
  // Role -> point size. These match common .cpfont build sizes so a single
  // family installed for CrossPoint covers InkCards too.
  //   small = 10, body = 14, head = 18, card = 18 (largest available)
  // The concrete font-id assignment is filled in when the SD font family is
  // loaded; see the TODO above.
  fonts.small = 0;
  fonts.body = 0;
  fonts.head = 0;
  fonts.card = 0;
  LOG_WARN("INK", "setupFonts: SD-card font wiring pending on-device integration");
}

static InkCardsApp* app = nullptr;

void setup() {
  Serial.begin(115200);
  LOG_INFO("INK", "InkCards starting");

  gpio.begin();
  if (!Storage.begin()) {
    LOG_ERR("INK", "SD storage init failed");
  }

  display.begin();
  renderer.begin();

  setupFonts();

  static InkCardsApp instance(renderer, input, fonts, currentDay, sessionConfig);
  app = &instance;
  app->begin();
}

void loop() {
  app->loop();
  // Modest delay to debounce input and save power; the e-ink UI is event
  // driven, so a tight spin is unnecessary.
  delay(20);
}
