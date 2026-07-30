// InkCards on-card format constants, shared by the firmware and mirrored by the
// Python companion (companion/inkcards/deckformat.py). See docs/FORMAT.md.
//
// This header is deliberately free of any Arduino or FreeInk SDK dependency so
// it compiles unchanged on the ESP32-C3 target and on a desktop host for unit
// testing.
#pragma once

#include <cstdint>

namespace inkcards {

// ".deck" magic: 'I','N','K','D'.
constexpr uint8_t kDeckMagic[4] = {0x49, 0x4E, 0x4B, 0x44};
constexpr uint8_t kDeckVersion = 1;

// ".rev" magic: 'I','N','K','R'.
constexpr uint8_t kRevMagic[4] = {0x49, 0x4E, 0x4B, 0x52};
constexpr uint8_t kRevVersion = 1;

// Deck header flag bits.
enum DeckFlag : uint8_t {
  DECK_FLAG_HAS_PINYIN = 1u << 0,
};

// Card field types. Values are stable on-disk identifiers.
enum FieldType : uint8_t {
  FIELD_FRONT = 0,
  FIELD_BACK = 1,
  FIELD_PINYIN = 2,
  FIELD_EXAMPLE = 3,
  FIELD_NOTES = 4,
  FIELD_HINT = 5,
  FIELD_TAGS = 6,
};

// fieldMask bit for a given field type (header advertises which fields exist).
constexpr uint16_t fieldMaskBit(FieldType t) { return static_cast<uint16_t>(1u << t); }

// Review grades, ordered from worst to best. The on-device buttons map to
// these; see docs/BUTTON_MAPPING.md.
enum Grade : uint8_t {
  GRADE_AGAIN = 0,
  GRADE_HARD = 1,
  GRADE_GOOD = 2,
  GRADE_EASY = 3,
};

// Seconds in a day, used to convert a Unix time plus a timezone offset into the
// integer "day number" the scheduler works in.
constexpr uint32_t kSecondsPerDay = 86400u;

}  // namespace inkcards
