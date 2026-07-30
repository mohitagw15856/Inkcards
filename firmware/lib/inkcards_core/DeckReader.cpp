#include "DeckReader.h"

#include "Serialization.h"

namespace inkcards {

bool DeckReader::open() {
  open_ = false;
  if (!reader_.seek(0)) return false;

  uint8_t magic[4];
  if (reader_.read(magic, 4) != 4) return false;
  for (int i = 0; i < 4; ++i) {
    if (magic[i] != kDeckMagic[i]) return false;
  }

  if (!readU8(reader_, header_.version)) return false;
  if (header_.version != kDeckVersion) return false;
  if (!readU8(reader_, header_.flags)) return false;
  if (!readU16(reader_, header_.fieldMask)) return false;
  if (!readU32(reader_, header_.cardCount)) return false;
  if (!readU32(reader_, header_.offsetTablePos)) return false;
  if (!readU32(reader_, header_.cardDataPos)) return false;
  if (!readString(reader_, header_.name)) return false;
  if (!readString(reader_, header_.description)) return false;
  if (!readString(reader_, header_.fontHint)) return false;

  // Sanity: the offset table must sit within the file and hold cardCount u32s.
  uint32_t tableEnd = header_.offsetTablePos + header_.cardCount * 4u;
  if (header_.offsetTablePos < 4 || tableEnd > reader_.size()) return false;

  open_ = true;
  return true;
}

bool DeckReader::cardOffset(uint32_t index, uint32_t& offsetOut) {
  if (!open_ || index >= header_.cardCount) return false;
  if (!reader_.seek(header_.offsetTablePos + index * 4u)) return false;
  return readU32(reader_, offsetOut);
}

bool DeckReader::readCard(uint32_t index, DeckCard& out) {
  uint32_t offset;
  if (!cardOffset(index, offset)) return false;
  return readCardAt(offset, out);
}

bool DeckReader::readCardId(uint32_t index, uint32_t& cardIdOut) {
  uint32_t offset;
  if (!cardOffset(index, offset)) return false;
  if (!reader_.seek(offset)) return false;
  return readU32(reader_, cardIdOut);
}

bool DeckReader::readCardAt(uint32_t offset, DeckCard& out) {
  out.clear();
  if (!reader_.seek(offset)) return false;
  if (!readU32(reader_, out.cardId)) return false;

  uint8_t fieldCount;
  if (!readU8(reader_, fieldCount)) return false;

  for (uint8_t i = 0; i < fieldCount; ++i) {
    uint8_t type;
    if (!readU8(reader_, type)) return false;

    // Route known fields to their slot; skip unknown ones so the format can
    // grow without breaking this reader.
    std::string* slot = nullptr;
    switch (type) {
      case FIELD_FRONT: slot = &out.front; break;
      case FIELD_BACK: slot = &out.back; break;
      case FIELD_PINYIN: slot = &out.pinyin; break;
      case FIELD_EXAMPLE: slot = &out.example; break;
      case FIELD_NOTES: slot = &out.notes; break;
      case FIELD_HINT: slot = &out.hint; break;
      case FIELD_TAGS: slot = &out.tags; break;
      default: break;
    }

    if (slot) {
      if (!readString(reader_, *slot)) return false;
      out.present |= fieldMaskBit(static_cast<FieldType>(type));
    } else {
      if (!skipString(reader_)) return false;
    }
  }
  return true;
}

}  // namespace inkcards
