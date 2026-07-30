# Spaced repetition (SM-2)

InkCards schedules reviews with the classic SM-2 algorithm, the same family of
algorithm that Anki's default scheduler descends from. The implementation is a
pure function in `firmware/lib/inkcards_core/Sm2.cpp` and is covered by host
unit tests.

## Grades

InkCards has four buttons, which map onto SM-2 quality values (0 to 5):

| Button | SM-2 quality | Notes                                        |
| ------ | ------------ | -------------------------------------------- |
| Again  | 1            | Failed recall. Forces a lapse.               |
| Hard   | 3            | Recalled with difficulty. Lowest passing grade. |
| Good   | 4            | Recalled after hesitation. Easiness unchanged. |
| Easy   | 5            | Perfect recall. Easiness rises.              |

## Per-card state

Each card carries the SM-2 state stored in the `.rev` file (see
[`FORMAT.md`](FORMAT.md)):

- **easiness factor** (EF), starting at 2.5, never below 1.3;
- **interval**, in days;
- **repetitions**, the count of consecutive passing grades;
- **lapses**, the count of failures;
- **due day**, the day number on which the card next becomes due.

## The update rule

On each review the easiness factor is updated first, on every grade including a
lapse:

```
EF' = EF + (0.1 - (5 - q) * (0.08 + (5 - q) * 0.02))
EF' = max(EF', 1.3)
```

So Good (q = 4) leaves EF unchanged, Easy (q = 5) adds 0.1, Hard (q = 3)
subtracts 0.14, and Again (q = 1) subtracts 0.54.

Then the interval:

- **Again** (a lapse): repetitions reset to 0, interval becomes 1 day, lapses
  increment.
- **Hard / Good / Easy** (passing):
  - first passing review: interval 1 day;
  - second: interval 6 days;
  - thereafter: interval becomes `round(previous_interval * EF')`;
  - repetitions increment.

The card's due day is set to today plus the new interval.

## Days, not timestamps

The scheduler works in whole local days (a "day number" is the count of days
since the Unix epoch in local time), not timestamps. This makes "due today" a
simple integer comparison and avoids intraday churn on a device whose clock may
only be roughly set. The conversion is in
`firmware/lib/inkcards_core/TimeUtil.h`.

## Sessions

A study session (see `Session.cpp`) builds today's queue by streaming once
through the deck:

1. **Due reviews**: cards already seen whose due day is today or earlier.
2. **New cards**: cards never seen, introduced up to a daily new-card budget
   (default 20), decremented by any new cards already introduced today.

Due reviews are queued ahead of new cards. Grading a card applies the SM-2
update, saves the state, and advances the queue. The first grade of a new
calendar day advances the study streak (or resets it to 1 if a day was missed)
and resets the per-day new-card counter.
