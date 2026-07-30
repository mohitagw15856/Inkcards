// A study session: turns a deck plus its review state into an ordered queue of
// cards to review today, and applies grades back to the review state. Pure
// logic over DeckReader + ReviewStore, so it is fully host-testable.
//
// Memory note: the queue holds one small entry (a card index plus a flag) per
// card scheduled for the session, never the card content. Card content is read
// one card at a time from the DeckReader on demand.
#pragma once

#include <cstdint>
#include <vector>

#include "DeckReader.h"
#include "InkCardsFormat.h"
#include "ReviewState.h"

namespace inkcards {

struct SessionConfig {
  uint16_t newCardsPerDay = 20;  // cap on brand-new cards introduced per day
  uint16_t maxReviews = 0;       // cap on scheduled reviews (0 = unlimited)
};

// Snapshot of session progress for the session screen.
struct SessionStats {
  uint32_t dueReviews = 0;    // existing cards that came due today
  uint32_t newCards = 0;      // brand-new cards scheduled today
  uint32_t reviewedToday = 0; // cards graded so far (this day)
  uint32_t remaining = 0;     // cards still queued in this session
  uint32_t streakDays = 0;    // consecutive-day study streak
};

class Session {
 public:
  Session(DeckReader& deck, ReviewStore& store, const SessionConfig& config)
      : deck_(deck), store_(store), config_(config) {}

  // Build today's queue. `todayDay` is the integer day number (see FORMAT.md).
  // Rolls the per-day counters if the stored day has advanced. Returns false if
  // the deck reader is not open.
  bool build(uint32_t todayDay);

  // True while there are cards left to review in this session.
  bool hasCurrent() const { return pos_ < queue_.size(); }

  // Deck index of the card currently being reviewed. Only valid when
  // hasCurrent() is true.
  uint32_t currentIndex() const { return queue_[pos_].deckIndex; }

  // Whether the current card is being seen for the first time.
  bool currentIsNew() const { return queue_[pos_].isNew; }

  // Grade the current card, update its schedule and the review state, advance
  // to the next card, and update the day/streak bookkeeping. No-op if there is
  // no current card.
  void grade(Grade grade, uint32_t todayDay);

  SessionStats stats() const;

 private:
  struct QueueItem {
    uint32_t deckIndex;
    uint32_t cardId;
    bool isNew;
  };

  DeckReader& deck_;
  ReviewStore& store_;
  SessionConfig config_;

  std::vector<QueueItem> queue_;
  size_t pos_ = 0;
  uint32_t reviewedToday_ = 0;
  uint32_t dueReviews_ = 0;
  uint32_t newCards_ = 0;

  // Advance the streak / day counters on the first grade of a new day.
  void recordReviewDay(uint32_t todayDay);
};

}  // namespace inkcards
