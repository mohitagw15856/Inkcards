// InkCards engine self-test firmware for the ESP32-C3.
//
// This is the environment CI builds (env:selftest). It links the portable
// InkCards core (firmware/lib/inkcards_core) into a real ESP32-C3 image and,
// on boot, uses only the Arduino core + the SD library (no FreeInk SDK) to:
//   1. mount the SD card,
//   2. enumerate /inkcards/decks/*.deck,
//   3. open each deck and print its header, card count and today's due/new
//      counts, streaming from SD via the core.
//
// Its purpose is twofold: prove the engine cross-compiles and fits on real
// silicon, and give a headless bring-up path for validating decks on hardware
// without the display. The full e-ink UI lives in the "device" environment and
// is built against the FreeInk SDK. See ARCHITECTURE.md.
#include <Arduino.h>
#include <SD.h>
#include <SPI.h>

#include "DeckReader.h"
#include "ReviewState.h"
#include "Session.h"
#include "ArduinoByteStream.h"

using namespace inkcards;

// SD chip-select pin. TODO(hardware-test): set to the X4/X3 wiring. The default
// keeps the build valid; on a board without SD the self-test simply reports it.
#ifndef INKCARDS_SD_CS
#define INKCARDS_SD_CS 7
#endif

static void inspectDeck(const String& path) {
  File f = SD.open(path.c_str(), FILE_READ);
  if (!f) {
    Serial.printf("  cannot open %s\n", path.c_str());
    return;
  }
  ArduinoFileReader reader(f);
  DeckReader deck(reader);
  if (!deck.open()) {
    Serial.printf("  %s: not a valid .deck file\n", path.c_str());
    f.close();
    return;
  }

  Serial.printf("  Deck: \"%s\"  cards=%u  pinyin=%d\n", deck.header().name.c_str(),
                static_cast<unsigned>(deck.cardCount()), deck.header().hasPinyin() ? 1 : 0);

  ReviewStore store;  // fresh state: every card is new
  SessionConfig cfg;
  Session session(deck, store, cfg);
  session.build(/*todayDay=*/0);
  SessionStats st = session.stats();
  Serial.printf("    due=%u new=%u remaining=%u\n", static_cast<unsigned>(st.dueReviews),
                static_cast<unsigned>(st.newCards), static_cast<unsigned>(st.remaining));

  // Read the first card to prove streaming works end to end.
  if (deck.cardCount() > 0) {
    DeckCard c;
    if (deck.readCard(0, c)) {
      Serial.printf("    first card: front=\"%s\" back=\"%s\"\n", c.front.c_str(), c.back.c_str());
    }
  }
  f.close();
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\nInkCards engine self-test");

  if (!SD.begin(INKCARDS_SD_CS)) {
    Serial.println("SD card not mounted (expected on a bare bring-up board)");
    return;
  }

  File dir = SD.open("/inkcards/decks");
  if (!dir || !dir.isDirectory()) {
    Serial.println("No /inkcards/decks directory on the card");
    return;
  }

  Serial.println("Decks found:");
  for (File entry = dir.openNextFile(); entry; entry = dir.openNextFile()) {
    String name = entry.name();
    bool isDeck = name.endsWith(".deck");
    String full = String("/inkcards/decks/") + name;
    entry.close();
    if (isDeck) inspectDeck(full);
  }
  Serial.println("Self-test complete");
}

void loop() { delay(1000); }
