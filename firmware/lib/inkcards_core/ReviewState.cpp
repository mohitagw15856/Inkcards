#include "ReviewState.h"

#include "InkCardsFormat.h"
#include "Serialization.h"

namespace inkcards {

void ReviewStore::reset() {
  newIntroduced = 0;
  lastReviewDay = 0;
  streakDays = 0;
  records_.clear();
}

bool ReviewStore::load(ByteReader& r) {
  reset();
  if (!r.seek(0)) return false;

  uint8_t magic[4];
  if (r.read(magic, 4) != 4) return false;
  for (int i = 0; i < 4; ++i) {
    if (magic[i] != kRevMagic[i]) return false;
  }

  uint8_t version, reserved;
  if (!readU8(r, version)) return false;
  if (version != kRevVersion) return false;
  if (!readU8(r, reserved)) return false;
  if (!readU16(r, newIntroduced)) return false;
  if (!readU32(r, lastReviewDay)) return false;
  if (!readU32(r, streakDays)) return false;

  uint32_t count;
  if (!readU32(r, count)) return false;

  records_.reserve(count);
  uint32_t prevId = 0;
  for (uint32_t i = 0; i < count; ++i) {
    ReviewRecord rec;
    uint8_t reps, lapses, lastGrade, rsv;
    if (!readU32(r, rec.cardId)) return false;
    if (!readU32(r, rec.sched.dueDay)) return false;
    if (!readU16(r, rec.sched.intervalDays)) return false;
    if (!readU16(r, rec.sched.easinessMilli)) return false;
    if (!readU8(r, reps)) return false;
    if (!readU8(r, lapses)) return false;
    if (!readU8(r, lastGrade)) return false;
    if (!readU8(r, rsv)) return false;
    rec.sched.reps = reps;
    rec.sched.lapses = lapses;
    rec.sched.lastGrade = lastGrade;
    // Records must be strictly ascending by cardId; tolerate equal-or-less by
    // rejecting so a corrupt file does not silently break lookups.
    if (i > 0 && rec.cardId <= prevId) return false;
    prevId = rec.cardId;
    records_.push_back(rec);
  }
  return true;
}

bool ReviewStore::save(ByteWriter& w) const {
  w.write(kRevMagic, 4);
  writeU8(w, kRevVersion);
  writeU8(w, 0);  // reserved
  writeU16(w, newIntroduced);
  writeU32(w, lastReviewDay);
  writeU32(w, streakDays);
  writeU32(w, static_cast<uint32_t>(records_.size()));

  for (const auto& rec : records_) {
    writeU32(w, rec.cardId);
    writeU32(w, rec.sched.dueDay);
    writeU16(w, rec.sched.intervalDays);
    writeU16(w, rec.sched.easinessMilli);
    writeU8(w, rec.sched.reps);
    writeU8(w, rec.sched.lapses);
    writeU8(w, rec.sched.lastGrade);
    writeU8(w, 0);  // reserved
  }
  return true;
}

size_t ReviewStore::lowerBound(uint32_t cardId) const {
  size_t lo = 0, hi = records_.size();
  while (lo < hi) {
    size_t mid = lo + (hi - lo) / 2;
    if (records_[mid].cardId < cardId) {
      lo = mid + 1;
    } else {
      hi = mid;
    }
  }
  return lo;
}

const CardSchedule* ReviewStore::find(uint32_t cardId) const {
  size_t i = lowerBound(cardId);
  if (i < records_.size() && records_[i].cardId == cardId) return &records_[i].sched;
  return nullptr;
}

void ReviewStore::upsert(uint32_t cardId, const CardSchedule& sched) {
  size_t i = lowerBound(cardId);
  if (i < records_.size() && records_[i].cardId == cardId) {
    records_[i].sched = sched;
  } else {
    records_.insert(records_.begin() + i, ReviewRecord{cardId, sched});
  }
}

uint32_t ReviewStore::dueCount(uint32_t todayDay) const {
  uint32_t n = 0;
  for (const auto& rec : records_) {
    if (isDue(rec.sched, todayDay)) ++n;
  }
  return n;
}

}  // namespace inkcards
