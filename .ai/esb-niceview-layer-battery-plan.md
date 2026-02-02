# ZMK ESB dongle + nice!view: layer (left only) + battery (both halves)

Baseline: this plan targets the **currently working user-config commit** `32b0bc3` (`32b0bc34207203aefe1b56db389496e8ea17596c`) and keeps your existing board/shield naming (e.g. `xiao_ble`, `corne_dongle`, `corne_left`, `corne_right`).

## Goals

- **Keep the current working ESB split dongle setup** (dongle = central over USB; left/right = ESB peripherals).
- Show **battery percentage** on **both** nice!view screens (left + right).
- Show **current layer name** on **left** nice!view screen only.
- Preserve “stupid simple” UI: readable text, minimal refresh complexity.

## Non-goals (this step)

- No UI polish (icons/animations/layout tuning/refresh rates).
- No transport re-architecture and no renaming boards/shields.
- No code changes in this step: this document is the only output.

## Current repo state (what you already have)

### Build targets

From `build.yaml`, you build:

- `xiao_ble` + `corne_dongle` (central/dongle)
- `nice_nano` + `corne_left nice_view_adapter nice_view` (left peripheral with nice!view)
- `nice_nano` + `corne_right nice_view_adapter nice_view` (right peripheral with nice!view)
- `settings_reset` for both `xiao_ble` and `nice_nano`

### West manifest and ESB module

From `config/west.yml`, your workspace uses:

- ZMK: `zmkfirmware/zmk` at `revision: main`
- ESB split transport module: `badjeff/zmk-feature-split-esb` at `revision: main`
- NCS forks/pins: `sdk-nrf` from `badjeff` + `nrfxlib` from `nrfconnect`

The ESB address block is centralized in `config/esb_addr.dtsi` and included by:

- `boards/shields/corne_dongle/corne_dongle.overlay`
- `config/corne_left.overlay`
- `config/corne_right.overlay`

### Central/peripheral configs (high signal)

- `config/corne.conf` disables BLE (`CONFIG_BT=n`, `CONFIG_ZMK_BLE=n`) and sets ESB tuning parameters.
- `config/corne_dongle.conf` sets **central** split role and disables display (`CONFIG_ZMK_DISPLAY=n`).
- `config/corne_left.conf` and `config/corne_right.conf` set **peripheral** role, enable display, and enable custom status screen.

### Custom display code is already compiled

You already have a custom LVGL status screen implementation in `config/custom_status_screen.c`, and it’s included in the build via:

- `config/CMakeLists.txt` → `zephyr_library_sources(custom_status_screen.c)`

So the “plumbing” for a custom screen is already in place.

## The key technical constraint (and why it matters)

ZMK’s split architecture explicitly states:

- The **central** handles internal keyboard state like **active layers**.
- **Peripherals do not own that state** and primarily forward input events to the central.

Reference: ZMK split keyboards docs, “Central and Peripheral Roles” (`https://zmk.dev/docs/features/split-keyboards`).

### Implication for your goal

- **Battery % on each half** is straightforward: each half measures and renders its own battery locally.
- **Layer name on the left half** is *not guaranteed to work* just by reading `zmk_keymap_highest_layer_active()` on the peripheral, because layer state is central-owned (dongle).

So, a working solution needs **some form of state propagation from dongle → left half**.

## Research notes (GitHub/Reddit) relevant to this setup

### ESB transport topology and constraints (authoritative)

- `badjeff/zmk-feature-split-esb` README (`https://github.com/badjeff/zmk-feature-split-esb`)
  - Describes the USB-only dongle + ESB-only topology (your case).
  - Notes the ESB timing/ack/retransmit knobs you already set.

### Display configuration (authoritative)

- ZMK display config reference (`https://zmk.dev/docs/config/displays`)
  - If `CONFIG_ZMK_DISPLAY=y`, you must use **exactly one** of:
    - built-in status screen, or
    - custom status screen
  - Documents widgets like layer/battery and dedicated display work queue options.

### Known display pitfalls on split peripherals (relevant risk)

- Issue: peripheral can become unresponsive if display is enabled on a side with no display hardware (`https://github.com/zmkfirmware/zmk/issues/679`)
  - Not directly your situation (both halves have nice!view), but it’s a reminder to keep display enablement consistent with actual hardware.

### Layer widget regressions / limitations (risk signal)

- Issue report: layer widget stopped updating in some setups (`https://github.com/zmkfirmware/zmk/issues/2467`)
  - This reinforces the need to validate layer display behavior carefully, especially in split configurations.

### Community “nice!view minimal screens” (inspiration, but watch dependencies)

- `infely/nice-view-battery` (`https://github.com/infely/nice-view-battery`)
  - Minimalist battery-focused custom nice!view screen/shield.
  - Good reference for “no frills” screen philosophy.

- `mctechnology17/zmk-nice-oled` (`https://github.com/mctechnology17/zmk-nice-oled`)
  - Very feature-rich; includes “peripheral screen” concepts and many Kconfig toggles.
  - Explicitly tested on ZMK v0.3.0 (good reference for a stable pinned baseline).
  - Likely more than you want right now, but useful for layout patterns and performance/latency cautions.

- `haerttrich/zmk-nice-custom` (`https://github.com/haerttrich/zmk-nice-custom`)
  - Shows a pattern of handling different data on left vs right displays and mentions “relay” patterns for split.
  - Some relay approaches in the ecosystem are BLE/GATT-based and won’t apply directly to ESB-only.

### Similar dongle-central (XIAO BLE) configs

- `jonathanforking/corax54-zmk-dongle` (`https://github.com/jonathanforking/corax54-zmk-dongle`)
  - Confirms the general “XIAO BLE dongle central + nice!nano peripherals” pattern exists in the wild.

### Reddit

- I attempted multiple `site:reddit.com` searches with 2026-scoped queries for “ZMK ESB split dongle + nice!view + layer display”, but did not get usable indexed results via automated search.
- If you want, a follow-up step can do **manual** Reddit discovery (keyword + subreddit browsing) once we start implementing and want troubleshooting anecdotes.

## Recommended implementation approach (minimal change, ESB-friendly)

### Summary

1. **Keep** your ESB split topology and all current board/shield names.
2. **Keep** using a custom LVGL status screen (you already have it compiling).
3. **Battery display** stays local on each half.
4. **Layer name display on left** is implemented by **propagating the layer index from dongle → left** using existing split infrastructure, without adding new transport packet types.

### Why propagation is needed

Per ZMK docs, layer state is central-owned. With a dongle as central and **no display on the dongle**, the only way to reliably show layer name on a peripheral is to **send that state to the peripheral**.

### The key trick: piggyback on “invoke behavior” split commands

ZMK split central already supports sending commands to a specific peripheral source using:

- `zmk_split_central_invoke_behavior(source, binding, event, state)`

And peripherals already handle the “invoke behavior” command type by calling `behavior_keymap_binding_pressed/released` on-device (see ZMK `app/src/split/peripheral.c` and `app/src/split/central.c` in `zmkfirmware/zmk` main).

That means you can avoid inventing a new “set layer state” transport message by:

- Creating a **tiny custom behavior** that “sets the displayed layer index” on the peripheral.
- Having the dongle (central) **invoke that behavior** on the left peripheral whenever the layer changes.

This should work across BLE split, wired split, and ESB split **as long as the transport implements `send_command`**, which ESB does.

## Step-by-step implementation plan (phased)

### Phase 0 — Verify the baseline behavior (no code changes)

Do these checks on the current `32b0bc3` build:

- **Battery**: confirm `BAT: xx%` updates on each half when battery changes.
- **Layer**: switch layers and see whether the displayed layer on the halves changes at all.
  - If it already changes correctly on left today, we may only need “left only” UI changes.
  - If it does **not** change (likely), proceed to Phase 2.

### Phase 1 — Make the display content per-side (left vs right)

Target behavior:

- Left: show “LayerName” + “BAT: xx%”
- Right: show “BAT: xx%” only

Implementation strategy (later code step):

- Add a Kconfig flag per half (e.g. in `corne_left.conf` set `CONFIG_SHOW_LAYER_ON_SCREEN=y`, in `corne_right.conf` set it to `n`) and compile the custom screen with that conditional.
  - This avoids runtime side detection and keeps behavior deterministic.

Why not disable display on right?

- Because you *do* have a nice!view on the right and want battery displayed there. (Also, `zmk/issues/679` is a reminder to not enable display on a side without hardware.)

### Phase 2 — Propagate layer changes from dongle → left half

Implementation strategy (later code step):

- **On the dongle build**:
  - Add a ZMK event listener for `zmk_layer_state_changed`.
  - When fired, compute `layer = zmk_keymap_highest_layer_active()`.
  - Invoke a custom behavior on **source 0** (your left peripheral) carrying `layer` in `param1`.

- **On the left peripheral**:
  - Implement a custom behavior “set_display_layer(layer_index)”.
  - Store the received layer index in a small module-global variable (or a dedicated “display state” struct).

- **In `custom_status_screen.c` on the left**:
  - Render layer name from the **propagated layer index** (via `zmk_keymap_layer_name(layer)`), not from local layer state.

Why this is the lowest-risk path:

- You avoid extending ZMK’s split protocol (no new enums/packet types).
- You reuse split’s existing command path that ESB already implements.
- You keep BLE disabled everywhere in your ESB-only topology.

### Phase 3 — Tighten compatibility and reduce surprises

Your CI workflow uses `zmkfirmware/zmk/.github/workflows/build-user-config.yml@v0.3` but your `config/west.yml` pins ZMK to `main`.

This can work (it did for `32b0bc3`), but for reproducibility:

- Decide on one baseline:
  - either pin ZMK to a known-good commit on `main`, or
  - pin to `v0.3.x` (matching the workflow), **if** ESB module compatibility holds.

Recommendation: once the feature is complete, pin ZMK + ESB module revisions to SHAs to prevent “random main broke it” incidents.

## Compatibility guardrails (avoid errors with your current ZMK)

### Keep ESB-only constraints intact

- Do not re-enable BLE on peripherals (`CONFIG_BT=n`, `CONFIG_ZMK_BLE=n`) unless you intentionally switch topologies.
- Avoid display widgets/modules that assume BLE output state unless confirmed safe in ESB-only builds.

### Keep display CPU impact controlled

You already use:

- `CONFIG_ZMK_DISPLAY_WORK_QUEUE_DEDICATED=y` on peripherals

That’s aligned with ZMK display docs recommendation for slower displays (e-paper / nice!view) to avoid input lag.

### Keep layer naming stable

You already set per-layer `display-name` values in `config/corne.keymap` (e.g. “Base”, “Colemak”, “Num”…).

That is exactly what you want to show on screen (instead of “Layer 2”).

## Verification checklist (after implementation)

### Build-time (CI/local)

- All 3 targets still build:
  - `xiao_ble` + `corne_dongle`
  - `nice_nano` + `corne_left + nice_view_adapter + nice_view`
  - `nice_nano` + `corne_right + nice_view_adapter + nice_view`
- No new Kconfig warnings about display/status screen selection.
- No link errors from missing behavior devices (common when a behavior is referenced but not present on a given side).

### Runtime (functional)

- Left half:
  - shows correct layer name when switching layers from either half (including tri-layer triggers).
  - battery percentage updates.
- Right half:
  - shows battery percentage updates.
  - does **not** show layer text.
- Dongle:
  - remains display-free and stable over USB.

### Latency sanity

- Confirm perceived latency is unchanged (or improved) vs the baseline.
- If you see input lag on heavy screen updates, keep the dedicated display thread priority conservative (per ZMK display docs) and avoid animations in this phase.

## Proposed “code changes” inventory (for the next step)

No code is created in this step; this is the plan for the next step.

- **New** small behavior driver module (files under `config/`):
  - A behavior that stores “current layer index to render”.
- **New** dongle-only listener module:
  - Subscribes to `zmk_layer_state_changed`
  - Invokes the behavior on left peripheral (`source 0`) with `param1 = layer`.
- **Update** `config/custom_status_screen.c`:
  - Left: render layer from propagated state + local battery.
  - Right: render battery only.
- **Update** `config/corne_left.conf` / `config/corne_right.conf`:
  - Add a per-side flag controlling whether layer is drawn (left on, right off).

