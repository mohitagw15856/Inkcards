// Streaming reader for .deck files. Holds at most one card in RAM and seeks
// directly to any card by index via the on-disk offset table. See
// docs/FORMAT.md for the format this parses.
#pragma once

#include <cstdint>
#include <string>

#include "ByteStream.h"
#include "InkCardsFormat.h"

namespace inkcards {

// One decoded card. Absent fields are empty strings; `present` tracks which
// fields the record actually carried so an empty gloss can be distinguished
// from a missing one where it matters.
struct DeckCard {
  uint32_t cardId = 0;
  std::string front;
  std::string back;
  std::string pinyin;
  std::string example;
  std::string notes;
  std::string hint;
  std::string tags;
  uint16_t present = 0;  // bitmask of fieldMaskBit(type)

  bool has(FieldType t) const { return (present & fieldMaskBit(t)) != 0; }
  void clear() { *this = DeckCard{}; }
};

// Parsed header metadata, cheap to keep around.
struct DeckHeader {
  uint8_t version = 0;
  uint8_t flags = 0;
  uint16_t fieldMask = 0;
  uint32_t cardCount = 0;
  uint32_t offsetTablePos = 0;
  uint32_t cardDataPos = 0;
  std::string name;
  std::string description;
  std::string fontHint;

  bool hasPinyin() const { return (flags & DECK_FLAG_HAS_PINYIN) != 0; }
};

class DeckReader {
 public:
  // Binds to a reader positioned at the start of a .deck file. The reader must
  // outlive the DeckReader.
  explicit DeckReader(ByteReader& reader) : reader_(reader) {}

  // Parse and validate the header. Returns false on bad magic/version or a
  // truncated file. Must be called before cardCount()/readCard().
  bool open();

  bool isOpen() const { return open_; }
  const DeckHeader& header() const { return header_; }
  uint32_t cardCount() const { return header_.cardCount; }

  // Look up the absolute file offset of card `index` from the offset table
  // (reads four bytes, does not cache the whole table). Returns false if index
  // is out of range or the read fails.
  bool cardOffset(uint32_t index, uint32_t& offsetOut);

  // Read card `index` fully into `out`. Streams the record; peak extra RAM is
  // the card's own field strings.
  bool readCard(uint32_t index, DeckCard& out);

  // Read only the stable cardId of card `index` (four bytes), without decoding
  // the record body. Used when building the due queue over many cards.
  bool readCardId(uint32_t index, uint32_t& cardIdOut);

 private:
  ByteReader& reader_;
  DeckHeader header_;
  bool open_ = false;

  bool readCardAt(uint32_t offset, DeckCard& out);
};

}  // namespace inkcards
