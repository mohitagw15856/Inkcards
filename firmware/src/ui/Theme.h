// Layout constants and font handles for the InkCards UI. Coordinates are in the
// renderer's portrait logical space (480 x 800 on the X4). The card area is a
// fixed band so it can be partially refreshed on its own between cards.
#pragma once

#include <cstdint>

namespace inkcards {

// Font ids registered with the GfxRenderer at boot. These are provisioned in
// main.cpp; the UI only ever refers to them through this struct so the concrete
// font assets can change without touching screen code.
struct InkFonts {
  int small = 0;  // captions, footer button labels
  int body = 0;   // list rows, session stats
  int head = 0;   // headings
  int card = 0;   // the card's front/back text (largest)
};

namespace layout {

// Logical portrait dimensions (X4). The X3 is smaller; the UI uses the
// renderer's reported width/height at run time and treats these as defaults.
constexpr int kScreenW = 480;
constexpr int kScreenH = 800;

constexpr int kMargin = 24;
constexpr int kHeaderH = 64;
constexpr int kFooterH = 56;

// The card content band, redrawn on its own between cards via partial refresh.
constexpr int kCardTop = kHeaderH + 16;
constexpr int kCardBottom = kScreenH - kFooterH - 16;
constexpr int kCardHeight = kCardBottom - kCardTop;

}  // namespace layout

}  // namespace inkcards
