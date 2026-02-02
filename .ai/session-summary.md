## Session Summary

### Goal
- Implement ESB split dongle + nice!view display: battery on both halves, layer name on left only.
- Keep ESB topology, no BLE, minimal UI.
- Fix blank nice!view screens and get reliable runtime logging.

### Major Changes Implemented
- Added layer relay path:
  - `config/behavior_layer_display.c` (custom behavior)
  - `config/layer_display_state.c/.h` (shared state)
  - `config/layer_display_relay.c` (dongle listener to send layer to left)
- Custom screen work:
  - `config/custom_status_screen.c` renders layer + battery, uses LVGL refresh timer.
  - Added printk-based diagnostics and display unblanking checks.
- Module setup:
  - `zephyr/module.yml` now includes `cmake: config`, `kconfig: config/Kconfig`,
    and `settings.board_root: .`, `settings.dts_root: .`.
  - `config/CMakeLists.txt` now conditionally compiles `custom_status_screen.c`
    only when `CONFIG_ZMK_DISPLAY` **and** `CONFIG_LVGL` are enabled.
- Build and schema fixes:
  - Added DT binding for the new behavior in
    `dts/bindings/behaviors/zmk,behavior-layer-display.yaml`.
  - Added `config/Kconfig` (display options + relay flags).
  - Added behavior node to `config/corne.keymap`.
- Logging:
  - Enabled USB CDC logging in `corne_left.conf` and `corne_right.conf`.
  - Added `zmk-usb-logging` snippet in `build.yaml` for left/right builds.
  - `CONFIG_PRINTK=y` on both halves.

### Breakthroughs
- CI failures due to invalid `module.yml` keys fixed by adding `cmake: config`.
- Runtime logs confirmed keyboard boots, but no display init logs (screen path not executing).
- Compile/link failures addressed by:
  - Removing event listeners from `custom_status_screen.c`.
  - Fixing `SYS_INIT` signature.
  - Gating custom screen build with `CONFIG_ZMK_DISPLAY` and `CONFIG_LVGL`.
  - Enabling `CONFIG_LVGL=y` on left/right.

### Current State
- CI builds still failing earlier due to custom screen linking errors (see below).
- Screens still blank; logs show no `custom_status_screen:` printk lines yet.
- Expectation: with `cmake: config` + `dts_root` + conditional build + LVGL enabled,
  the custom screen code should now be linked and printk lines should appear.

### Key Files Changed
- `config/custom_status_screen.c`
- `config/CMakeLists.txt`
- `config/corne_left.conf`
- `config/corne_right.conf`
- `config/corne_dongle.conf`
- `config/corne.keymap`
- `config/Kconfig`
- `config/behavior_layer_display.c`
- `config/layer_display_state.c/.h`
- `config/layer_display_relay.c`
- `dts/bindings/behaviors/zmk,behavior-layer-display.yaml`
- `zephyr/module.yml`
- `build.yaml`

### Current Config Flags (important)
- Left/right:
  - `CONFIG_ZMK_DISPLAY=y`
  - `CONFIG_ZMK_DISPLAY_STATUS_SCREEN_CUSTOM=y`
  - `CONFIG_ZMK_DISPLAY_WORK_QUEUE_DEDICATED=y`
  - `CONFIG_ZMK_DISPLAY_SHOW_LAYER=y` (left), `n` (right)
  - `CONFIG_LVGL=y`
  - USB CDC logging enabled + `CONFIG_PRINTK=y`
  - nice!view widgets disabled (custom screen only)
- Dongle:
  - `CONFIG_ZMK_DISPLAY=n`
  - `CONFIG_ZMK_LAYER_DISPLAY_RELAY=y`

### Known Errors Resolved
- Undefined `ZMK_DISPLAY_SHOW_LAYER` (fixed by `config/Kconfig` + module.yml)
- `dts_root` schema error (fixed by proper module.yml structure)
- LVGL missing in xiao_ble build (gated custom screen + LVGL only on halves)
- SYS_INIT signature mismatch and event manager link errors
- Link errors in xiao_ble build (undefined LVGL/keymap symbols) addressed by
  gating `custom_status_screen.c` on `CONFIG_ZMK_DISPLAY && CONFIG_LVGL &&
  CONFIG_ZMK_DISPLAY_STATUS_SCREEN_CUSTOM` and guarding `zmk_keymap_layer_name`.
- Build failure from missing `<drivers/behavior.h>` fixed by using `<zmk/behavior.h>`.

### Next Steps
- Rebuild and flash left/right with latest changes.
- Capture boot logs and confirm `custom_status_screen:` printk lines appear.
  - If they appear, verify `display ready/unblanked` messages.
  - If they do **not** appear, verify that the flashed artifact matches the latest CI run.
- Re-run CI to confirm linker errors are gone in xiao_ble/settings_reset builds.
- If screen init logs appear but display still blank:
  - Check for chosen display node and device readiness in logs.
  - Consider temporarily switching to built-in status screen to isolate hardware.
- Once screen renders:
  - Remove debug logging and printk settings if not needed.
  - Validate layer relay: dongle sends to left peripheral.
