// Little-endian integer, varint (unsigned LEB128) and length-prefixed string
// helpers over the ByteReader / ByteWriter interfaces. See docs/FORMAT.md.
#pragma once

#include <cstdint>
#include <string>

#include "ByteStream.h"

namespace inkcards {

// --- reading ---------------------------------------------------------------

inline bool readU8(ByteReader& r, uint8_t& out) { return r.read(&out, 1) == 1; }

inline bool readU16(ByteReader& r, uint16_t& out) {
  uint8_t b[2];
  if (r.read(b, 2) != 2) return false;
  out = static_cast<uint16_t>(b[0] | (b[1] << 8));
  return true;
}

inline bool readU32(ByteReader& r, uint32_t& out) {
  uint8_t b[4];
  if (r.read(b, 4) != 4) return false;
  out = static_cast<uint32_t>(b[0]) | (static_cast<uint32_t>(b[1]) << 8) | (static_cast<uint32_t>(b[2]) << 16) |
        (static_cast<uint32_t>(b[3]) << 24);
  return true;
}

// Unsigned LEB128. Reads at most 5 bytes (enough for a 32-bit value).
inline bool readVarint(ByteReader& r, uint32_t& out) {
  uint32_t result = 0;
  int shift = 0;
  for (int i = 0; i < 5; ++i) {
    uint8_t byte;
    if (r.read(&byte, 1) != 1) return false;
    result |= static_cast<uint32_t>(byte & 0x7F) << shift;
    if ((byte & 0x80) == 0) {
      out = result;
      return true;
    }
    shift += 7;
  }
  return false;  // overlong / malformed
}

// varint length prefix followed by that many UTF-8 bytes.
inline bool readString(ByteReader& r, std::string& out) {
  uint32_t len;
  if (!readVarint(r, len)) return false;
  out.resize(len);
  if (len == 0) return true;
  return r.read(&out[0], len) == len;
}

// Skip a length-prefixed string without materialising it.
inline bool skipString(ByteReader& r) {
  uint32_t len;
  if (!readVarint(r, len)) return false;
  return r.seek(r.position() + len);
}

// --- writing ---------------------------------------------------------------

inline void writeU8(ByteWriter& w, uint8_t v) { w.write(&v, 1); }

inline void writeU16(ByteWriter& w, uint16_t v) {
  uint8_t b[2] = {static_cast<uint8_t>(v & 0xFF), static_cast<uint8_t>((v >> 8) & 0xFF)};
  w.write(b, 2);
}

inline void writeU32(ByteWriter& w, uint32_t v) {
  uint8_t b[4] = {static_cast<uint8_t>(v & 0xFF), static_cast<uint8_t>((v >> 8) & 0xFF),
                  static_cast<uint8_t>((v >> 16) & 0xFF), static_cast<uint8_t>((v >> 24) & 0xFF)};
  w.write(b, 4);
}

inline void writeVarint(ByteWriter& w, uint32_t v) {
  uint8_t byte;
  do {
    byte = v & 0x7F;
    v >>= 7;
    if (v) byte |= 0x80;
    w.write(&byte, 1);
  } while (v);
}

inline void writeString(ByteWriter& w, const std::string& s) {
  writeVarint(w, static_cast<uint32_t>(s.size()));
  if (!s.empty()) w.write(s.data(), s.size());
}

}  // namespace inkcards
