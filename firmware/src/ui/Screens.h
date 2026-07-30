// Stateless draw routines for the InkCards screens. Each takes the renderer and
// the data to show and paints into the framebuffer; the caller decides when to
// push a full or partial refresh. Keeping these free of state makes the redraw
// policy (full screen vs card-area-only) explicit at the call site.
#pragma once

#include <GfxRenderer.h>

#include <string>
#include <vector>

#include "DeckReader.h"
#include "Session.h"
#include "Theme.h"

namespace inkcards {

// The full deck-picker screen.
void drawDeckList(GfxRenderer& r, const InkFonts& fonts, const std::vector<std::string>& deckNames, int selected);

// The session summary shown before and after a review run.
void drawSessionSummary(GfxRenderer& r, const InkFonts& fonts, const std::string& deckName, const SessionStats& stats,
                        bool finished);

// Header + footer chrome for the review screen (drawn once per card entry, full
// refresh). The card band itself is painted by drawCardArea.
void drawReviewChrome(GfxRenderer& r, const InkFonts& fonts, const std::string& deckName, const SessionStats& stats);

// The card content band only. `showAnswer` toggles between prompt and answer.
// This is the region pushed with a partial refresh between cards and on reveal.
void drawCardArea(GfxRenderer& r, const InkFonts& fonts, const DeckCard& card, bool showAnswer, bool deckHasPinyin);

// Footer button-label row for the review screen, reflecting the current phase
// (reveal vs grade). Drawn inside the footer band.
void drawReviewFooter(GfxRenderer& r, const InkFonts& fonts, bool showAnswer);

}  // namespace inkcards
