// Logical button layer for InkCards, mapping the X4/X3 physical front buttons
// onto the actions the review flow needs. The raw HalGPIO sampling and edge
// queries live in inkkit (inkkit/Buttons.h); this header keeps only the
// InkCards-specific logical map so the firmware does not depend on CrossPoint
// app code.
//
// The X4/X3 expose four front buttons plus a back/menu and power control. During
// review the four front buttons double as the four grades, which is why the
// grade order (Again, Hard, Good, Easy) is laid out left to right to match the
// physical button order. See docs/BUTTON_MAPPING.md.
#pragma once

#include <inkkit/Buttons.h>

#include <cstdint>

namespace inkcards {

enum class Btn : uint8_t { Front1 = 0, Front2, Front3, Front4, Back, Confirm, Count };

// Physical HalGPIO button indices for each logical button.
//
// TODO(hardware-test): verify these indices on a real X4 and X3. They are the
// best guess from the CrossPoint front-button layout (btn1..btn4 left to right)
// and must be confirmed on device; see docs/HARDWARE_TESTING.md.
struct InkButtonMap {
  uint8_t front1 = 0;
  uint8_t front2 = 1;
  uint8_t front3 = 2;
  uint8_t front4 = 3;
  uint8_t back = 4;
  uint8_t confirm = 5;
};

class InkInput {
 public:
  InkInput(HalGPIO& gpio, const InkButtonMap& map = InkButtonMap{}) : buttons_(gpio), map_(map) {}

  void update() const { buttons_.update(); }

  bool wasPressed(Btn b) const { return buttons_.wasPressed(indexOf(b)); }
  bool isPressed(Btn b) const { return buttons_.isPressed(indexOf(b)); }

  // True if any of the four front buttons or Confirm was pressed this frame.
  // Used to reveal the answer, where the specific button does not matter.
  bool wasAnyActionPressed() const {
    return wasPressed(Btn::Front1) || wasPressed(Btn::Front2) || wasPressed(Btn::Front3) ||
           wasPressed(Btn::Front4) || wasPressed(Btn::Confirm);
  }

 private:
  inkkit::Buttons buttons_;
  InkButtonMap map_;

  uint8_t indexOf(Btn b) const {
    switch (b) {
      case Btn::Front1: return map_.front1;
      case Btn::Front2: return map_.front2;
      case Btn::Front3: return map_.front3;
      case Btn::Front4: return map_.front4;
      case Btn::Back: return map_.back;
      case Btn::Confirm: return map_.confirm;
      default: return 0;
    }
  }
};

}  // namespace inkcards
