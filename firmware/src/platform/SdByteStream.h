// Adapters that expose a FreeInk SDK HalFile as the InkCards core's ByteReader /
// ByteWriter interfaces, so the portable deck/state code runs unchanged on the
// device against SD-card files.
//
// TODO(hardware-test): confirm the exact HalFile method names against the
// installed freeink-sdk version. This is written against the API used by
// CrossPoint's src (Storage.openFileForRead/Write, HalFile::read/write/close,
// seekSet/position/size). If a method differs, only this file needs updating.
#pragma once

#include <HalStorage.h>

#include <cstdint>

#include "ByteStream.h"

namespace inkcards {

// Wraps an already-opened HalFile for random-access reading.
class SdFileReader : public ByteReader {
 public:
  explicit SdFileReader(HalFile& file) : file_(file), size_(static_cast<uint32_t>(file.size())) {}

  bool seek(uint32_t pos) override {
    if (pos > size_) return false;
    return file_.seekSet(pos);
  }
  uint32_t position() const override { return static_cast<uint32_t>(file_.position()); }
  uint32_t size() const override { return size_; }
  size_t read(void* dst, size_t n) override { return file_.read(static_cast<uint8_t*>(dst), n); }

 private:
  HalFile& file_;
  uint32_t size_;
};

// Wraps an already-opened HalFile for sequential writing.
class SdFileWriter : public ByteWriter {
 public:
  explicit SdFileWriter(HalFile& file) : file_(file) {}
  size_t write(const void* src, size_t n) override {
    return file_.write(static_cast<const uint8_t*>(src), n);
  }

 private:
  HalFile& file_;
};

}  // namespace inkcards
