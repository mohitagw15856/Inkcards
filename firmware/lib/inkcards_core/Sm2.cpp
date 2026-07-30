#include "Sm2.h"

namespace inkcards {

namespace {

// Map the four review buttons onto SM-2 quality responses (0..5).
//   Again -> 1  (failed recall, forces a lapse)
//   Hard  -> 3  (recalled with serious difficulty; lowest "correct" grade)
//   Good  -> 4  (recalled after hesitation; easiness unchanged)
//   Easy  -> 5  (perfect recall; easiness rises)
int qualityFor(Grade grade) {
  switch (grade) {
    case GRADE_AGAIN: return 1;
    case GRADE_HARD: return 3;
    case GRADE_GOOD: return 4;
    case GRADE_EASY: return 5;
  }
  return 4;
}

// Round a non-negative double to the nearest uint32_t.
uint32_t roundu(double x) { return static_cast<uint32_t>(x + 0.5); }

}  // namespace

CardSchedule sm2Apply(const CardSchedule& prev, Grade grade, uint32_t todayDay) {
  CardSchedule next = prev;
  const int q = qualityFor(grade);

  // Update the easiness factor. This happens on every review in SM-2,
  // including lapses. EF' = EF + (0.1 - (5-q) * (0.08 + (5-q) * 0.02)).
  double ef = prev.easinessMilli / 1000.0;
  const double diff = 5 - q;
  ef = ef + (0.1 - diff * (0.08 + diff * 0.02));
  if (ef < 1.3) ef = 1.3;
  next.easinessMilli = static_cast<uint16_t>(roundu(ef * 1000.0));

  if (q < 3) {
    // Lapse: relearn from scratch, due again tomorrow.
    next.reps = 0;
    next.intervalDays = 1;
    if (next.lapses < 255) next.lapses += 1;
  } else {
    // Correct: advance the repetition count and grow the interval.
    uint16_t interval;
    if (prev.reps == 0) {
      interval = 1;
    } else if (prev.reps == 1) {
      interval = 6;
    } else {
      uint32_t grown = roundu(prev.intervalDays * ef);
      if (grown < 1) grown = 1;
      if (grown > 0xFFFF) grown = 0xFFFF;
      interval = static_cast<uint16_t>(grown);
    }
    next.intervalDays = interval;
    if (next.reps < 255) next.reps += 1;
  }

  next.dueDay = todayDay + next.intervalDays;
  next.lastGrade = static_cast<uint8_t>(grade);
  return next;
}

}  // namespace inkcards
