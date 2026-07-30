// In-memory model of a deck's review state (.rev file), with load/save over the
// ByteReader/ByteWriter interfaces. See docs/FORMAT.md.
//
// The schedule records are small and fixed-size; a 1000-card deck's state is
// under 16 KB. Deck *content* is never held here, only per-card SM-2 state.
#pragma once

#include <cstdint>
#include <vector>

#include "ByteStream.h"
#include "Sm2.h"

namespace inkcards {

struct ReviewRecord {
  uint32_t cardId = 0;
  CardSchedule sched;
};

class ReviewStore {
 public:
  // Per-day counters and streak, mirrored to the .rev header.
  uint16_t newIntroduced = 0;
  uint32_t lastReviewDay = 0;
  uint32_t streakDays = 0;

  // Start from an empty state (deck never studied).
  void reset();

  // Parse a .rev file. Returns false on bad magic/version or truncation.
  // Records are expected in ascending cardId order and kept that way.
  bool load(ByteReader& r);

  // Serialise the whole state. Records are written in ascending cardId order.
  bool save(ByteWriter& w) const;

  // Look up a card's schedule, or nullptr if the card has never been graded.
  const CardSchedule* find(uint32_t cardId) const;

  // Insert or replace a card's schedule, keeping records sorted by cardId.
  void upsert(uint32_t cardId, const CardSchedule& sched);

  size_t recordCount() const { return records_.size(); }
  const std::vector<ReviewRecord>& records() const { return records_; }

  // Count records that are due on or before `todayDay`.
  uint32_t dueCount(uint32_t todayDay) const;

 private:
  std::vector<ReviewRecord> records_;  // sorted ascending by cardId

  // Index of cardId in records_, or records_.size() if absent (insertion point).
  size_t lowerBound(uint32_t cardId) const;
};

}  // namespace inkcards
