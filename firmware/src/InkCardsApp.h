// InkCards application state machine. Drives the deck picker, the session
// summary and the card review flow on top of the FreeInk SDK renderer and the
// InkCards portable core. One deck is open at a time; card content is streamed
// from SD on demand, never held whole in RAM.
#pragma once

#include <GfxRenderer.h>
#include <HalStorage.h>

#include <memory>
#include <string>
#include <vector>

#include "DeckReader.h"
#include "ReviewState.h"
#include "Session.h"
#include "platform/InkInput.h"
#include "platform/SdByteStream.h"
#include "ui/Theme.h"

namespace inkcards {

// Supplies the current local day number to the scheduler. Injected so the app
// does not depend directly on a particular clock source.
using DayProvider = uint32_t (*)();

class InkCardsApp {
 public:
  InkCardsApp(GfxRenderer& renderer, InkInput& input, const InkFonts& fonts, DayProvider dayProvider,
              const SessionConfig& config)
      : renderer_(renderer), input_(input), fonts_(fonts), day_(dayProvider), config_(config) {}

  // Draw the initial screen. Call once after hardware/font init.
  void begin();

  // Poll input and advance the UI. Call every main-loop iteration.
  void loop();

 private:
  enum class Screen { DeckList, Summary, Review, Done };

  GfxRenderer& renderer_;
  InkInput& input_;
  InkFonts fonts_;
  DayProvider day_;
  SessionConfig config_;

  Screen screen_ = Screen::DeckList;

  // Deck picker state.
  std::vector<std::string> deckPaths_;
  std::vector<std::string> deckNames_;
  int selected_ = 0;

  // Open-deck state. The HalFile must stay open for the life of the session so
  // the DeckReader can seek into it; hence it lives here.
  HalFile deckFile_;
  std::unique_ptr<SdFileReader> deckStream_;
  std::unique_ptr<DeckReader> deck_;
  ReviewStore store_;
  std::unique_ptr<Session> session_;
  std::string openDeckName_;

  // Review state.
  DeckCard card_;
  bool showAnswer_ = false;
  int partialCount_ = 0;  // fast refreshes since the last full refresh

  // Screen transitions.
  void showDeckList();
  void openSelectedDeck();
  void showSummary(bool finished);
  void startReview();
  void loadCurrentCard();
  void revealAnswer();
  void applyGrade(Grade grade);
  void closeDeck();

  // Input handlers per screen.
  void handleDeckListInput();
  void handleSummaryInput();
  void handleReviewInput();
  void handleDoneInput();

  // Push the framebuffer, choosing a full refresh periodically to clear e-ink
  // ghosting and a fast refresh otherwise. Passing forceFull=true always does a
  // full refresh (screen changes); otherwise it fast-refreshes and every
  // kFullRefreshEvery-th call promotes to a full refresh.
  void present(bool forceFull);

  static constexpr int kFullRefreshEvery = 8;
};

}  // namespace inkcards
