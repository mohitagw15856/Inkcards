#include "Session.h"

namespace inkcards {

bool Session::build(uint32_t todayDay) {
  queue_.clear();
  pos_ = 0;
  reviewedToday_ = 0;
  dueReviews_ = 0;
  newCards_ = 0;
  if (!deck_.isOpen()) return false;

  // Roll the per-day new-card counter when the day advances. reviewedToday
  // starts at zero each session; the streak/lastReviewDay in the store are only
  // moved forward when a card is actually graded (recordReviewDay).
  bool sameDay = (store_.lastReviewDay == todayDay);
  uint16_t introducedToday = sameDay ? store_.newIntroduced : 0;
  uint16_t newBudget = 0;
  if (config_.newCardsPerDay > introducedToday) {
    newBudget = static_cast<uint16_t>(config_.newCardsPerDay - introducedToday);
  }

  // Single streaming pass over the deck. For each card read only its id (four
  // bytes), classify it as a due review or an as-yet-unseen new card, and queue
  // accordingly. Due reviews are queued ahead of new cards.
  const uint32_t total = deck_.cardCount();
  std::vector<QueueItem> newItems;
  for (uint32_t i = 0; i < total; ++i) {
    uint32_t cardId;
    if (!deck_.readCardId(i, cardId)) continue;

    const CardSchedule* sched = store_.find(cardId);
    if (sched) {
      if (isDue(*sched, todayDay)) {
        if (config_.maxReviews == 0 || dueReviews_ < config_.maxReviews) {
          queue_.push_back(QueueItem{i, cardId, false});
          ++dueReviews_;
        }
      }
    } else {
      if (newCards_ < newBudget) {
        newItems.push_back(QueueItem{i, cardId, true});
        ++newCards_;
      }
    }
  }

  for (const auto& it : newItems) queue_.push_back(it);
  return true;
}

void Session::recordReviewDay(uint32_t todayDay) {
  if (store_.lastReviewDay == todayDay) return;  // already counted today

  if (store_.lastReviewDay + 1 == todayDay) {
    store_.streakDays += 1;  // consecutive day
  } else {
    store_.streakDays = 1;  // streak broken (or first ever day)
  }
  store_.lastReviewDay = todayDay;
  store_.newIntroduced = 0;  // reset per-day new-card counter on the new day
}

void Session::grade(Grade grade, uint32_t todayDay) {
  if (!hasCurrent()) return;

  recordReviewDay(todayDay);

  const QueueItem& item = queue_[pos_];
  const CardSchedule* prev = store_.find(item.cardId);
  CardSchedule base = prev ? *prev : CardSchedule{};
  CardSchedule updated = sm2Apply(base, grade, todayDay);
  store_.upsert(item.cardId, updated);

  if (item.isNew && store_.newIntroduced < 0xFFFF) {
    store_.newIntroduced += 1;
  }

  ++reviewedToday_;
  ++pos_;
}

SessionStats Session::stats() const {
  SessionStats s;
  s.dueReviews = dueReviews_;
  s.newCards = newCards_;
  s.reviewedToday = reviewedToday_;
  s.remaining = (pos_ < queue_.size()) ? static_cast<uint32_t>(queue_.size() - pos_) : 0;
  s.streakDays = store_.streakDays;
  return s;
}

}  // namespace inkcards
