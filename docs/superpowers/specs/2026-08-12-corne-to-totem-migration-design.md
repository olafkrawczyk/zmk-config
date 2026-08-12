# Corne → TOTEM Migration Design

Date: 2026-08-12
Status: Approved (in-chat)

## Goal

Convert this repo from a Corne (42-key, nice_nano_v2, nice!view) ZMK config into a
TOTEM (38-key, Seeed XIAO BLE, **no display**) ZMK config, **in place** (same git
history and GitHub remote/Actions).

Only the **layout idea** comes from the Corne repo (layers, combos, macros,
hold-tap tuning). Everything else (shield definition, build.yaml, west.yml,
base conf) comes from `GEIGEIGEIST/zmk-config-totem` (branch `master`).

## Hardware target

- Seeed XIAO BLE (`seeeduino_xiao_ble`), no display, no RGB.
- TOTEM shield ships inside the config repo at `config/boards/shields/totem/`.
- west.yml tracks ZMK `main` (as the TOTEM repo does).

## TOTEM key positions (from shield transform)

```
Row0:      0  1  2  3  4 │  5  6  7  8  9
Row1:     10 11 12 13 14 │ 15 16 17 18 19
Row2: 20 21 22 23 24 25  │ 26 27 28 29 30 31   (20/31 = extra outer bottom keys)
Thumbs:      32 33 34    │ 35 36 37             (32/37 outer, 34/35 inner)
```

Binding order: row0 L+R, row1 L+R, row2 L+R (6 each), thumbs L, thumbs R = 38 keys.

Left half:  0-4, 10-14, 20-25, 32-34
Right half: 5-9, 15-19, 26-31, 35-37

## Core mapping decisions

- Corne's outer pinky column (DEL/CTRL/SHFT left, BSPC/'/RSHFT-ESC right) is gone.
- **DEL → key 20**, **BSPC → key 31** (extra bottom-outer keys, same "outer pinky" job).
- Thumb cluster **unchanged** from Corne: `MSE/ESC · NUM · ENT │ SPC · SYM · TAB`.
- `'` (SQT) lives on the SYM layer only.
- Home-row mods unchanged in behavior; only trigger positions renumbered.
- Layer indices unchanged: BASE 0, COL 1, NUM 2, SYM 3, MSE 4, NAV 5.
- Tri-layer (NUM+SYM → NAV) unchanged.

## Layers (final grids)

### BASE
```
         Q    W    E    R    T   │   Y    U    I    O    P
         A    S    D    F    G   │   H    J    K    L    ;
  DEL    Z    X    C    V    B   │   N    M    ,    .    /   BSPC
           MSE/ESC NUM  ENT      │  SPC  SYM   TAB
```
Row1 uses `mode_tap_left`/`mode_tap_right` exactly as the Corne keymap:
A=SFT S=CTL D=GUI F=ALT │ J=ALT K=GUI L=CTL ;=SFT (right-hand variants).

### COL (Colemak)
```
         Q    W    F    P    B   │   J    L    U    Y    '
 trans   Z    X    C    D    V   │   K    H    ,    .    /   trans
           trans  NUM  trans     │ trans  SYM   trans
```
Row1 mods mirror BASE (A=SFT R=CTL S=GUI T=ALT │ N=ALT E=GUI I=CTL O=SFT).

### NUM
```
         1    2    3    4    5   │   6    7    8    9    0
        BT1  BT2  BT3  BT4  BT5  │ LEFT  DOWN   UP  RIGHT trans
 GRAVE BRI- BRI+ VOL- VOL+ MUTE  │ trans trans trans trans TOG_COL trans
           trans trans trans     │ LCTRL trans  trans
```

### SYM
```
         !    @    #    $    %   │   {    }    (    )    `
         ^    &    *    _    +   │   [    ]    :    "    ;
 LSHFT   ~    |    \    -    =   │   <    >    ?    '    .    ESC
           trans NUM  trans      │ trans trans  trans
```

### MSE
```
       BT_CLR trans  F2  trans trans │ trans trans trans trans OUT_TOG
       trans  MCLK trans LCLK  RCLK  │ M_LT  M_DN  M_UP  M_RT  trans
 trans trans  trans SCRL+ SCRL- trans│ trans trans trans trans trans trans
           trans trans trans         │ trans trans  trans
```

### NAV (tri-layer: NUM+SYM)
```
         F1   F2   F3   F4   F5  │  F6   F7   F8   F9   F10
        trans LA(SPC) LG(K) trans trans│ LEFT DOWN UP RIGHT LG(P)
  F11  snap_l snap_c snap_m snap_r trans│ HOME tab<- tab-> END LC(GRAVE) F12
           trans trans trans     │ trans trans  trans
```
(`tab<-`/`tab->` = `LG(LS(LBKT))` / `LG(LS(RBKT))`; snap macros unchanged.)

## Combos (renumbered, same fingers)

| Name        | Positions  | Keys      | Output        |
|-------------|-----------|-----------|---------------|
| esc         | 16 17     | J+K       | ESC           |
| semi        | 17 18     | K+L       | SEMI          |
| arrow       | 12 13     | D+F       | `=>` macro    |
| interp      | 11 12     | S+D       | `${}` macro   |
| strict      | 13 14     | F+G       | `!==` macro   |
| screenshot  | 7 8       | I+O       | screenshot    |
| sys_reset   | 9 20 31   | P+DEL+BSPC| sys_reset     |
| bootloader  | 10 20 31  | A+DEL+BSPC| bootloader    |

timeout-ms = 50 for all (as Corne).

## Behaviors

- `mode_tap_left`: tapping-term 175, tap-preferred, require-prior-idle 125,
  quick-tap 0, hold-trigger-key-positions = `<5 6 7 8 9 15 16 17 18 19 26 27 28 29 30 31 35 36 37>`
- `mode_tap_right`: same params, hold-trigger-key-positions =
  `<0 1 2 3 4 10 11 12 13 14 20 21 22 23 24 25 32 33 34>`
- Pointing tuning kept: `&mmv time-to-max-speed-ms = <450>`, `&msc acceleration-exponent = <1>`.
- Dropped as unused: `enable_mouse` behavior, `ZMK_POINTING_DEFAULT_MOVE_VAL` define.

## Macros (ported unchanged)

m_arrow, m_interp, m_strict, m_snap_l, m_snap_r, m_snap_m, m_snap_c, m_screenshot.

## config/totem.conf

Base from TOTEM repo (`CONFIG_ZMK_USB_LOGGING=n`), plus only what the layout
functionally needs — **no display, no nice!view, no RGB**:

```
CONFIG_ZMK_MOUSE=y
CONFIG_ZMK_COMBO_MAX_COMBOS_PER_KEY=6
CONFIG_ZMK_COMBO_MAX_KEYS_PER_COMBO=3

CONFIG_ZMK_SLEEP=y
CONFIG_ZMK_IDLE_SLEEP_TIMEOUT=600000

# Bluetooth stability
CONFIG_BT_CTLR_TX_PWR_PLUS_8=y
CONFIG_ZMK_BLE_EXPERIMENTAL_FEATURES=y
CONFIG_ZMK_BLE_KEYBOARD_REPORT_QUEUE_SIZE=32
CONFIG_ZMK_BLE_CONSUMER_REPORT_QUEUE_SIZE=12
CONFIG_ZMK_SPLIT_BLE_CENTRAL_POSITION_QUEUE_SIZE=32
CONFIG_ZMK_SPLIT_BLE_CENTRAL_SPLIT_RUN_QUEUE_SIZE=32
CONFIG_ZMK_SPLIT_BLE_PERIPHERAL_POSITION_QUEUE_SIZE=32
CONFIG_BT_MAX_CONN=5
CONFIG_BT_MAX_PAIRED=5

# Debounce
CONFIG_ZMK_KSCAN_DEBOUNCE_PRESS_MS=3
CONFIG_ZMK_KSCAN_DEBOUNCE_RELEASE_MS=5
```

## File operations

**Fetch from `GEIGEIGEIST/zmk-config-totem@master`:**
- `config/boards/shields/totem/` (all 9 files)
- `build.yaml` (seeeduino_xiao_ble + totem_left/right)
- `config/west.yml` (zmk `main`)
- `.github/workflows/build.yml` (replace with TOTEM repo version)

**Write new:** `config/totem.keymap` (per grids above), `config/totem.conf` (above).

**Delete:** `config/corne.keymap`, `config/corne.conf`, root `boards/`,
`zephyr/module.yml`.

**Update:** `LAYOUT_VISUALIZER_NEW.html` → TOTEM 38-key geometry + new layer
contents, keep the all-layers print view.

**Untouched/untracked:** `.zmk/` local west workspace is now stale (ZMK v0.3,
Corne-oriented). Local builds would need a fresh `west init`/`west update` per
the new west.yml; out of scope for this migration.

## Verification

- Push to GitHub → Actions build produces `totem_left-seeeduino_xiao_ble-zmk.uf2`
  and `totem_right-seeeduino_xiao_ble-zmk.uf2`.
- Flash both halves; check: base typing, home-row mods, all combos, tri-layer
  NAV, mouse layer, BT profile switching.
