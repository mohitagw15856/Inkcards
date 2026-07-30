// Day-number helper. The scheduler works in whole local days rather than
// timestamps so "due today" is an integer comparison and roughly-set device
// clocks do not cause intraday churn. See docs/FORMAT.md.
#pragma once

#include <cstdint>

#include "InkCardsFormat.h"

namespace inkcards {

// Number of whole days since the Unix epoch in local time.
// tzOffsetSeconds is the local offset from UTC (e.g. +8h = 28800).
inline uint32_t dayNumber(uint64_t unixSeconds, int32_t tzOffsetSeconds) {
  int64_t local = static_cast<int64_t>(unixSeconds) + tzOffsetSeconds;
  if (local < 0) local = 0;
  return static_cast<uint32_t>(local / kSecondsPerDay);
}

}  // namespace inkcards
