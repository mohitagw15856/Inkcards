#include "Screens.h"

#include <cstdio>

namespace inkcards {

namespace {

using EpdStyle = EpdFontFamily::Style;

// Decode one UTF-8 codepoint starting at byte i; returns the codepoint and
// advances i past it. Malformed bytes are treated as a single-byte codepoint so
// the loop always makes progress.
uint32_t nextCodepoint(const std::string& s, size_t& i) {
  unsigned char c = static_cast<unsigned char>(s[i]);
  uint32_t cp;
  size_t len;
  if (c < 0x80) {
    cp = c;
    len = 1;
  } else if ((c >> 5) == 0x6) {
    cp = c & 0x1F;
    len = 2;
  } else if ((c >> 4) == 0xE) {
    cp = c & 0x0F;
    len = 3;
  } else if ((c >> 3) == 0x1E) {
    cp = c & 0x07;
    len = 4;
  } else {
    i += 1;
    return c;
  }
  for (size_t k = 1; k < len && (i + k) < s.size(); ++k) {
    cp = (cp << 6) | (static_cast<unsigned char>(s[i + k]) & 0x3F);
  }
  i += len;
  return cp;
}

bool isCjk(uint32_t cp) {
  return (cp >= 0x2E80 && cp <= 0x9FFF) || (cp >= 0xAC00 && cp <= 0xD7AF) || (cp >= 0xF900 && cp <= 0xFAFF) ||
         (cp >= 0xFF00 && cp <= 0xFFEF) || (cp >= 0x20000 && cp <= 0x2FA1F);
}

// Split text into break units: whole Latin words, individual CJK codepoints,
// and single spaces (soft separators). This lets wrapText break between words
// for Latin scripts and between characters for CJK.
std::vector<std::string> tokenise(const std::string& text) {
  std::vector<std::string> units;
  std::string word;
  size_t i = 0;
  while (i < text.size()) {
    size_t start = i;
    uint32_t cp = nextCodepoint(text, i);
    std::string ch = text.substr(start, i - start);
    if (cp == ' ' || cp == '\t') {
      if (!word.empty()) {
        units.push_back(word);
        word.clear();
      }
      units.push_back(" ");
    } else if (cp == '\n') {
      if (!word.empty()) {
        units.push_back(word);
        word.clear();
      }
      units.push_back("\n");
    } else if (isCjk(cp)) {
      if (!word.empty()) {
        units.push_back(word);
        word.clear();
      }
      units.push_back(ch);
    } else {
      word += ch;
    }
  }
  if (!word.empty()) units.push_back(word);
  return units;
}

std::vector<std::string> wrapText(GfxRenderer& r, int fontId, const std::string& text, int maxWidth) {
  std::vector<std::string> lines;
  std::string line;
  for (const auto& unit : tokenise(text)) {
    if (unit == "\n") {
      lines.push_back(line);
      line.clear();
      continue;
    }
    std::string candidate = line + unit;
    if (r.getTextWidth(fontId, candidate.c_str(), EpdStyle::REGULAR) <= maxWidth) {
      line = candidate;
    } else {
      // Trim a trailing space before wrapping.
      while (!line.empty() && line.back() == ' ') line.pop_back();
      if (!line.empty()) lines.push_back(line);
      line = (unit == " ") ? "" : unit;
    }
  }
  while (!line.empty() && line.back() == ' ') line.pop_back();
  if (!line.empty()) lines.push_back(line);
  return lines;
}

// Draw wrapped, horizontally centred text within [x, x+width), starting at y.
// Returns the y just below the last line.
int drawWrappedCentred(GfxRenderer& r, int fontId, int x, int y, int width, const std::string& text) {
  const int lh = r.getTextHeight(fontId) + 6;
  for (const auto& line : wrapText(r, fontId, text, width)) {
    int w = r.getTextWidth(fontId, line.c_str(), EpdStyle::REGULAR);
    r.drawText(fontId, x + (width - w) / 2, y, line.c_str());
    y += lh;
  }
  return y;
}

}  // namespace

void drawDeckList(GfxRenderer& r, const InkFonts& fonts, const std::vector<std::string>& deckNames, int selected) {
  r.clearScreen();
  r.drawText(fonts.head, layout::kMargin, layout::kMargin, "InkCards");
  r.drawLine(layout::kMargin, layout::kHeaderH, layout::kScreenW - layout::kMargin, layout::kHeaderH);

  if (deckNames.empty()) {
    r.drawCenteredText(fonts.body, layout::kScreenH / 2, "No decks found");
    drawWrappedCentred(r, fonts.small, layout::kMargin, layout::kScreenH / 2 + 40,
                       layout::kScreenW - 2 * layout::kMargin, "Copy .deck files to /inkcards/decks on the SD card.");
    return;
  }

  const int rowH = r.getTextHeight(fonts.body) + 20;
  int y = layout::kHeaderH + 20;
  for (size_t i = 0; i < deckNames.size(); ++i) {
    const int rowY = y + static_cast<int>(i) * rowH;
    if (static_cast<int>(i) == selected) {
      // Highlight the selected row with a filled bar; draw its label in white.
      r.fillRect(layout::kMargin - 8, rowY - 6, layout::kScreenW - 2 * (layout::kMargin - 8), rowH, true);
      r.drawText(fonts.body, layout::kMargin, rowY, deckNames[i].c_str(), /*black=*/false);
    } else {
      r.drawText(fonts.body, layout::kMargin, rowY, deckNames[i].c_str());
    }
  }

  r.drawLine(layout::kMargin, layout::kScreenH - layout::kFooterH, layout::kScreenW - layout::kMargin,
             layout::kScreenH - layout::kFooterH);
  r.drawText(fonts.small, layout::kMargin, layout::kScreenH - layout::kFooterH + 18, "Up/Down: move    Confirm: open");
}

void drawSessionSummary(GfxRenderer& r, const InkFonts& fonts, const std::string& deckName, const SessionStats& stats,
                        bool finished) {
  r.clearScreen();
  r.drawText(fonts.head, layout::kMargin, layout::kMargin, deckName.c_str());
  r.drawLine(layout::kMargin, layout::kHeaderH, layout::kScreenW - layout::kMargin, layout::kHeaderH);

  int y = layout::kHeaderH + 48;
  char buf[64];

  r.drawCenteredText(fonts.head, y, finished ? "Session complete" : "Ready to study");
  y += 72;

  std::snprintf(buf, sizeof(buf), "Due today: %u", static_cast<unsigned>(stats.dueReviews));
  r.drawText(fonts.body, layout::kMargin + 20, y, buf);
  y += 44;
  std::snprintf(buf, sizeof(buf), "New cards: %u", static_cast<unsigned>(stats.newCards));
  r.drawText(fonts.body, layout::kMargin + 20, y, buf);
  y += 44;
  std::snprintf(buf, sizeof(buf), "Reviewed today: %u", static_cast<unsigned>(stats.reviewedToday));
  r.drawText(fonts.body, layout::kMargin + 20, y, buf);
  y += 44;
  std::snprintf(buf, sizeof(buf), "Streak: %u day%s", static_cast<unsigned>(stats.streakDays),
                stats.streakDays == 1 ? "" : "s");
  r.drawText(fonts.body, layout::kMargin + 20, y, buf);

  r.drawLine(layout::kMargin, layout::kScreenH - layout::kFooterH, layout::kScreenW - layout::kMargin,
             layout::kScreenH - layout::kFooterH);
  const char* footer =
      finished ? "Confirm: study again    Back: decks" : (stats.remaining ? "Confirm: start    Back: decks" : "Back: decks");
  r.drawText(fonts.small, layout::kMargin, layout::kScreenH - layout::kFooterH + 18, footer);
}

void drawReviewChrome(GfxRenderer& r, const InkFonts& fonts, const std::string& deckName, const SessionStats& stats) {
  r.clearScreen();
  r.drawText(fonts.body, layout::kMargin, layout::kMargin, deckName.c_str());

  char buf[48];
  std::snprintf(buf, sizeof(buf), "%u left", static_cast<unsigned>(stats.remaining));
  int w = r.getTextWidth(fonts.small, buf, EpdStyle::REGULAR);
  r.drawText(fonts.small, layout::kScreenW - layout::kMargin - w, layout::kMargin + 4, buf);

  r.drawLine(layout::kMargin, layout::kHeaderH, layout::kScreenW - layout::kMargin, layout::kHeaderH);
  r.drawLine(layout::kMargin, layout::kScreenH - layout::kFooterH, layout::kScreenW - layout::kMargin,
             layout::kScreenH - layout::kFooterH);
}

void drawCardArea(GfxRenderer& r, const InkFonts& fonts, const DeckCard& card, bool showAnswer, bool deckHasPinyin) {
  // Clear only the card band so this can be pushed as a partial refresh.
  r.fillRect(0, layout::kCardTop, layout::kScreenW, layout::kCardHeight, /*black=*/false);

  const int contentX = layout::kMargin;
  const int contentW = layout::kScreenW - 2 * layout::kMargin;

  if (!showAnswer) {
    // Prompt only, vertically centred-ish in the band.
    int y = layout::kCardTop + layout::kCardHeight / 3;
    drawWrappedCentred(r, fonts.card, contentX, y, contentW, card.front);
    if (!card.hint.empty()) {
      drawWrappedCentred(r, fonts.small, contentX, layout::kCardBottom - 40, contentW, card.hint);
    }
    return;
  }

  // Answer view: prompt at the top, divider, then reading/answer/extras.
  int y = layout::kCardTop + 8;
  y = drawWrappedCentred(r, fonts.head, contentX, y, contentW, card.front);

  if (deckHasPinyin && card.has(FIELD_PINYIN) && !card.pinyin.empty()) {
    y = drawWrappedCentred(r, fonts.body, contentX, y + 4, contentW, card.pinyin);
  }

  y += 12;
  r.drawLine(contentX + 40, y, layout::kScreenW - contentX - 40, y);
  y += 20;

  y = drawWrappedCentred(r, fonts.card, contentX, y, contentW, card.back);

  if (card.has(FIELD_EXAMPLE) && !card.example.empty()) {
    y = drawWrappedCentred(r, fonts.body, contentX, y + 16, contentW, card.example);
  }
  if (card.has(FIELD_NOTES) && !card.notes.empty()) {
    drawWrappedCentred(r, fonts.small, contentX, y + 12, contentW, card.notes);
  }
}

void drawReviewFooter(GfxRenderer& r, const InkFonts& fonts, bool showAnswer) {
  // Clear the footer label strip (leave the divider line intact).
  const int stripTop = layout::kScreenH - layout::kFooterH + 4;
  r.fillRect(0, stripTop, layout::kScreenW, layout::kFooterH - 4, /*black=*/false);

  if (!showAnswer) {
    r.drawCenteredText(fonts.small, stripTop + 14, "Any button: show answer");
    return;
  }

  // Four evenly spaced grade labels matching the four front buttons.
  const char* labels[4] = {"Again", "Hard", "Good", "Easy"};
  const int cellW = layout::kScreenW / 4;
  for (int i = 0; i < 4; ++i) {
    int w = r.getTextWidth(fonts.small, labels[i], EpdStyle::REGULAR);
    int cx = i * cellW + (cellW - w) / 2;
    r.drawText(fonts.small, cx, stripTop + 14, labels[i]);
  }
}

}  // namespace inkcards
