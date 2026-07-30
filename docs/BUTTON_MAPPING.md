# Button mapping

The Xteink X4/X3 has four front buttons plus a back/menu control and a power
control, and no touchscreen. InkCards uses the four front buttons as the four
review grades during a review, which is why the grade order (Again, Hard, Good,
Easy) is laid out left to right to match the physical button order.

The logical buttons are defined in
`firmware/src/platform/InkInput.h` as `Front1..Front4`, `Back` and `Confirm`.

> The mapping from logical buttons to physical GPIO indices is marked
> `TODO(hardware-test)` in `InkInput.h`. The indices below are the intended
> layout and must be confirmed on a real device; see
> [`HARDWARE_TESTING.md`](HARDWARE_TESTING.md).

## Deck list

| Button   | Action              |
| -------- | ------------------- |
| Front 1  | Move selection up   |
| Front 2  | Move selection down |
| Confirm  | Open the deck       |

## Session summary

| Button   | Action                          |
| -------- | ------------------------------- |
| Confirm  | Start (or resume) the review    |
| Back     | Return to the deck list         |

## Reviewing a card

While the answer is hidden:

| Button                       | Action           |
| ---------------------------- | ---------------- |
| Any front button, or Confirm | Show the answer  |
| Back                         | Leave the review |

Once the answer is shown, the four front buttons are the four grades:

| Button   | Grade | Meaning                                    |
| -------- | ----- | ------------------------------------------ |
| Front 1  | Again | You did not recall it. The card lapses.    |
| Front 2  | Hard  | Recalled with serious difficulty.          |
| Front 3  | Good  | Recalled after some hesitation.            |
| Front 4  | Easy  | Recalled perfectly.                        |

The grade labels are drawn along the bottom of the screen above their buttons,
so the mapping is always visible.

## Session complete

| Button   | Action                              |
| -------- | ----------------------------------- |
| Confirm  | Study again (picks up lapsed cards) |
| Back     | Return to the deck list             |

How the grades affect scheduling is described in
[`SPACED_REPETITION.md`](SPACED_REPETITION.md).
