// Host-side helper that assembles a .deck byte buffer, so DeckReader can be
// tested against a known-good layout without pulling in the Python companion.
// This mirrors docs/FORMAT.md and companion/inkcards/deckformat.py.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "InkCardsFormat.h"
#include "Serialization.h"

namespace testing {

struct BuilderCard {
  uint32_t cardId;
  std::vector<std::pair<inkcards::FieldType, std::string>> fields;
};

inline std::vector<uint8_t> buildDeck(const std::string& name, const std::string& description,
                                      const std::string& fontHint, const std::vector<BuilderCard>& cards,
                                      bool hasPinyin) {
  using namespace inkcards;

  // Encode each card record independently first so we know its length.
  std::vector<std::vector<uint8_t>> records;
  uint16_t fieldMask = 0;
  for (const auto& c : cards) {
    MemoryWriter mw;
    writeU32(mw, c.cardId);
    writeU8(mw, static_cast<uint8_t>(c.fields.size()));
    for (const auto& f : c.fields) {
      writeU8(mw, static_cast<uint8_t>(f.first));
      writeString(mw, f.second);
      fieldMask |= fieldMaskBit(f.first);
    }
    records.push_back(mw.bytes());
  }

  // Header, then offset table, then card data. Compute positions.
  MemoryWriter head;
  head.write(kDeckMagic, 4);
  writeU8(head, kDeckVersion);
  writeU8(head, hasPinyin ? DECK_FLAG_HAS_PINYIN : 0);
  writeU16(head, fieldMask);
  writeU32(head, static_cast<uint32_t>(cards.size()));
  // offsetTablePos and cardDataPos are patched after we know the header size.
  size_t patchOffsetTablePos = head.bytes().size();
  writeU32(head, 0);
  size_t patchCardDataPos = head.bytes().size();
  writeU32(head, 0);
  writeString(head, name);
  writeString(head, description);
  writeString(head, fontHint);

  uint32_t offsetTablePos = static_cast<uint32_t>(head.bytes().size());
  uint32_t cardDataPos = offsetTablePos + static_cast<uint32_t>(cards.size() * 4);

  // Patch the two position fields in the header buffer.
  auto patch = [&](size_t at, uint32_t v) {
    head.bytes()[at + 0] = v & 0xFF;
    head.bytes()[at + 1] = (v >> 8) & 0xFF;
    head.bytes()[at + 2] = (v >> 16) & 0xFF;
    head.bytes()[at + 3] = (v >> 24) & 0xFF;
  };
  patch(patchOffsetTablePos, offsetTablePos);
  patch(patchCardDataPos, cardDataPos);

  // Assemble the full buffer.
  std::vector<uint8_t> out = head.bytes();
  uint32_t cursor = cardDataPos;
  std::vector<uint32_t> offsets;
  for (const auto& rec : records) {
    offsets.push_back(cursor);
    cursor += static_cast<uint32_t>(rec.size());
  }
  for (uint32_t off : offsets) {
    out.push_back(off & 0xFF);
    out.push_back((off >> 8) & 0xFF);
    out.push_back((off >> 16) & 0xFF);
    out.push_back((off >> 24) & 0xFF);
  }
  for (const auto& rec : records) out.insert(out.end(), rec.begin(), rec.end());
  return out;
}

}  // namespace testing
