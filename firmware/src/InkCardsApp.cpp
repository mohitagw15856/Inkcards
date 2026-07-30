#include "InkCardsApp.h"

#include <Logging.h>

#include "platform/InkCardsStorage.h"
#include "ui/Screens.h"

namespace inkcards {

void InkCardsApp::begin() {
  ensureStateDir();
  showDeckList();
}

void InkCardsApp::present(bool forceFull) {
  HalDisplay::RefreshMode mode = HalDisplay::FAST_REFRESH;
  if (forceFull || ++partialCount_ >= kFullRefreshEvery) {
    mode = HalDisplay::FULL_REFRESH;
    partialCount_ = 0;
  }
  renderer_.displayBuffer(mode);
}

// --- deck list -------------------------------------------------------------

void InkCardsApp::showDeckList() {
  screen_ = Screen::DeckList;
  deckPaths_ = listDeckFiles();
  deckNames_.clear();

  // Read each deck's header name for display. Only the header is parsed here,
  // not the cards.
  for (const auto& path : deckPaths_) {
    HalFile f;
    std::string name = path;  // fall back to the path if the header cannot be read
    if (Storage.openFileForRead("INK", path.c_str(), f)) {
      SdFileReader reader(f);
      DeckReader dr(reader);
      if (dr.open()) name = dr.header().name;
      f.close();
    }
    deckNames_.push_back(name);
  }

  if (selected_ >= static_cast<int>(deckNames_.size())) selected_ = 0;
  drawDeckList(renderer_, fonts_, deckNames_, selected_);
  present(/*forceFull=*/true);
}

void InkCardsApp::handleDeckListInput() {
  if (deckNames_.empty()) return;

  bool changed = false;
  if (input_.wasPressed(Btn::Front1)) {  // up
    selected_ = (selected_ + static_cast<int>(deckNames_.size()) - 1) % static_cast<int>(deckNames_.size());
    changed = true;
  } else if (input_.wasPressed(Btn::Front2)) {  // down
    selected_ = (selected_ + 1) % static_cast<int>(deckNames_.size());
    changed = true;
  }

  if (changed) {
    drawDeckList(renderer_, fonts_, deckNames_, selected_);
    present(/*forceFull=*/false);
    return;
  }

  if (input_.wasPressed(Btn::Confirm)) openSelectedDeck();
}

// --- open deck / summary ---------------------------------------------------

void InkCardsApp::openSelectedDeck() {
  closeDeck();
  const std::string& path = deckPaths_[selected_];
  if (!Storage.openFileForRead("INK", path.c_str(), deckFile_)) {
    LOG_ERR("INK", "Cannot open deck %s", path.c_str());
    return;
  }
  deckStream_.reset(new SdFileReader(deckFile_));
  deck_.reset(new DeckReader(*deckStream_));
  if (!deck_->open()) {
    LOG_ERR("INK", "Bad deck file %s", path.c_str());
    closeDeck();
    return;
  }

  openDeckName_ = deck_->header().name;
  loadReviewState(openDeckName_, store_);
  session_.reset(new Session(*deck_, store_, config_));
  session_->build(day_());
  showSummary(/*finished=*/false);
}

void InkCardsApp::showSummary(bool finished) {
  screen_ = finished ? Screen::Done : Screen::Summary;
  drawSessionSummary(renderer_, fonts_, openDeckName_, session_->stats(), finished);
  present(/*forceFull=*/true);
}

void InkCardsApp::handleSummaryInput() {
  if (input_.wasPressed(Btn::Back)) {
    closeDeck();
    showDeckList();
    return;
  }
  if (input_.wasPressed(Btn::Confirm)) {
    if (session_->hasCurrent()) {
      startReview();
    }
  }
}

// --- review ----------------------------------------------------------------

void InkCardsApp::startReview() {
  screen_ = Screen::Review;
  drawReviewChrome(renderer_, fonts_, openDeckName_, session_->stats());
  loadCurrentCard();
  present(/*forceFull=*/true);
}

void InkCardsApp::loadCurrentCard() {
  showAnswer_ = false;
  card_.clear();
  if (session_->hasCurrent()) {
    deck_->readCard(session_->currentIndex(), card_);
  }
  drawCardArea(renderer_, fonts_, card_, showAnswer_, deck_->header().hasPinyin());
  drawReviewFooter(renderer_, fonts_, showAnswer_);
}

void InkCardsApp::revealAnswer() {
  showAnswer_ = true;
  drawCardArea(renderer_, fonts_, card_, showAnswer_, deck_->header().hasPinyin());
  drawReviewFooter(renderer_, fonts_, showAnswer_);
  present(/*forceFull=*/false);
}

void InkCardsApp::applyGrade(Grade grade) {
  session_->grade(grade, day_());
  saveReviewState(openDeckName_, store_);  // persist after every grade

  if (!session_->hasCurrent()) {
    showSummary(/*finished=*/true);
    return;
  }

  // Advance to the next card: repaint only the card band and the footer, then
  // fast-refresh. The header ("N left") is refreshed on the periodic full
  // refresh rather than between every card.
  loadCurrentCard();
  present(/*forceFull=*/false);
}

void InkCardsApp::handleReviewInput() {
  if (input_.wasPressed(Btn::Back)) {
    saveReviewState(openDeckName_, store_);
    showSummary(/*finished=*/false);
    return;
  }

  if (!showAnswer_) {
    if (input_.wasAnyActionPressed()) revealAnswer();
    return;
  }

  // Answer shown: the four front buttons are the four grades, left to right.
  if (input_.wasPressed(Btn::Front1)) {
    applyGrade(GRADE_AGAIN);
  } else if (input_.wasPressed(Btn::Front2)) {
    applyGrade(GRADE_HARD);
  } else if (input_.wasPressed(Btn::Front3)) {
    applyGrade(GRADE_GOOD);
  } else if (input_.wasPressed(Btn::Front4)) {
    applyGrade(GRADE_EASY);
  }
}

// --- done ------------------------------------------------------------------

void InkCardsApp::handleDoneInput() {
  if (input_.wasPressed(Btn::Back)) {
    closeDeck();
    showDeckList();
    return;
  }
  if (input_.wasPressed(Btn::Confirm)) {
    // Rebuild for another pass (picks up anything now due, e.g. lapsed cards).
    session_->build(day_());
    if (session_->hasCurrent()) {
      startReview();
    } else {
      showSummary(/*finished=*/true);
    }
  }
}

// --- teardown --------------------------------------------------------------

void InkCardsApp::closeDeck() {
  session_.reset();
  deck_.reset();
  deckStream_.reset();
  if (deckFile_.isOpen()) deckFile_.close();
  store_.reset();
  openDeckName_.clear();
}

// --- main loop -------------------------------------------------------------

void InkCardsApp::loop() {
  input_.update();
  switch (screen_) {
    case Screen::DeckList: handleDeckListInput(); break;
    case Screen::Summary: handleSummaryInput(); break;
    case Screen::Review: handleReviewInput(); break;
    case Screen::Done: handleDoneInput(); break;
  }
}

}  // namespace inkcards
