#!/usr/bin/env python3
"""Generate the device UI mockups shown in the README.

These are design mockups of the InkCards screens, drawn to match the layout
constants in firmware/src/ui/Theme.h and the draw routines in
firmware/src/ui/Screens.cpp. They are not photographs of hardware; see
docs/HARDWARE_TESTING.md for capturing real screenshots.

Run:

    python3 docs/images/screens/generate_screens.py

Writes <name>.svg for each screen here, and, if cairosvg is installed, renders a
matching <name>.png (what the README embeds).
"""

from __future__ import annotations

from pathlib import Path

HERE = Path(__file__).resolve().parent

# Screen is portrait 480x800 (Xteink X4 logical space). We wrap it in a simple
# device body with four front buttons to convey the physical layout.
SCREEN_W, SCREEN_H = 480, 800
PAD_X, PAD_TOP = 60, 64
BTN_ROW_H = 150
CANVAS_W = SCREEN_W + 2 * PAD_X
CANVAS_H = PAD_TOP + SCREEN_H + BTN_ROW_H

INK = "#1b1b1a"
PAPER = "#fcfbf7"
MUTE = "#6b6b66"
LINE = "#2a2a28"
BODY = "#242a2c"
ACCENT = "#2DB3A6"

SANS = "'DejaVu Sans','Segoe UI',Helvetica,Arial,sans-serif"
CJK = "'Noto Sans CJK SC','WenQuanYi Zen Hei',sans-serif"

# Layout constants mirroring Theme.h.
HEADER_H = 64
FOOTER_H = 56
CARD_TOP = HEADER_H + 16
CARD_BOTTOM = SCREEN_H - FOOTER_H - 16


def esc(s: str) -> str:
    return s.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")


class Screen:
    def __init__(self, caption: str, buttons: list[str] | None = None):
        self.body: list[str] = []
        self.caption = caption
        self.buttons = buttons or []

    def text(self, x, y, s, size, *, fill=INK, weight="400", anchor="start", family=SANS):
        self.body.append(
            f'<text x="{x}" y="{y}" font-family="{family}" font-size="{size}" '
            f'font-weight="{weight}" fill="{fill}" text-anchor="{anchor}">{esc(s)}</text>'
        )

    def line(self, x1, y1, x2, y2, *, stroke=LINE, w=2):
        self.body.append(f'<line x1="{x1}" y1="{y1}" x2="{x2}" y2="{y2}" stroke="{stroke}" stroke-width="{w}"/>')

    def rect(self, x, y, w, h, *, fill, rx=0):
        self.body.append(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="{rx}" fill="{fill}"/>')

    def render(self) -> str:
        # Screen contents are drawn in screen-local coords then translated.
        inner = "\n    ".join(self.body)
        # Four front buttons centred under the screen.
        btns = []
        n = 4
        bx0 = PAD_X
        cell = SCREEN_W / n
        by = PAD_TOP + SCREEN_H + 54
        for i in range(n):
            cx = bx0 + cell * i + cell / 2
            btns.append(f'<rect x="{cx-46:.0f}" y="{by}" width="92" height="40" rx="20" fill="#3a4245"/>')
            if i < len(self.buttons) and self.buttons[i]:
                btns.append(
                    f'<text x="{cx:.0f}" y="{by+26}" font-family="{SANS}" font-size="18" '
                    f'fill="#dfe7e8" text-anchor="middle">{esc(self.buttons[i])}</text>'
                )
        btns_svg = "\n  ".join(btns)

        return f"""<svg xmlns="http://www.w3.org/2000/svg" width="{CANVAS_W}" height="{CANVAS_H}" viewBox="0 0 {CANVAS_W} {CANVAS_H}" role="img" aria-label="{esc(self.caption)}">
  <rect x="8" y="8" width="{CANVAS_W-16}" height="{CANVAS_H-16}" rx="46" fill="{BODY}"/>
  <rect x="{PAD_X-6}" y="{PAD_TOP-6}" width="{SCREEN_W+12}" height="{SCREEN_H+12}" rx="10" fill="#0d0f10"/>
  <rect x="{PAD_X}" y="{PAD_TOP}" width="{SCREEN_W}" height="{SCREEN_H}" fill="{PAPER}"/>
  <g transform="translate({PAD_X} {PAD_TOP})">
    {inner}
  </g>
  {btns_svg}
</svg>
"""


def deck_list() -> Screen:
    s = Screen("InkCards deck list", ["Up", "Down", "", "Open"])
    s.text(24, 44, "InkCards", 34, weight="800")
    s.line(24, HEADER_H, SCREEN_W - 24, HEADER_H)
    rows = ["HSK 1 Vocabulary", "Capitals of the World", "InkCards Demo"]
    y = 96
    row_h = 58
    for i, name in enumerate(rows):
        ry = y + i * row_h
        if i == 0:
            s.rect(16, ry - 20, SCREEN_W - 32, row_h - 8, fill=INK, rx=10)
            s.text(36, ry + 16, name, 26, fill=PAPER, weight="600")
        else:
            s.text(36, ry + 16, name, 26)
    s.line(24, SCREEN_H - FOOTER_H, SCREEN_W - 24, SCREEN_H - FOOTER_H)
    s.text(24, SCREEN_H - FOOTER_H + 34, "Up / Down: move      Confirm: open", 18, fill=MUTE)
    return s


def review_front() -> Screen:
    s = Screen("Reviewing a card, prompt", ["", "", "", ""])
    s.text(24, 40, "HSK 1 Vocabulary", 24, weight="600")
    s.text(SCREEN_W - 24, 40, "12 left", 18, fill=MUTE, anchor="end")
    s.line(24, HEADER_H, SCREEN_W - 24, HEADER_H)
    # Prompt centred in the card band.
    cy = CARD_TOP + (CARD_BOTTOM - CARD_TOP) // 3
    s.text(SCREEN_W / 2, cy, "你好", 118, anchor="middle", family=CJK, weight="700")
    s.line(24, SCREEN_H - FOOTER_H, SCREEN_W - 24, SCREEN_H - FOOTER_H)
    s.text(SCREEN_W / 2, SCREEN_H - FOOTER_H + 34, "Press any button to show the answer", 18, fill=MUTE, anchor="middle")
    return s


def review_back() -> Screen:
    s = Screen("Reviewing a card, answer", ["Again", "Hard", "Good", "Easy"])
    s.text(24, 40, "HSK 1 Vocabulary", 24, weight="600")
    s.text(SCREEN_W - 24, 40, "12 left", 18, fill=MUTE, anchor="end")
    s.line(24, HEADER_H, SCREEN_W - 24, HEADER_H)
    s.text(SCREEN_W / 2, CARD_TOP + 70, "你好", 60, anchor="middle", family=CJK, weight="700")
    s.text(SCREEN_W / 2, CARD_TOP + 118, "nǐ hǎo", 30, anchor="middle", fill=MUTE)
    s.line(120, CARD_TOP + 150, SCREEN_W - 120, CARD_TOP + 150, stroke=MUTE, w=1)
    s.text(SCREEN_W / 2, CARD_TOP + 250, "hello", 64, anchor="middle", weight="600")
    s.text(SCREEN_W / 2, CARD_TOP + 320, "a greeting", 22, anchor="middle", fill=MUTE)
    s.line(24, SCREEN_H - FOOTER_H, SCREEN_W - 24, SCREEN_H - FOOTER_H)
    # Grade labels aligned above the four buttons.
    labels = ["Again", "Hard", "Good", "Easy"]
    for i, lab in enumerate(labels):
        cx = SCREEN_W / 4 * i + SCREEN_W / 8
        s.text(cx, SCREEN_H - FOOTER_H + 34, lab, 20, anchor="middle", weight="600")
    return s


def session_summary() -> Screen:
    s = Screen("Session summary", ["", "", "", "Start"])
    s.text(24, 44, "HSK 1 Vocabulary", 28, weight="700")
    s.line(24, HEADER_H, SCREEN_W - 24, HEADER_H)
    s.text(SCREEN_W / 2, 170, "Ready to study", 36, anchor="middle", weight="700")
    stats = [("Due today", "12"), ("New cards", "8"), ("Reviewed today", "0"), ("Streak", "5 days")]
    y = 270
    for i, (k, v) in enumerate(stats):
        ry = y + i * 62
        s.text(60, ry, k, 26, fill=MUTE)
        s.text(SCREEN_W - 60, ry, v, 26, weight="700", anchor="end")
    s.line(24, SCREEN_H - FOOTER_H, SCREEN_W - 24, SCREEN_H - FOOTER_H)
    s.text(24, SCREEN_H - FOOTER_H + 34, "Confirm: start      Back: decks", 18, fill=MUTE)
    return s


SCREENS = {
    "deck-list": deck_list,
    "review-front": review_front,
    "review-back": review_back,
    "session-summary": session_summary,
}


def main() -> int:
    try:
        import cairosvg  # noqa: F401

        have_cairo = True
    except ImportError:
        have_cairo = False

    for name, fn in SCREENS.items():
        svg = fn().render()
        svg_path = HERE / f"{name}.svg"
        svg_path.write_text(svg, encoding="utf-8")
        print(f"wrote {svg_path.name}")
        if have_cairo:
            import cairosvg

            cairosvg.svg2png(url=str(svg_path), write_to=str(HERE / f"{name}.png"), output_width=SCREEN_W + 2 * PAD_X)
            print(f"rendered {name}.png")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
