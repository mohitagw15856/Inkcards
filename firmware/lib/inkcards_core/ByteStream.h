// Minimal seekable byte-stream abstractions used by the InkCards core so the
// same parsing/serialisation code runs against an SD-card file on the device
// and against an in-memory buffer in host unit tests.
//
// No Arduino / FreeInk SDK dependency here on purpose: device adapters live in
// firmware/src/platform and wrap these interfaces around SdFat's File.
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace inkcards {

// Read-only random-access source (a .deck file, or a .rev file being read).
class ByteReader {
 public:
  virtual ~ByteReader() = default;
  // Move the read cursor to an absolute position. Returns false if out of range.
  virtual bool seek(uint32_t pos) = 0;
  virtual uint32_t position() const = 0;
  virtual uint32_t size() const = 0;
  // Read up to n bytes into dst; returns the number of bytes actually read.
  virtual size_t read(void* dst, size_t n) = 0;
};

// Sequential sink (a .rev file being written). Writing is append-only from the
// caller's point of view; the whole small file is rewritten each save.
class ByteWriter {
 public:
  virtual ~ByteWriter() = default;
  virtual size_t write(const void* src, size_t n) = 0;
};

// In-memory reader, primarily for host tests but also handy for small assets.
class MemoryReader : public ByteReader {
 public:
  MemoryReader(const uint8_t* data, uint32_t len) : data_(data), len_(len) {}
  explicit MemoryReader(const std::vector<uint8_t>& v) : data_(v.data()), len_(static_cast<uint32_t>(v.size())) {}

  bool seek(uint32_t pos) override {
    if (pos > len_) return false;
    pos_ = pos;
    return true;
  }
  uint32_t position() const override { return pos_; }
  uint32_t size() const override { return len_; }
  size_t read(void* dst, size_t n) override {
    if (pos_ >= len_) return 0;
    size_t avail = len_ - pos_;
    size_t take = n < avail ? n : avail;
    if (take && dst) {
      const uint8_t* src = data_ + pos_;
      auto* out = static_cast<uint8_t*>(dst);
      for (size_t i = 0; i < take; ++i) out[i] = src[i];
    }
    pos_ += static_cast<uint32_t>(take);
    return take;
  }

 private:
  const uint8_t* data_;
  uint32_t len_;
  uint32_t pos_ = 0;
};

// In-memory writer that accumulates into a growable buffer.
class MemoryWriter : public ByteWriter {
 public:
  size_t write(const void* src, size_t n) override {
    const auto* in = static_cast<const uint8_t*>(src);
    buf_.insert(buf_.end(), in, in + n);
    return n;
  }
  const std::vector<uint8_t>& bytes() const { return buf_; }
  std::vector<uint8_t>& bytes() { return buf_; }

 private:
  std::vector<uint8_t> buf_;
};

}  // namespace inkcards
