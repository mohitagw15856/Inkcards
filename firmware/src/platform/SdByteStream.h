// Adapters that expose a FreeInk SDK HalFile as the InkCards core's ByteReader /
// ByteWriter interfaces, so the portable deck/state code runs unchanged on the
// device against SD-card files.
//
// The raw HalFile seek/position/size/read/write calls live once in inkkit
// (inkkit/SdStream.h); these classes just re-present inkkit's stream as the
// core's ByteReader/ByteWriter so InkCardsApp and ReviewStore signatures are
// untouched.
#pragma once

#include <inkkit/SdStream.h>

#include <cstddef>
#include <cstdint>

#include "ByteStream.h"

namespace inkcards {

// Wraps an already-opened HalFile for random-access reading.
class SdFileReader : public ByteReader {
 public:
  explicit SdFileReader(HalFile& file) : impl_(file) {}

  bool seek(uint32_t pos) override { return impl_.seek(pos); }
  uint32_t position() const override { return impl_.position(); }
  uint32_t size() const override { return impl_.size(); }
  size_t read(void* dst, size_t n) override { return impl_.read(dst, n); }

 private:
  inkkit::SdFileReader impl_;
};

// Wraps an already-opened HalFile for sequential writing.
class SdFileWriter : public ByteWriter {
 public:
  explicit SdFileWriter(HalFile& file) : impl_(file) {}
  size_t write(const void* src, size_t n) override { return impl_.write(src, n); }

 private:
  inkkit::SdFileWriter impl_;
};

}  // namespace inkcards
