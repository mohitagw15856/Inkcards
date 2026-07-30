// SM-2 spaced-repetition scheduling, pure and side-effect free so it can be
// unit tested on the host. The four on-device grades map onto SM-2 quality
// values as documented in docs/SPACED_REPETITION.md.
#pragma once

#include <cstdint>

#include "InkCardsFormat.h"

namespace inkcards {

// Per-card scheduling state. Mirrors the .rev card record (see docs/FORMAT.md)
// but in a convenient in-memory shape. easinessMilli is the SM-2 easiness
// factor times 1000; the SM-2 default is 2.5 -> 2500.
struct CardSchedule {
  uint16_t intervalDays = 0;
  uint16_t easinessMilli = 2500;
  uint8_t reps = 0;
  uint8_t lapses = 0;
  uint32_t dueDay = 0;
  uint8_t lastGrade = GRADE_GOOD;
};

// SM-2 minimum easiness factor (1.3) in the milli representation.
constexpr uint16_t kMinEasinessMilli = 1300;
constexpr uint16_t kDefaultEasinessMilli = 2500;

// Apply a grade to a card's schedule as of day `todayDay`, returning the new
// schedule. `prev` is the state before this review (default-constructed for a
// brand-new card being graded for the first time).
CardSchedule sm2Apply(const CardSchedule& prev, Grade grade, uint32_t todayDay);

// True when a card with this schedule is due to be reviewed on `todayDay`.
inline bool isDue(const CardSchedule& s, uint32_t todayDay) { return s.dueDay <= todayDay; }

}  // namespace inkcards
