# Pomodoro screen for the Prospector dongle — feasibility assessment

Date: 2026-08-22
Status: assessment only, no code written
Target: `olafkrawczyk/prospector-zmk-module` branch `feat/touch-swipe-screens`

## Verdict

Straightforward build, no architectural blockers. Everything a Pomodoro screen
needs already has an established pattern in the module (custom behavior,
custom event, layout slot, backlight PWM control). Roughly 400 new lines, one
focused PR.

## Requirements

- Timer: 25 min work / 5 min break
- Start/stop controlled by keymap buttons on a layer (e.g. NAV layer)
- Flashing screen when the timer ends

## How each piece maps to existing code

### 1. The screen itself

New `src/layouts/pomodoro/` exposing `zmk_prospector_screen_pomodoro_create()`.
Wiring is mechanical, following the pattern introduced by the touch/swipe work:

- one `PROSPECTOR_SCREEN_POMODORO_ENABLED` Kconfig bool
  (`depends on PROSPECTOR_TOUCHSCREEN` if it should be swipe-only)
- one entry in the CMake `foreach` layout list
- one entry in the screen manager `screens[]` table

The layout is one of the simplest in the module: a big countdown label +
state label (WORK / BREAK / IDLE). Becomes just another swipe destination.

### 2. Timer state machine

The one genuinely new component: a small `pomodoro.c` service
(idle → work 25:00 → flash → break 5:00 → flash → idle) driven by a Zephyr
`k_work_delayable` at 1 Hz (~150 lines).

Key design point: the timer state lives **decoupled from the screen** — the
screen only renders current state — so the countdown keeps running while
other screens (Classic/Operator) are shown. The screen subscribes to a new
`pomodoro_state_changed` event; the module already ships two custom events
(`caps_word_state_changed`, `split_central_status_changed`), so this follows
a proven pattern.

### 3. Start/stop via layer buttons

The cleanest part, thanks to two facts:

- ZMK split architecture: behaviors always run on the **central** (the
  dongle). Peripherals forward key positions; the dongle executes the keymap.
  No split-communication work needed.
- The module already ships a custom behavior (`behavior_caps_word.c`).

A `&pomodoro_toggle` (plus optionally `&pomodoro_reset`) is a copy of that
pattern: a dts binding + ~40-line C file. Bound in the keymap on the NAV
layer like any other behavior — a pure zmk-config keymap change.

### 4. Flashing at the end

Two options, both trivial (~15–20 lines each):

- **Backlight blink**: toggle the display PWM via the same
  `led_set_brightness` path the brightness gestures use. Works even when
  another screen is shown; catches the eye better than a visual-only flash.
- **Auto-switch to the Pomodoro screen + visual flash**: the screen manager
  owns `lv_screen_load_anim`, so timer-end can pull up the Pomodoro screen
  and flash an overlay.

Recommendation: do both (switch + backlight blink).

## Points to decide

- **Auto-advance vs manual**: classic Pomodoro auto-starts the break when
  work ends. Recommendation: auto-advance work → break, stop after break
  (avoids infinite looping when walking away).
- **Display sleep**: the timer keeps counting while the display is blanked
  on idle (a `k_timer`/`k_work_delayable` is independent of the display
  loop); the screen shows the correct remaining time on wake.
- **Persistence**: state resets on dongle reboot. Fine for a Pomodoro timer —
  not worth settings/NVS.
- **Tap-to-start on the screen**: optional extra. The touch rework made all
  widgets inert, but the screen root is still the gesture hit-target, so a
  tap gesture on the Pomodoro screen could also toggle start/stop. Not
  required — keymap buttons are the primary control.

## Risks

| Risk | Assessment |
| ---- | ---------- |
| RAM | One more LVGL screen tree ≈ a few KB of the 32K heap (already bumped for touch). No risk. |
| Split transport | None needed — behaviors run on the central. |
| LVGL/ZMK internals | None touched — all patterns already exist in the module. |
| Timer accuracy | 1 Hz `k_work_delayable` — second-level accuracy, more than enough. |

## Effort estimate

| Component | Size | Risk |
| --------- | ---- | ---- |
| `pomodoro` layout + wiring | ~120 lines | none |
| Timer service + custom event | ~180 lines | low (patterns exist) |
| `&pomodoro_toggle` behavior + dts binding | ~60 lines | none (copy of caps_word pattern) |
| End-of-timer flash (backlight + auto-switch) | ~20 lines | none |
| Keymap binding on NAV layer | 2 keys | trivial |

**Total: roughly 400 new lines, one focused PR.**

## Suggested build order

1. Timer service + custom event (headless, loggable)
2. Behavior + keymap binding (start/stop working, verified via logs)
3. Pomodoro layout screen (renders state)
4. Flash + auto-switch on timer end
