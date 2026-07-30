// FNV-1a 32-bit hash, used for the stable deck id that pairs a .deck with its
// .rev file. Must match companion/inkcards/deckformat.py exactly.
#pragma once

#include <cstdint>
#include <string>

namespace inkcards {

inline uint32_t fnv1a32(const std::string& s) {
  uint32_t hash = 0x811c9dc5u;
  for (unsigned char c : s) {
    hash ^= c;
    hash *= 0x01000193u;
  }
  return hash;
}

// Eight lowercase hex digits, as used for the .rev filename.
inline std::string deckIdHex(const std::string& name) {
  uint32_t h = fnv1a32(name);
  static const char* digits = "0123456789abcdef";
  std::string out(8, '0');
  for (int i = 7; i >= 0; --i) {
    out[i] = digits[h & 0xF];
    h >>= 4;
  }
  return out;
}

}  // namespace inkcards
