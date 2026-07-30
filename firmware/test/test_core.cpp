// Host unit tests for the InkCards portable core. Built and run by CI (see
// firmware/test/README.md and .github/workflows/ci.yml).
#include <cstdio>

#include "ByteStream.h"
#include "DeckReader.h"
#include "Fnv.h"
#include "ReviewState.h"
#include "Serialization.h"
#include "Session.h"
#include "Sm2.h"
#include "TimeUtil.h"
#include "check.h"
#include "deck_builder.h"

using namespace inkcards;
using testing::BuilderCard;
using testing::buildDeck;

static void test_varint() {
  const uint32_t values[] = {0, 1, 127, 128, 300, 16384, 0xFFFFFFFF};
  for (uint32_t v : values) {
    MemoryWriter w;
    writeVarint(w, v);
    MemoryReader r(w.bytes());
    uint32_t got = 0;
    CHECK(readVarint(r, got));
    CHECK_EQ(got, v);
  }
  // String round-trip including empty and UTF-8 (CJK).
  const std::string samples[] = {"", "hello", u8"你好", u8"中文 mixed"};
  for (const auto& s : samples) {
    MemoryWriter w;
    writeString(w, s);
    MemoryReader r(w.bytes());
    std::string got;
    CHECK(readString(r, got));
    CHECK_EQ(got, s);
  }
}

static void test_fnv() {
  // Canonical FNV-1a 32-bit test vectors.
  CHECK_EQ(fnv1a32(""), 0x811c9dc5u);
  CHECK_EQ(fnv1a32("a"), 0xe40c292cu);
  CHECK_EQ(fnv1a32("foobar"), 0xbf9cf968u);
  CHECK_EQ(deckIdHex("a"), std::string("e40c292c"));
}

static void test_deck_roundtrip() {
  std::vector<BuilderCard> cards = {
      {1, {{FIELD_FRONT, u8"你好"}, {FIELD_PINYIN, u8"nǐ hǎo"}, {FIELD_BACK, "hello"}}},
      {2, {{FIELD_FRONT, "France"}, {FIELD_BACK, "Paris"}}},
      {7, {{FIELD_FRONT, "front3"}, {FIELD_BACK, "back3"}, {FIELD_NOTES, "a note"}}},
  };
  auto bytes = buildDeck("Demo Deck", "a description", "NotoSansSC", cards, true);
  MemoryReader r(bytes);
  DeckReader deck(r);
  CHECK(deck.open());
  CHECK_EQ(deck.cardCount(), 3u);
  CHECK(deck.header().hasPinyin());
  CHECK_EQ(deck.header().name, std::string("Demo Deck"));
  CHECK_EQ(deck.header().fontHint, std::string("NotoSansSC"));

  // Random access: read card 1 (the middle-by-id one) before card 0.
  DeckCard c;
  CHECK(deck.readCard(1, c));
  CHECK_EQ(c.cardId, 2u);
  CHECK_EQ(c.front, std::string("France"));
  CHECK_EQ(c.back, std::string("Paris"));
  CHECK(!c.has(FIELD_PINYIN));

  CHECK(deck.readCard(0, c));
  CHECK_EQ(c.cardId, 1u);
  CHECK(c.has(FIELD_PINYIN));
  CHECK_EQ(c.pinyin, std::string(u8"nǐ hǎo"));

  uint32_t id = 0;
  CHECK(deck.readCardId(2, id));
  CHECK_EQ(id, 7u);

  // Out-of-range access fails cleanly.
  CHECK(!deck.readCard(3, c));
}

static void test_deck_unknown_field() {
  // A card carrying an unknown field type (99) between known ones must still
  // parse, skipping the unknown field.
  MemoryWriter rec;
  writeU32(rec, 5);
  writeU8(rec, 3);
  writeU8(rec, FIELD_FRONT);
  writeString(rec, "F");
  writeU8(rec, 99);  // unknown
  writeString(rec, "ignored");
  writeU8(rec, FIELD_BACK);
  writeString(rec, "B");

  // Wrap a one-card deck around it by hand.
  MemoryWriter head;
  head.write(kDeckMagic, 4);
  writeU8(head, kDeckVersion);
  writeU8(head, 0);
  writeU16(head, fieldMaskBit(FIELD_FRONT) | fieldMaskBit(FIELD_BACK));
  writeU32(head, 1);
  size_t pOff = head.bytes().size();
  writeU32(head, 0);
  size_t pData = head.bytes().size();
  writeU32(head, 0);
  writeString(head, "u");
  writeString(head, "");
  writeString(head, "");
  uint32_t offTable = (uint32_t)head.bytes().size();
  uint32_t dataPos = offTable + 4;
  auto patch = [&](size_t at, uint32_t v) {
    for (int i = 0; i < 4; ++i) head.bytes()[at + i] = (v >> (8 * i)) & 0xFF;
  };
  patch(pOff, offTable);
  patch(pData, dataPos);
  std::vector<uint8_t> bytes = head.bytes();
  for (int i = 0; i < 4; ++i) bytes.push_back((dataPos >> (8 * i)) & 0xFF);
  bytes.insert(bytes.end(), rec.bytes().begin(), rec.bytes().end());

  MemoryReader r(bytes);
  DeckReader deck(r);
  CHECK(deck.open());
  DeckCard c;
  CHECK(deck.readCard(0, c));
  CHECK_EQ(c.front, std::string("F"));
  CHECK_EQ(c.back, std::string("B"));
}

static void test_sm2() {
  const uint32_t today = 100;
  CardSchedule s;  // fresh card

  // Good, Good, Good grows 1 -> 6 -> 15 (round(6 * 2.5)).
  s = sm2Apply(s, GRADE_GOOD, today);
  CHECK_EQ(s.reps, 1);
  CHECK_EQ(s.intervalDays, 1);
  CHECK_EQ(s.dueDay, today + 1);
  CHECK_EQ(s.easinessMilli, 2500);  // Good leaves EF unchanged

  s = sm2Apply(s, GRADE_GOOD, today + 1);
  CHECK_EQ(s.reps, 2);
  CHECK_EQ(s.intervalDays, 6);

  s = sm2Apply(s, GRADE_GOOD, today + 7);
  CHECK_EQ(s.reps, 3);
  CHECK_EQ(s.intervalDays, 15);

  // Easy raises EF by 0.1.
  CardSchedule e = sm2Apply(CardSchedule{}, GRADE_EASY, today);
  CHECK_EQ(e.easinessMilli, 2600);

  // Hard lowers EF by 0.14.
  CardSchedule h = sm2Apply(CardSchedule{}, GRADE_HARD, today);
  CHECK_EQ(h.easinessMilli, 2360);
  CHECK_EQ(h.reps, 1);  // Hard is still a correct answer

  // Again is a lapse: reps reset, interval 1, lapses incremented, EF drops.
  CardSchedule lap = sm2Apply(s, GRADE_AGAIN, today + 22);
  CHECK_EQ(lap.reps, 0);
  CHECK_EQ(lap.intervalDays, 1);
  CHECK_EQ(lap.lapses, 1);
  CHECK_EQ(lap.dueDay, today + 23);
  CHECK(lap.easinessMilli < s.easinessMilli);

  // EF never drops below 1.3.
  CardSchedule floorCard;
  floorCard.easinessMilli = 1300;
  floorCard = sm2Apply(floorCard, GRADE_AGAIN, today);
  CHECK_EQ(floorCard.easinessMilli, 1300);
}

static void test_review_store() {
  ReviewStore store;
  store.reset();
  CardSchedule a;
  a.dueDay = 50;
  a.intervalDays = 6;
  a.easinessMilli = 2400;
  a.reps = 2;
  store.upsert(10, a);

  CardSchedule b;
  b.dueDay = 200;
  store.upsert(3, b);  // lower id inserted after: must stay sorted

  CHECK_EQ(store.recordCount(), 2u);
  CHECK_EQ(store.records()[0].cardId, 3u);
  CHECK_EQ(store.records()[1].cardId, 10u);
  CHECK(store.find(10) != nullptr);
  CHECK_EQ(store.find(10)->intervalDays, 6);
  CHECK(store.find(999) == nullptr);

  // Upsert replaces in place.
  CardSchedule a2 = a;
  a2.intervalDays = 99;
  store.upsert(10, a2);
  CHECK_EQ(store.recordCount(), 2u);
  CHECK_EQ(store.find(10)->intervalDays, 99);

  store.newIntroduced = 4;
  store.lastReviewDay = 123;
  store.streakDays = 7;

  // Save then load must reproduce state exactly.
  MemoryWriter w;
  CHECK(store.save(w));
  MemoryReader r(w.bytes());
  ReviewStore loaded;
  CHECK(loaded.load(r));
  CHECK_EQ(loaded.recordCount(), 2u);
  CHECK_EQ(loaded.newIntroduced, 4);
  CHECK_EQ(loaded.lastReviewDay, 123u);
  CHECK_EQ(loaded.streakDays, 7u);
  CHECK_EQ(loaded.find(10)->intervalDays, 99);
  CHECK_EQ(loaded.find(3)->dueDay, 200u);
  CHECK_EQ(loaded.dueCount(60), 1u);  // only card 3 (due 200)? no: card10 due50<=60
}

static void test_review_store_due_count() {
  ReviewStore store;
  CardSchedule due;
  due.dueDay = 10;
  CardSchedule notDue;
  notDue.dueDay = 100;
  store.upsert(1, due);
  store.upsert(2, notDue);
  store.upsert(3, due);
  CHECK_EQ(store.dueCount(50), 2u);
  CHECK_EQ(store.dueCount(5), 0u);
  CHECK_EQ(store.dueCount(100), 3u);
}

static void test_session_basic() {
  // Deck of 4 cards, none seen yet. New-card budget of 2.
  std::vector<BuilderCard> cards = {
      {1, {{FIELD_FRONT, "a"}, {FIELD_BACK, "1"}}},
      {2, {{FIELD_FRONT, "b"}, {FIELD_BACK, "2"}}},
      {3, {{FIELD_FRONT, "c"}, {FIELD_BACK, "3"}}},
      {4, {{FIELD_FRONT, "d"}, {FIELD_BACK, "4"}}},
  };
  auto bytes = buildDeck("S", "", "", cards, false);
  MemoryReader r(bytes);
  DeckReader deck(r);
  CHECK(deck.open());

  ReviewStore store;
  SessionConfig cfg;
  cfg.newCardsPerDay = 2;
  Session sess(deck, store, cfg);
  const uint32_t day = 500;
  CHECK(sess.build(day));

  SessionStats st = sess.stats();
  CHECK_EQ(st.newCards, 2u);
  CHECK_EQ(st.dueReviews, 0u);
  CHECK_EQ(st.remaining, 2u);

  // Grade both new cards Good.
  CHECK(sess.hasCurrent());
  CHECK(sess.currentIsNew());
  sess.grade(GRADE_GOOD, day);
  CHECK(sess.hasCurrent());
  sess.grade(GRADE_GOOD, day);
  CHECK(!sess.hasCurrent());

  st = sess.stats();
  CHECK_EQ(st.reviewedToday, 2u);
  CHECK_EQ(st.remaining, 0u);
  CHECK_EQ(store.newIntroduced, 2);
  CHECK_EQ(store.streakDays, 1u);
  CHECK_EQ(store.lastReviewDay, day);
  CHECK_EQ(store.recordCount(), 2u);

  // Re-building the same day introduces no further new cards (budget spent),
  // and the two graded cards are scheduled for tomorrow, so nothing is due.
  CHECK(sess.build(day));
  st = sess.stats();
  CHECK_EQ(st.newCards, 0u);
  CHECK_EQ(st.dueReviews, 0u);
}

static void test_session_streak_and_due() {
  std::vector<BuilderCard> cards = {
      {1, {{FIELD_FRONT, "a"}, {FIELD_BACK, "1"}}},
      {2, {{FIELD_FRONT, "b"}, {FIELD_BACK, "2"}}},
  };
  auto bytes = buildDeck("S", "", "", cards, false);
  MemoryReader r(bytes);
  DeckReader deck(r);
  deck.open();

  ReviewStore store;
  SessionConfig cfg;
  cfg.newCardsPerDay = 10;
  Session sess(deck, store, cfg);

  // Day 1: introduce and grade both Again so they stay due tomorrow.
  sess.build(1);
  sess.grade(GRADE_AGAIN, 1);
  sess.grade(GRADE_AGAIN, 1);
  CHECK_EQ(store.streakDays, 1u);

  // Day 2 (consecutive): both cards due again as reviews. Streak advances.
  CHECK(sess.build(2));
  SessionStats st = sess.stats();
  CHECK_EQ(st.dueReviews, 2u);
  CHECK_EQ(st.newCards, 0u);
  sess.grade(GRADE_GOOD, 2);
  CHECK_EQ(store.streakDays, 2u);
  CHECK_EQ(store.newIntroduced, 0);  // reset on the new day

  // Jump to day 10 (gap): streak resets to 1 on next graded review.
  sess.build(10);
  sess.grade(GRADE_GOOD, 10);
  CHECK_EQ(store.streakDays, 1u);
}

static void test_day_number() {
  // 1970-01-02 00:00:00 UTC is day 1.
  CHECK_EQ(dayNumber(86400, 0), 1u);
  // Same instant at UTC+8 is already day 1 (plus 8h), still day 1.
  CHECK_EQ(dayNumber(86400, 8 * 3600), 1u);
  // 23:00 UTC on day 0 at UTC+8 rolls into day 1.
  CHECK_EQ(dayNumber(23 * 3600, 8 * 3600), 1u);
}

int main() {
  std::printf("InkCards core tests\n");
  RUN(test_varint);
  RUN(test_fnv);
  RUN(test_deck_roundtrip);
  RUN(test_deck_unknown_field);
  RUN(test_sm2);
  RUN(test_review_store);
  RUN(test_review_store_due_count);
  RUN(test_session_basic);
  RUN(test_session_streak_and_due);
  RUN(test_day_number);

  std::printf("\n%d checks, %d failures\n", testing::checks(), testing::failures());
  return testing::failures() == 0 ? 0 : 1;
}
