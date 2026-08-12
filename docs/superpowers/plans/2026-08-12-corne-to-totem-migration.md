# Corne → TOTEM Migration Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Convert this repo in place from a Corne (nice_nano_v2) ZMK config to a TOTEM (Seeed XIAO BLE, no display) ZMK config, keeping only the Corne's layout idea.

**Architecture:** Repo skeleton (TOTEM shield, build.yaml, west.yml, workflow) is fetched verbatim from `GEIGEIGEIST/zmk-config-totem@master`. A new `config/totem.keymap` + `config/totem.conf` carry the migrated layout. Corne files are deleted. The HTML visualizer is updated to the 38-key grid.

**Tech Stack:** ZMK firmware (tracks `main`), devicetree keymaps, GitHub Actions (build = compiler check), one self-contained React/HTML visualizer.

## Global Constraints

- Spec: `docs/superpowers/specs/2026-08-12-corne-to-totem-migration-design.md` (approved).
- Hardware: `seeeduino_xiao_ble`, **no display, no RGB** — no display/nice!view/sleep-display config anywhere.
- TOTEM key positions (from `config/boards/shields/totem/totem.dtsi` transform):
  ```
  Row0:      0  1  2  3  4 │  5  6  7  8  9
  Row1:     10 11 12 13 14 │ 15 16 17 18 19
  Row2: 20 21 22 23 24 25  │ 26 27 28 29 30 31
  Thumb:       32 33 34    │ 35 36 37
  ```
  Left half = 0-4, 10-14, 20-25, 32-34. Right half = 5-9, 15-19, 26-31, 35-37.
- Every layer's `bindings` block must contain **exactly 38** `&…` tokens.
- Layer indices: BASE 0, COL 1, NUM 2, SYM 3, MSE 4, NAV 5.
- Do NOT touch the untracked `.zmk/` workspace or `.tmp_logs/` (stale, out of scope).
- Git commits are part of each task; the user has been asked for permission at execution start.

---

### Task 1: Fetch TOTEM skeleton, delete Corne files

**Files:**
- Create: `config/boards/shields/totem/` (10 files from upstream)
- Modify: `build.yaml`, `config/west.yml`, `.github/workflows/build.yml`
- Delete: `config/corne.keymap`, `config/corne.conf`, `zephyr/module.yml`, root `boards/`

**Interfaces:**
- Consumes: nothing.
- Produces: `config/boards/shields/totem/totem.dtsi` (defines key positions for all later tasks); `build.yaml` targeting `seeeduino_xiao_ble` + `totem_left`/`totem_right`.

- [ ] **Step 1: Fetch upstream files**

```bash
mkdir -p config/boards/shields/totem
BASE="https://raw.githubusercontent.com/GEIGEIGEIST/zmk-config-totem/master"
for f in Kconfig.defconfig Kconfig.shield totem.conf totem.dtsi totem.keymap totem.zmk.yml totem_left.conf totem_left.overlay totem_right.conf totem_right.overlay; do
  curl -fsSL "$BASE/config/boards/shields/totem/$f" -o "config/boards/shields/totem/$f"
done
curl -fsSL "$BASE/build.yaml" -o build.yaml
curl -fsSL "$BASE/config/west.yml" -o config/west.yml
curl -fsSL "$BASE/.github/workflows/build.yml" -o .github/workflows/build.yml
```

- [ ] **Step 2: Delete Corne-only files**

```bash
rm config/corne.keymap config/corne.conf zephyr/module.yml
rm -rf boards
```

- [ ] **Step 3: Verify structure**

Run: `ls config/boards/shields/totem && cat build.yaml`
Expected: 10 shield files listed; build.yaml contains exactly:

```yaml
include:
  - board: seeeduino_xiao_ble
    shield: totem_left
  - board: seeeduino_xiao_ble
    shield: totem_right
# there is no settingsreset (needed) for the XIAO
```

Also: `cat config/west.yml` → contains `revision: main`.

- [ ] **Step 4: Commit**

```bash
git add -A
git commit -m "Switch repo skeleton from Corne to TOTEM (XIAO BLE)"
```

---

### Task 2: Write `config/totem.conf`

**Files:**
- Create: `config/totem.conf`

**Interfaces:**
- Consumes: Task 1 skeleton.
- Produces: Kconfig enabling mouse, combos, BT stability, debounce — consumed by the ZMK build for both halves.

- [ ] **Step 1: Write the file**

```conf
CONFIG_ZMK_USB_LOGGING=n

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

- [ ] **Step 2: Verify no display config leaked in**

Run: `grep -ciE 'display|nice_view|rgb|underglow|widget|SPI' config/totem.conf`
Expected: `0`

- [ ] **Step 3: Commit**

```bash
git add config/totem.conf
git commit -m "Add totem.conf with mouse, BT stability, and debounce tuning"
```

---

### Task 3: Write `config/totem.keymap`

**Files:**
- Create: `config/totem.keymap`

**Interfaces:**
- Consumes: TOTEM key positions (Global Constraints), `behaviors.dtsi` from ZMK main.
- Produces: complete 6-layer keymap; combo positions; `mode_tap_left`/`mode_tap_right` behaviors used by BASE/COL rows; macros `m_arrow`, `m_interp`, `m_strict`, `m_snap_l`, `m_snap_r`, `m_snap_m`, `m_snap_c`, `m_screenshot` used by combos/NAV.

- [ ] **Step 1: Write the file**

Write `config/totem.keymap` with exactly this content:

```c
/*
 * TOTEM keymap - migrated from Corne config.
 * See docs/superpowers/specs/2026-08-12-corne-to-totem-migration-design.md
 *
 * Key positions (from totem shield transform):
 * Row0:      0  1  2  3  4 |  5  6  7  8  9
 * Row1:     10 11 12 13 14 | 15 16 17 18 19
 * Row2: 20 21 22 23 24 25  | 26 27 28 29 30 31
 * Thumb:       32 33 34    | 35 36 37
 */

#include <behaviors.dtsi>
#include <dt-bindings/zmk/bt.h>
#include <dt-bindings/zmk/keys.h>
#include <dt-bindings/zmk/outputs.h>
#include <dt-bindings/zmk/pointing.h>

#define BASE 0
#define COL  1
#define NUM  2
#define SYM  3
#define MSE  4
#define NAV  5

&mmv { time-to-max-speed-ms = <450>; };

&msc { acceleration-exponent = <1>; };

/ {
    combos {
        compatible = "zmk,combos";

        combo_esc {
            timeout-ms = <50>;
            key-positions = <16 17>;
            bindings = <&kp ESC>;
        };

        combo_semi {
            timeout-ms = <50>;
            key-positions = <17 18>;
            bindings = <&kp SEMI>;
        };

        combo_arrow {
            timeout-ms = <50>;
            key-positions = <12 13>;
            bindings = <&m_arrow>;
        };

        combo_interp {
            timeout-ms = <50>;
            key-positions = <11 12>;
            bindings = <&m_interp>;
        };

        combo_strict {
            timeout-ms = <50>;
            key-positions = <13 14>;
            bindings = <&m_strict>;
        };

        combo_screenshot {
            timeout-ms = <50>;
            key-positions = <7 8>;
            bindings = <&m_screenshot>;
        };

        combo_reset {
            timeout-ms = <50>;
            key-positions = <9 20 31>;
            bindings = <&sys_reset>;
        };

        combo_bootloader {
            timeout-ms = <50>;
            key-positions = <10 20 31>;
            bindings = <&bootloader>;
        };
    };

    conditional_layers {
        compatible = "zmk,conditional-layers";
        tri_layer {
            if-layers = <NUM SYM>;
            then-layer = <NAV>;
        };
    };

    macros {
        m_arrow: m_arrow {
            compatible = "zmk,behavior-macro";
            #binding-cells = <0>;
            bindings = <&kp EQUAL &kp GREATER_THAN>;
            label = "M_ARROW";
        };

        m_interp: m_interp {
            compatible = "zmk,behavior-macro";
            #binding-cells = <0>;
            bindings = <&kp DOLLAR &kp LBRC &kp RBRC &kp LEFT>;
            label = "M_INTERP";
        };

        m_strict: m_strict {
            compatible = "zmk,behavior-macro";
            #binding-cells = <0>;
            bindings = <&kp EXCL &kp EQUAL &kp EQUAL>;
            label = "M_STRICT";
        };

        m_snap_l: m_snap_l {
            compatible = "zmk,behavior-macro";
            #binding-cells = <0>;
            bindings = <&kp LG(LA(LEFT))>;
            label = "M_SNAP_L";
        };

        m_snap_r: m_snap_r {
            compatible = "zmk,behavior-macro";
            #binding-cells = <0>;
            bindings = <&kp LG(LA(RIGHT))>;
            label = "M_SNAP_R";
        };

        m_snap_m: m_snap_m {
            compatible = "zmk,behavior-macro";
            #binding-cells = <0>;
            bindings = <&kp LG(LA(F))>;
            label = "M_SNAP_M";
        };

        m_snap_c: m_snap_c {
            compatible = "zmk,behavior-macro";
            #binding-cells = <0>;
            bindings = <&kp LG(LA(C))>;
            label = "M_SNAP_C";
        };

        m_screenshot: m_screenshot {
            compatible = "zmk,behavior-macro";
            #binding-cells = <0>;
            bindings = <&kp LG(LC(LS(N4)))>;
            label = "M_SCREENSHOT";
        };
    };

    behaviors {
        mode_tap_left: mode_tap_left {
            compatible = "zmk,behavior-hold-tap";
            label = "MODE_TAP_LEFT";
            bindings = <&kp>, <&kp>;

            #binding-cells = <2>;
            tapping-term-ms = <175>;
            flavor = "tap-preferred";
            require-prior-idle-ms = <125>;
            quick-tap-ms = <0>;
            hold-trigger-key-positions = <5 6 7 8 9 15 16 17 18 19 26 27 28 29 30 31 35 36 37>;
        };

        mode_tap_right: mode_tap_right {
            compatible = "zmk,behavior-hold-tap";
            label = "MODE_TAP_RIGHT";
            bindings = <&kp>, <&kp>;

            #binding-cells = <2>;
            tapping-term-ms = <175>;
            flavor = "tap-preferred";
            require-prior-idle-ms = <125>;
            quick-tap-ms = <0>;
            hold-trigger-key-positions = <0 1 2 3 4 10 11 12 13 14 20 21 22 23 24 25 32 33 34>;
        };
    };

    keymap {
        compatible = "zmk,keymap";

        BASE {
            display-name = "Base";

            // ---------------------------------------------------------------
            // |  Q  |  W  |  E  |  R  |  T  |   |  Y  |  U  |  I  |  O  |  P  |
            // |  A  |  S  |  D  |  F  |  G  |   |  H  |  J  |  K  |  L  |  ;  |
            // | DEL |  Z  |  X  |  C  |  V  |  B  |  N  |  M  |  ,  |  .  |  /  | BSPC |
            //               | MSE | NUM | ENT |   | SPC | SYM | TAB |

            bindings = <
&kp Q  &kp W  &kp E  &kp R  &kp T     &kp Y  &kp U  &kp I  &kp O  &kp P
&mode_tap_left LEFT_SHIFT A  &mode_tap_left LEFT_CONTROL S  &mode_tap_left LEFT_GUI D  &mode_tap_left RIGHT_ALT F  &kp G     &kp H  &mode_tap_right RIGHT_ALT J  &mode_tap_right RIGHT_GUI K  &mode_tap_right RIGHT_CONTROL L  &mode_tap_right RIGHT_SHIFT SEMICOLON
&kp DEL  &kp Z  &kp X  &kp C  &kp V  &kp B     &kp N  &kp M  &kp COMMA  &kp DOT  &kp FSLH  &kp BSPC
&lt MSE ESCAPE  &mo NUM  &kp ENTER     &kp SPACE  &mo SYM  &kp TAB
            >;
        };

        COL {
            display-name = "Colemak";

            bindings = <
&kp Q  &kp W  &kp F  &kp P  &kp B     &kp J  &kp L  &kp U  &kp Y  &kp SQT
&mode_tap_left LEFT_SHIFT A  &mode_tap_left LEFT_CONTROL R  &mode_tap_left LEFT_GUI S  &mode_tap_left RIGHT_ALT T  &kp G     &kp M  &mode_tap_right RIGHT_ALT N  &mode_tap_right RIGHT_GUI E  &mode_tap_right RIGHT_CONTROL I  &mode_tap_right RIGHT_SHIFT O
&trans  &kp Z  &kp X  &kp C  &kp D  &kp V     &kp K  &kp H  &kp COMMA  &kp PERIOD  &kp SLASH  &trans
&trans  &mo NUM  &trans     &trans  &mo SYM  &trans
            >;
        };

        NUM {
            display-name = "Num";

            bindings = <
&kp N1  &kp N2  &kp N3  &kp N4  &kp N5     &kp N6  &kp N7  &kp N8  &kp N9  &kp N0
&bt BT_SEL 0  &bt BT_SEL 1  &bt BT_SEL 2  &bt BT_SEL 3  &bt BT_SEL 4     &kp LEFT  &kp DOWN  &kp UP  &kp RIGHT  &trans
&kp GRAVE  &kp C_BRI_DN  &kp C_BRI_UP  &kp C_VOLUME_DOWN  &kp C_VOL_UP  &kp K_MUTE     &trans  &trans  &trans  &trans  &tog COL  &trans
&trans  &trans  &trans     &kp LEFT_CONTROL  &trans  &trans
            >;
        };

        SYM {
            display-name = "Symbol";

            bindings = <
&kp EXCL  &kp AT_SIGN  &kp HASH  &kp DOLLAR  &kp PERCENT     &kp LBRC  &kp RBRC  &kp LPAR  &kp RPAR  &kp GRAVE
&kp CARET  &kp AMPERSAND  &kp ASTERISK  &kp UNDERSCORE  &kp PLUS     &kp LEFT_BRACKET  &kp RIGHT_BRACKET  &kp COLON  &kp DOUBLE_QUOTES  &kp SEMI
&kp LSHFT  &kp TILDE  &kp PIPE  &kp BSLH  &kp MINUS  &kp EQUAL     &kp LESS_THAN  &kp GREATER_THAN  &kp QUESTION  &kp SQT  &kp PERIOD  &kp ESC
&trans  &mo NUM  &trans     &trans  &trans  &trans
            >;
        };

        MSE {
            display-name = "Mouse";

            bindings = <
&bt BT_CLR  &trans  &kp F2  &trans  &trans     &trans  &trans  &trans  &trans  &out OUT_TOG
&trans  &mkp MCLK  &trans  &mkp LCLK  &mkp RCLK     &mmv MOVE_LEFT  &mmv MOVE_DOWN  &mmv MOVE_UP  &mmv MOVE_RIGHT  &trans
&trans  &trans  &trans  &msc SCRL_UP  &msc SCRL_DOWN  &trans     &trans  &trans  &trans  &trans  &trans  &trans
&trans  &trans  &trans     &trans  &trans  &trans
            >;
        };

        NAV {
            display-name = "Nav";

            bindings = <
&kp F1  &kp F2  &kp F3  &kp F4  &kp F5     &kp F6  &kp F7  &kp F8  &kp F9  &kp F10
&trans  &kp LA(SPACE)  &kp LG(K)  &trans  &trans     &kp LEFT  &kp DOWN  &kp UP  &kp RIGHT  &kp LG(P)
&kp F11  &m_snap_l  &m_snap_c  &m_snap_m  &m_snap_r  &trans     &kp HOME  &kp LG(LS(LBKT))  &kp LG(LS(RBKT))  &kp END  &kp LC(GRAVE)  &kp F12
&trans  &trans  &trans     &trans  &trans  &trans
            >;
        };
    };
};
```

- [ ] **Step 2: Run structural validation**

Save and run this script (e.g. to the temp dir, not the repo):

```python
import re, sys

src = open("config/totem.keymap").read()
errors = []

# 1. Each layer's bindings block has exactly 38 &-tokens
for layer in ["BASE", "COL", "NUM", "SYM", "MSE", "NAV"]:
    m = re.search(rf"\b{layer}\s*\{{(.*?)bindings\s*=\s*<(.*?)>;", src, re.S)
    if not m:
        errors.append(f"{layer}: layer/bindings not found")
        continue
    count = len(re.findall(r"&\w+", m.group(2)))
    if count != 38:
        errors.append(f"{layer}: {count} bindings, expected 38")

# 2. Combo positions within 0..37
for cm in re.finditer(r"key-positions\s*=\s*<([^>]*)>", src):
    for p in cm.group(1).split():
        if not (0 <= int(p) <= 37):
            errors.append(f"combo position {p} out of range")

# 3. Hold-tap triggers reference only the opposite half
LEFT = {0,1,2,3,4,10,11,12,13,14,20,21,22,23,24,25,32,33,34}
RIGHT = {5,6,7,8,9,15,16,17,18,19,26,27,28,29,30,31,35,36,37}
for name, expected in [("mode_tap_left", RIGHT), ("mode_tap_right", LEFT)]:
    m = re.search(rf"{name}\s*\{{(.*?)hold-trigger-key-positions\s*=\s*<([^>]*)>", src, re.S)
    positions = {int(p) for p in m.group(2).split()}
    if positions != expected:
        errors.append(f"{name}: trigger set mismatch: {sorted(positions ^ expected)}")

# 4. All referenced behaviors are defined locally or are ZMK builtins
LOCAL = {"mode_tap_left", "mode_tap_right", "m_arrow", "m_interp", "m_strict",
         "m_snap_l", "m_snap_r", "m_snap_m", "m_snap_c", "m_screenshot"}
BUILTIN = {"kp", "mo", "lt", "tog", "bt", "out", "trans", "none", "mkp",
           "mmv", "msc", "sys_reset", "bootloader"}
for name in set(re.findall(r"&(\w+)", src)):
    if name not in LOCAL | BUILTIN:
        errors.append(f"unknown behavior &{name}")

print("\n".join(errors) if errors else "ALL CHECKS PASSED")
sys.exit(1 if errors else 0)
```

Run: `python3 /path/to/validate_totem.py`
Expected: `ALL CHECKS PASSED`

- [ ] **Step 3: Commit**

```bash
git add config/totem.keymap
git commit -m "Migrate Corne keymap to TOTEM 38-key grid"
```

---

### Task 4: Update `LAYOUT_VISUALIZER_NEW.html` to TOTEM

**Files:**
- Modify: `LAYOUT_VISUALIZER_NEW.html`

**Interfaces:**
- Consumes: layer contents from Task 3.
- Produces: visualizer whose `LAYERS` data and geometry match the TOTEM keymap. Rows are 5/5/6 keys wide (6th bottom-row key = extra outer key, DEL/BSPC on BASE).

- [ ] **Step 1: Replace the `LAYERS` constant**

```js
const LAYERS = {
    BASE: {
        name: "Base",
        description: "Alpha layer with Home Row Mods (Shift, Ctrl, Cmd, Alt)",
        left: [
            ["Q", "W", "E", "R", "T"],
            ["A (⇧)", "S (⌃)", "D (⌘)", "F (⌥)", "G"],
            ["DEL", "Z", "X", "C", "V", "B"]
        ],
        right: [
            ["Y", "U", "I", "O", "P"],
            ["H", "J (⌥)", "K (⌘)", "L (⌃)", "; (⇧)"],
            ["N", "M", ",", ".", "/", "⌫"]
        ],
        thumbs: ["ESC (MSE)", "NUM", "ENT", "SPC", "SYM", "TAB"]
    },
    COL: {
        name: "Colemak",
        description: "Colemak-DH alphas with home-row mods (hold for Shift/Ctrl/Cmd/Alt)",
        left: [
            ["Q", "W", "F", "P", "B"],
            ["A (⇧)", "R (⌃)", "S (⌘)", "T (⌥)", "G"],
            ["", "Z", "X", "C", "D", "V"]
        ],
        right: [
            ["J", "L", "U", "Y", "'"],
            ["M", "N (⌥)", "E (⌘)", "I (⌃)", "O (⇧)"],
            ["K", "H", ",", ".", "/", ""]
        ],
        thumbs: ["", "NUM", "", "", "SYM", ""]
    },
    NUM: {
        name: "Numbers/BT",
        description: "Numbers, Bluetooth profile selection, and Brightness/Volume",
        left: [
            ["1", "2", "3", "4", "5"],
            ["BT 0", "BT 1", "BT 2", "BT 3", "BT 4"],
            ["`", "BRI-", "BRI+", "VOL-", "VOL+", "MUTE"]
        ],
        right: [
            ["6", "7", "8", "9", "0"],
            ["←", "↓", "↑", "→", ""],
            ["", "", "", "", "TOG COL", ""]
        ],
        thumbs: ["", "", "", "⌃", "", ""]
    },
    SYM: {
        name: "Symbols",
        description: "Coding symbols optimized for JavaScript/TypeScript development",
        left: [
            ["!", "@", "#", "$", "%"],
            ["^", "&", "*", "_", "+"],
            ["⇧", "~", "|", "\\", "-", "="]
        ],
        right: [
            ["{", "}", "(", ")", "`"],
            ["[", "]", ":", "\"", ";"],
            ["<", ">", "?", "'", ".", "ESC"]
        ],
        thumbs: ["", "NUM", "", "", "", ""]
    },
    MSE: {
        name: "Mouse",
        description: "Mouse movement, clicks, and scroll wheel emulation",
        left: [
            ["BT CLR", "", "F2", "", ""],
            ["", "MCLK", "", "LCLK", "RCLK"],
            ["", "", "", "SCRL↑", "SCRL↓", ""]
        ],
        right: [
            ["", "", "", "", "OUT"],
            ["M-←", "M-↓", "M-↑", "M-→", ""],
            ["", "", "", "", "", ""]
        ],
        thumbs: ["", "", "", "", "", ""]
    },
    NAV: {
        name: "Navigation",
        description: "F-keys, Window management (Magnet), and VS Code shortcuts",
        left: [
            ["F1", "F2", "F3", "F4", "F5"],
            ["", "Raycast", "Cmd+K", "", ""],
            ["F11", "Snap L", "Snap C", "Snap M", "Snap R", ""]
        ],
        right: [
            ["F6", "F7", "F8", "F9", "F10"],
            ["←", "↓", "↑", "→", "File"],
            ["HOME", "Tab-", "Tab+", "END", "Term", "F12"]
        ],
        thumbs: ["", "", "", "", "", ""]
    }
};
```

(`COMBOS` stays unchanged — all six combos use the same letter pairs as before.)

- [ ] **Step 2: Make `renderSide` half-aware so the 6-key bottom row sticks out on the outer edge**

Change the signature to `renderSide(sideData, isLeft)` and add `justify-end` to rows of the left half. Full replacement:

```js
function renderSide(sideData, isLeft) {
    return (
        <div className="grid grid-rows-3 gap-0.5">
            {sideData.map((row, i) => (
                <div key={i} className={`flex ${isLeft ? "justify-end" : ""}`}>
                    {row.map((key, j) => {
                        let type = "normal";
                        if (!key) type = "empty";
                        else if (key.includes("(") || ["⌃", "⇧", "DEL", "⌫", "ESC"].includes(key)) type = "mod";
                        else if (["NUM", "SYM", "NAV", "MSE", "COL"].some(l => key.includes(l))) type = "layer";
                        else if (["Raycast", "Term", "File", "Snap", "OUT"].some(s => key.includes(s))) type = "special";
                        return <Key key={j} label={key} type={type} />;
                    })}
                </div>
            ))}
        </div>
    );
}
```

Update the two call sites in `Board`: `renderSide(data.left, true)` and `renderSide(data.right, false)`.

- [ ] **Step 3: Retitle Corne → TOTEM**

- `<title>TOTEM ZMK Layout Visualizer</title>`
- Header `<h1>TOTEM ZMK Configuration</h1>`, version text `v4.0 - TOTEM`
- Print title: `TOTEM ZMK Configuration — All Layers`

- [ ] **Step 4: Verify in browser**

Run: `open LAYOUT_VISUALIZER_NEW.html`
Expected: 38 keys per layer (5/5/6 per side + 3 thumbs each); BASE shows DEL bottom-outer-left and ⌫ bottom-outer-right; print preview (Cmd+P) shows all 6 layers in B&W with halves side by side.

- [ ] **Step 5: Commit**

```bash
git add LAYOUT_VISUALIZER_NEW.html
git commit -m "Update layout visualizer to TOTEM 38-key grid"
```

---

### Task 5: Push and verify the GitHub Actions build

**Files:**
- None (CI verification only).

**Interfaces:**
- Consumes: Tasks 1-4.
- Produces: green Actions run with `totem_left-seeeduino_xiao_ble-zmk.uf2` and `totem_right-seeeduino_xiao_ble-zmk.uf2` artifacts.

- [ ] **Step 1: Push**

```bash
git push
```

- [ ] **Step 2: Watch the build**

```bash
gh run watch
```
Expected: build succeeds for both halves. If it fails (e.g. a Kconfig symbol renamed in ZMK `main`), read the log with `gh run view --log`, fix the offending line, commit, push, repeat.

- [ ] **Step 3: Confirm artifacts**

```bash
gh run view
```
Expected: firmware archive contains `totem_left-seeeduino_xiao_ble-zmk.uf2` and `totem_right-seeeduino_xiao_ble-zmk.uf2`.

---

## Self-Review Notes

- Spec coverage: skeleton fetch (T1), totem.conf (T2), keymap w/ combos+behaviors+macros+tri-layer (T3), visualizer (T4), CI verification (T5). Corne deletions in T1. All spec sections covered.
- Binding counts per layer hand-counted to 38 each; machine-checked in T3 Step 2.
- Known risk: `&mmv`/`&msc` property overrides and `CONFIG_ZMK_KSCAN_DEBOUNCE_*`/`CONFIG_BT_CTLR_TX_PWR_PLUS_8` on ZMK `main` — accepted risk, caught by CI in T5 with fix loop.
