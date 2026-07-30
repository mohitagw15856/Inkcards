// Adapter exposing an Arduino fs::File as the InkCards core ByteReader, used by
// the self-test firmware (which reads the SD card with the Arduino SD library
// rather than the FreeInk SDK).
#pragma once

#include <FS.h>

#include "ByteStream.h"

namespace inkcards {

class ArduinoFileReader : public ByteReader {
 public:
  explicit ArduinoFileReader(fs::File& file) : file_(file), size_(static_cast<uint32_t>(file.size())) {}

  bool seek(uint32_t pos) override {
    if (pos > size_) return false;
    return file_.seek(pos);
  }
  uint32_t position() const override { return static_cast<uint32_t>(file_.position()); }
  uint32_t size() const override { return size_; }
  size_t read(void* dst, size_t n) override { return file_.read(static_cast<uint8_t*>(dst), n); }

 private:
  fs::File& file_;
  uint32_t size_;
};

}  // namespace inkcards
