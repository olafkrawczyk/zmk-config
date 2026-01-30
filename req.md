# ZMK ESB Dongle Implementation Study
## Technical Guide for Corne Keyboard with nice!nano v2 and XIAO nRF52840

**Hardware Configuration**
- Keyboard: Corne (split keyboard)
- Controllers: nice!nano v2 (left and right halves)
- Dongle: Seeed XIAO nRF52840 BLE
- Displays: nice!view on both halves
- Target: Screenless dongle, low-latency USB HID output

**Objective**: Replace Bluetooth host connectivity with USB dongle using ESB (Enhanced ShockBurst) protocol for lowest possible latency and extended battery life.

---

## Executive Summary

ZMK firmware does not officially support USB dongle mode with ESB transport as of v0.3.1 (August 2025)[1]. However, community-maintained modules enable this functionality through two primary approaches:

1. **BLE-based dongle mode** (officially supported architecture, unofficial documentation)
2. **ESB-based dongle mode** (community module, significant latency reduction)

For your goal of "lowest latency possible and extended battery life," the ESB approach is optimal, reducing split transport latency from ~7.5ms (BLE) to sub-millisecond levels[2][3].

---

## Architecture Options

### Option A: BLE Split Transport + USB Dongle (Baseline)

**Architecture:**
Left Half (nice!nano v2, peripheral) ←──BLE──→ Dongle (XIAO nRF52840, central) ──USB──> PC
Right Half (nice!nano v2, peripheral) ←──BLE──┘

**Characteristics:**
- Uses standard ZMK split BLE transport
- Dongle acts as ZMK central, outputs USB HID to computer
- Computer never uses Bluetooth (dongle handles wireless)
- Battery life improvement: former central (now peripheral) gains ~50% battery life[4]
- Latency: ~7.5ms worst-case for BLE split transport[5]

**Status**: Supported by ZMK architecture, but lacks official documentation[6][7]

### Option B: ESB Split Transport + USB Dongle (Target Configuration)

**Architecture:**
Left Half (nice!nano v2, peripheral) ←──ESB──→ Dongle (XIAO nRF52840, central) ──USB──> PC
Right Half (nice!nano v2, peripheral) ←──ESB──┘

**Characteristics:**
- Uses Nordic Enhanced ShockBurst (2.4 GHz proprietary, not Bluetooth)
- Significantly reduced latency: <1ms over-the-air typical[8]
- No Bluetooth radio activity (ESB operates on different protocol stack)
- Improved battery life due to lower protocol overhead[9]
- Dongle outputs USB HID to computer

**Status**: Community module, requires fork/module integration

---

## Implementation Repositories

### Primary: kmobs/zmk-config

**Repository**: https://github.com/kmobs/zmk-config[9]

**Description**: Complete working example of ESB split transport with XIAO BLE dongle for Ferris Sweep. Explicitly designed for "reduced latency and reliable peripheral links" with "USB dongle for desktop connectivity"[9].

**Key Features**:
- ESB-based wireless split transport implementation
- Dedicated XIAO BLE dongle build target
- Reduced latency between splits and desktop
- Improved battery longevity
- USB dongle can wake computer from sleep

**Maintenance Status**: Repository shows activity through November 2025 (Reddit post date)[9]

**Application to Corne**: Shield/overlay configuration would need adaptation from Ferris Sweep matrix (36 keys) to Corne matrix (42 keys), but transport and dongle configuration is directly applicable.

### ESB Module: badjeff/zmk-feature-split-esb

**Repository**: Referenced in community discussions[10]

**Description**: ZMK module implementing ESB split transport as alternative to BLE split. This is the underlying module that enables ESB functionality.

**Integration Method**: West manifest module (config/west.yml)

**Status**: Actively referenced in 2025 community discussions[10]. Requires verification of commit history to confirm 12-month maintenance requirement.

**Warning**: This module is not in zmkfirmware official repositories and represents a fork/extension of ZMK's split transport layer.

### Supporting: mctechnology17/zmk-dongle-display-view

**Repository**: https://github.com/mctechnology17/zmk-dongle-display-view[11]

**Description**: Module for using nice!view display on dongle (optional, not required for your screenless dongle configuration)

**Relevance**: Demonstrates dongle-specific shield configurations for XIAO nRF52840

**Last Updated**: August 2024[11]

### Reference: ZMK Official Dongle Documentation

**Source**: https://zmk.dev/docs/development/hardware-integration/dongle[4]

**Description**: Official ZMK documentation describing dongle architecture principles (BLE-based)

**Key Configuration**: 
CONFIG_ZMK_SPLIT_BLE_CENTRAL_PERIPHERALS=2  # For two halves
CONFIG_BT_MAX_CONN=7  # PERIPHERALS + BT profiles (default 5)

**Status**: Documentation exists but marked as development/integration documentation, not user-facing setup guide[6]

---

## Critical Dependencies and Version Constraints

### ZMK Version Compatibility

**Target Version**: ZMK v0.3.0 (August 2025) or v0.3.1 (current)[1]

**Critical Feature**: Runtime selection of split transport introduced in v0.3.0[1]
Commit 6b44d33: "split: Runtime selection of split transport"

This feature may simplify switching between BLE and ESB transports, but ESB module compatibility with v0.3.x requires verification.

### ESB Module Compatibility Risk

**Potential Bottleneck**: ESB split transport modules may target older ZMK versions or specific branches.

**Verification Required**:
1. Check badjeff/zmk-feature-split-esb for explicit ZMK version compatibility
2. Review kmobs/zmk-config for ZMK base version in west.yml manifest
3. Confirm no hard dependencies on pre-v0.3 split transport APIs

**Migration Path if Incompatible**:
- Pin ZMK to last known compatible version in west.yml
- Monitor ESB module repository for v0.3.x updates
- Contribute compatibility patches if module is maintained but not yet updated

### Zephyr RTOS Dependency

**Consideration**: ZMK runs on Zephyr RTOS. ESB implementation may depend on specific Zephyr versions bundled with ZMK releases.

**Action**: Use ZMK's declared Zephyr version from main repository manifest, do not override.

### Nordic ESB SDK

**Implementation Detail**: ESB protocol is part of Nordic nRF5 SDK, integrated into Zephyr. No separate SDK installation required, but firmware must enable Nordic ESB subsystem in Zephyr.

**Kconfig Requirements**: ESB module will define necessary Kconfig symbols. Typical symbols:
CONFIG_ESB=y
CONFIG_ZMK_SPLIT_BLE=n  # Disable BLE split when using ESB

---

## nice!view Display Support on Peripherals

### Current Architecture

In standard ZMK split configuration, displays work on both central and peripheral halves:
- Central: Runs full status screen (battery, layer, output, etc.)
- Peripheral: Runs same status screen independently[12]

### Dongle Mode Implications

**Scenario**: Dongle (central) has no screen. Left and right halves (peripherals) have nice!view displays.

**Expected Behavior**:

1. **Peripheral displays continue to function** for local state (layer, modifiers, WPM)[13]
2. **Battery level reporting requires configuration**:
   - Peripherals can display their own battery level
   - Central (dongle) battery level not visible on peripheral screens by default
   - Cross-peripheral battery levels (left seeing right's battery) requires special configuration

### Configuration for Peripheral Displays

**Enable Display on Peripherals** (build.yaml):
include:
  - board: nice_nano_v2
    shield: corne_left nice_view_adapter nice_view
  - board: nice_nano_v2
    shield: corne_right nice_view_adapter nice_view
  - board: seeeduino_xiao_ble
    shield: corne_dongle  # Custom dongle shield, no display shields

**Enable Custom Status Screen** (corne.conf or shield-specific .conf):
CONFIG_ZMK_DISPLAY=y
CONFIG_ZMK_DISPLAY_STATUS_SCREEN_CUSTOM=y

**Enable External Power Toggle** (required for nice!view)[14]:

In keymap file:
#include <dt-bindings/zmk/ext_power.h>

// Add to keymap layer:
&ext_power EP_TOG

**Battery Level Monitoring Configuration**:

For dongle to fetch peripheral battery levels (optional, affects BLE overhead):
CONFIG_ZMK_SPLIT_BLE_CENTRAL_BATTERY_LEVEL_FETCHING=y
CONFIG_ZMK_SPLIT_BLE_CENTRAL_BATTERY_LEVEL_PROXY=y

**Note**: With ESB transport, BLE battery reporting may not function. Peripherals can only display their own battery levels.

### ESB Transport Display Considerations

**Unknown Factor**: Whether peripheral status screens update correctly when using ESB instead of BLE split transport.

**Likely Scenario**: Display updates are driven by local events (keypresses, layer changes), not transport protocol, so peripherals should continue displaying status.

**Testing Required**: Verify that peripheral displays update layer/modifier state correctly when central is dongle using ESB.

**Fallback**: If peripheral displays fail to show synchronized state, this is a known limitation of the ESB module and may require module enhancement.

### Display Module Options (Optional Enhancement)

If peripheral displays need to show dongle battery or cross-peripheral information, consider:

**zmk-dongle-display module** by englmaxi[15]
- Custom status screen for 128x64 OLED/nice!view on dongle
- Shows peripheral battery percentages on dongle screen

**zmk-dongle-display-view** by mctechnology17[11]
- Adapted version of above for nice!view specifically
- Supports pro_micro and seeeduino_xiao_ble pinouts

**Application to Your Setup**: These modules are for dongle displays. For your screenless dongle + peripheral displays setup, standard nice!view configuration should suffice.

---

## Implementation Roadmap

### Phase 1: Baseline BLE Dongle (De-risk Strategy)

**Goal**: Establish working dongle mode with standard BLE split before attempting ESB.

**Steps**:
1. Create dongle shield definition for XIAO nRF52840 in zmk-config
2. Configure build.yaml for three targets:
   - `seeeduino_xiao_ble` + `corne_dongle` (central)
   - `nice_nano_v2` + `corne_left` (peripheral)
   - `nice_nano_v2` + `corne_right` (peripheral)
3. Set Kconfig for dongle:
   CONFIG_ZMK_SPLIT=y
   CONFIG_ZMK_SPLIT_ROLE_CENTRAL=y
   CONFIG_ZMK_SPLIT_BLE_CENTRAL_PERIPHERALS=2
   CONFIG_BT_MAX_CONN=7
   CONFIG_USB_DEVICE_VID=0x1234  # Your vendor ID
   CONFIG_USB_DEVICE_PID=0x5678  # Your product ID
4. Set Kconfig for peripherals:
   CONFIG_ZMK_SPLIT=y
   CONFIG_ZMK_SPLIT_ROLE_PERIPHERAL=y
5. Test pairing: peripherals should auto-pair with dongle on first boot
6. Verify USB HID output to computer
7. Verify nice!view displays on peripherals

**Expected Latency**: ~7.5ms worst-case split transport[5]

**Success Criteria**: 
- Computer recognizes keyboard via USB (not Bluetooth)
- Both halves report keypresses through dongle
- Peripheral displays show layer/battery status

### Phase 2: ESB Transport Integration

**Goal**: Replace BLE split transport with ESB for latency reduction.

**Steps**:

1. **Add ESB module to west.yml**:
   manifest:
     remotes:
       - name: badjeff
         url-base: https://github.com/badjeff
     projects:
       - name: zmkfirmware/zmk
         remote: zmkfirmware
         revision: v0.3.1  # Or main
         import: app/west.yml
       - name: zmk-feature-split-esb
         remote: badjeff
         revision: main  # Verify latest stable branch
     self:
       path: config

2. **Update Kconfig for ESB transport**:
   
   Dongle (corne_dongle.conf):
   CONFIG_ZMK_SPLIT=y
   CONFIG_ZMK_SPLIT_ROLE_CENTRAL=y
   CONFIG_ZMK_SPLIT_BLE=n  # Disable BLE split
   CONFIG_ZMK_SPLIT_ESB=y  # Enable ESB split
   CONFIG_ZMK_SPLIT_BLE_CENTRAL_PERIPHERALS=2
   
   Peripherals (corne_left.conf, corne_right.conf):
   CONFIG_ZMK_SPLIT=y
   CONFIG_ZMK_SPLIT_ROLE_PERIPHERAL=y
   CONFIG_ZMK_SPLIT_BLE=n
   CONFIG_ZMK_SPLIT_ESB=y

3. **Reference kmobs/zmk-config** for:
   - ESB-specific devicetree overlays (if required)
   - Channel/address configuration for ESB
   - Any shield-specific ESB initialization

4. **Adapt Ferris Sweep config to Corne**:
   - Copy ESB transport configuration
   - Modify matrix definitions for Corne (6 cols × 4 rows per half)
   - Preserve display configurations

5. **Build and flash**:
   west build -p -b seeeduino_xiao_ble -- -DSHIELD=corne_dongle
   west build -p -b nice_nano_v2 -- -DSHIELD="corne_left nice_view_adapter nice_view"
   west build -p -b nice_nano_v2 -- -DSHIELD="corne_right nice_view_adapter nice_view"

6. **Test ESB pairing**: 
   - ESB uses different pairing mechanism than BLE
   - May require specific pairing sequence (documented in module README)
   - Verify both peripherals connect to dongle

7. **Measure latency improvement**: 
   - Use high-speed camera or oscilloscope to measure keypress-to-USB-event latency
   - Compare against Phase 1 baseline

**Expected Latency**: Sub-millisecond ESB transport[8], plus USB polling interval (1ms typical)

**Success Criteria**:
- Dongle connects to peripherals via ESB (not BLE)
- Measurable latency reduction vs. baseline
- Peripheral displays continue functioning
- Battery life extends due to reduced protocol overhead

### Phase 3: Optimization and Validation

**Goal**: Maximize battery life and minimize latency.

**Actions**:

1. **Tune ESB parameters** (if exposed by module):
   - Transmission power (lower = better battery, may reduce range)
   - Retransmit count (balance reliability vs. latency)
   - Polling interval (faster = lower latency, higher power)

2. **Configure peripheral sleep settings**:
   CONFIG_ZMK_SLEEP=y
   CONFIG_ZMK_IDLE_SLEEP_TIMEOUT=900000  # 15 minutes
   CONFIG_ZMK_IDLE_TIMEOUT=30000  # 30 seconds to idle

3. **Disable unused features on peripherals**:
   CONFIG_ZMK_BLE=n  # No BLE needed on peripherals with ESB
   CONFIG_BT=n  # Disable Bluetooth stack entirely

4. **Monitor battery life**:
   - Track mAh consumption over time
   - Compare against standard BLE split configuration
   - Document improvement metrics

5. **Validate display behavior**:
   - Ensure layer changes reflect on peripheral displays
   - Test modifier state visibility
   - Verify battery percentage accuracy

**Success Criteria**:
- Battery life ≥2x improvement over BLE split (target)
- Latency <2ms end-to-end (keypress to USB HID)
- Displays fully functional on peripherals
- Stable operation over extended use (no disconnects)

---

## Potential Bottlenecks and Risks

### 1. ESB Module Maintenance Status

**Risk**: zmk-feature-split-esb may not have commits in past 12 months.

**Mitigation**:
- Verify commit history before starting implementation
- If unmaintained, assess fork viability:
  - Check open issues/PRs for unresolved bugs
  - Evaluate code quality and test coverage
  - Consider maintaining personal fork if necessary

**Fallback**: Use BLE dongle mode (Phase 1) as production configuration until maintained ESB module emerges.

### 2. ZMK v0.3.x Compatibility

**Risk**: ESB module may target ZMK v0.2.x or earlier, incompatible with v0.3.x API changes.

**Indicators**:
- Build errors referencing changed function signatures
- Missing Kconfig symbols
- Devicetree binding incompatibilities

**Mitigation**:
- Pin ZMK to last compatible version in west.yml if necessary
- Monitor ESB module repository for v0.3.x compatibility updates
- Engage with module maintainer (badjeff) for compatibility roadmap

**Fallback**: Run on ZMK v0.2.x if ESB critical to requirements, accepting loss of v0.3.x features (Studio support, latest bug fixes).

### 3. Peripheral Display Synchronization with ESB

**Risk**: Peripheral displays may not update correctly when using ESB transport due to missing state synchronization messages.

**Likely Cause**: ESB module may not implement all BLE split GATT characteristics for battery/state reporting.

**Indicators**:
- Displays show stale layer information
- Battery percentages freeze or show incorrect values
- WPM/modifier widgets not updating

**Mitigation**:
- Test thoroughly in Phase 2, step 6
- Review ESB module code for state sync implementation
- May require custom display status screen that only shows local peripheral state

**Fallback**: Accept peripheral displays showing only local state (layer active on that half, local battery), not full keyboard state.

### 4. ESB Pairing Complexity

**Risk**: ESB pairing mechanism may be less automatic than BLE split pairing.

**Potential Issues**:
- Manual address configuration required in devicetree
- No automatic pairing/bonding like BLE
- Channel selection conflicts or interference

**Mitigation**:
- Follow kmobs/zmk-config ESB configuration exactly for initial setup
- Document pairing procedure for future re-flashing
- May need physical reset sequence to re-establish ESB link

**Fallback**: None; this is inherent to ESB protocol. Document clear pairing procedure.

### 5. Power Consumption Assumptions

**Risk**: ESB may not provide expected battery life improvement if implemented inefficiently.

**Factors**:
- ESB polling rate vs. BLE connection interval
- Retransmit overhead in noisy RF environment
- Wake-from-sleep latency (ESB may require faster wake than BLE)

**Mitigation**:
- Measure actual battery consumption in Phase 3
- Adjust ESB parameters based on measurements
- Compare against BLE baseline with same usage patterns

**Fallback**: If ESB shows worse battery life than BLE, revert to BLE dongle (Phase 1) and accept higher latency.

### 6. USB HID Boot Protocol Support

**Risk**: Dongle may not support USB HID boot protocol, preventing BIOS/bootloader keyboard access.

**Status**: ZMK has open discussion on boot protocol enablement[16]

**Indicators**:
- Keyboard non-functional in BIOS/UEFI
- No input during bootloader (GRUB, etc.)

**Mitigation**:
- Test dongle keyboard in BIOS immediately after Phase 1
- If unsupported, track ZMK issue #632 for boot protocol implementation[16]

**Fallback**: Keep wired USB keyboard for BIOS access, or connect one half directly via USB when needed (ZMK auto-switches to USB mode).

### 7. nice!view Power Management with ESB

**Risk**: External power toggle for nice!view may interact unexpectedly with ESB sleep modes.

**Potential Issue**: Display may not wake from sleep or drain battery if ESB wake behavior differs from BLE.

**Mitigation**:
- Test display wake-from-sleep thoroughly
- Review ESB module for external power (ext_power) subsystem integration
- May need custom power management configuration

**Fallback**: Disable auto-sleep or tune sleep timeout to avoid issue.

---

## Configuration Reference

### Minimal Dongle Configuration (BLE Baseline)

**File: config/boards/shields/corne_dongle/corne_dongle.overlay**

/ {
    chosen {
        zmk,kscan = &kscan_none;
    };
};

&kscan_none {
    compatible = "zmk,kscan-none";
};

**File: config/boards/shields/corne_dongle/corne_dongle.conf**

# Enable split
CONFIG_ZMK_SPLIT=y
CONFIG_ZMK_SPLIT_ROLE_CENTRAL=y
CONFIG_ZMK_SPLIT_BLE=y
CONFIG_ZMK_SPLIT_BLE_CENTRAL_PERIPHERALS=2

# Increase BLE connections for split + profiles
CONFIG_BT_MAX_CONN=7
CONFIG_BT_MAX_PAIRED=7

# USB HID
CONFIG_USB_DEVICE_MANUFACTURER="YourName"
CONFIG_USB_DEVICE_PRODUCT="Corne Dongle"
CONFIG_USB_DEVICE_VID=0x1234
CONFIG_USB_DEVICE_PID=0x5678

# Disable display (dongle has no screen)
CONFIG_ZMK_DISPLAY=n

# Enable logging for debugging (disable in production)
CONFIG_ZMK_USB_LOGGING=y

**File: config/boards/shields/corne_dongle/Kconfig.shield**

config SHIELD_CORNE_DONGLE
    def_bool $(shields_list_contains,corne_dongle)

**File: config/boards/shields/corne_dongle/Kconfig.defconfig**

if SHIELD_CORNE_DONGLE

config ZMK_KEYBOARD_NAME
    default "Corne"

endif

### Peripheral Configuration (nice!view Enabled)

**File: config/corne.conf** (applied to left/right shields)

# Enable display
CONFIG_ZMK_DISPLAY=y
CONFIG_ZMK_DISPLAY_STATUS_SCREEN_CUSTOM=n  # Use default or custom

# Battery reporting (peripheral side)
CONFIG_ZMK_BATTERY_REPORTING=y

# Sleep settings for battery life
CONFIG_ZMK_SLEEP=y
CONFIG_ZMK_IDLE_SLEEP_TIMEOUT=900000
CONFIG_ZMK_IDLE_TIMEOUT=30000

# Logging (disable in production)
# CONFIG_ZMK_USB_LOGGING=n

### Build Configuration

**File: config/build.yaml**

---
include:
  # Dongle (central)
  - board: seeeduino_xiao_ble
    shield: corne_dongle
  # Left peripheral with nice!view
  - board: nice_nano_v2
    shield: corne_left nice_view_adapter nice_view
  # Right peripheral with nice!view
  - board: nice_nano_v2
    shield: corne_right nice_view_adapter nice_view

### West Manifest for ESB (Phase 2)

**File: config/west.yml**

manifest:
  remotes:
    - name: zmkfirmware
      url-base: https://github.com/zmkfirmware
    - name: badjeff
      url-base: https://github.com/badjeff
  
  projects:
    - name: zmk
      remote: zmkfirmware
      revision: v0.3.1
      import: app/west.yml
    
    # ESB split transport module
    - name: zmk-feature-split-esb
      remote: badjeff
      revision: main  # Verify branch/tag
  
  self:
    path: config

**Action**: After modifying west.yml, run:
west update

---

## Testing and Validation Protocol

### Phase 1 Validation (BLE Dongle)

**Hardware Setup**:
1. Flash dongle firmware to XIAO nRF52840
2. Flash left/right firmware to nice!nano v2 boards
3. Power on dongle (via USB to PC)
4. Power on left half (battery or USB)
5. Power on right half (battery or USB)

**Expected Behavior**:
- Dongle LED indicates power (XIAO onboard LED)
- Peripherals auto-pair with dongle (may take 10-30 seconds first boot)
- nice!view displays activate showing layer/battery
- Computer recognizes USB HID keyboard device

**Tests**:
1. Type keys from left half → characters appear in text editor
2. Type keys from right half → characters appear
3. Test layer switching → displays update, layer-specific keys work
4. Check battery percentage on displays
5. Verify Bluetooth disabled on PC (dongle handles all wireless)
6. Measure latency using typingtest.com or similar
7. Test sleep/wake: leave idle 15 minutes, verify wake-on-keypress

**Pass Criteria**: All keys functional, displays working, no PC Bluetooth required.

### Phase 2 Validation (ESB Transport)

**Hardware Setup**: Same as Phase 1, but with ESB firmware.

**Expected Behavior**:
- ESB pairing (mechanism may differ from BLE)
- Lower latency than Phase 1
- Displays continue functioning

**Tests**:
1. Verify ESB link establishment (check logs if enabled)
2. Repeat all Phase 1 tests
3. Latency measurement: compare against Phase 1 baseline
   - High-speed camera filming keypress + screen update (240fps+)
   - Oscilloscope if available (GPIO toggle on keypress, measure to USB HID)
4. Stress test: rapid keypresses, verify no missed events
5. Range test: verify function at typical desk distances (0.5-1.0m)
6. Interference test: nearby WiFi/BT devices, verify stable connection

**Pass Criteria**: 
- Latency <2ms improvement over BLE baseline
- All tests from Phase 1 pass
- Displays functional

### Phase 3 Validation (Battery Life)

**Test Duration**: 7-14 days normal use

**Metrics**:
1. Initial battery voltage (full charge, ~4.2V LiPo)
2. Battery percentage reported by ZMK
3. Daily usage hours
4. Total runtime until shutdown (~3.3V cutoff)

**Calculation**:
Estimated Days of Use = (Battery mAh) / (Average Daily mAh Consumption)

For nice!nano typical 110mAh battery:
- **Target**: >30 days moderate use (2-4 hours/day typing)
- **BLE baseline**: ~14-21 days (peripheral in standard split)
- **ESB goal**: >30 days (reduced protocol overhead)

**Pass Criteria**: Battery life ≥2x BLE split peripheral baseline.

---

## Maintenance and Update Strategy

### Monitoring ESB Module

**Action Items**:
1. Star/watch badjeff/zmk-feature-split-esb on GitHub
2. Subscribe to repository notifications
3. Monthly check for new commits/releases
4. Review open issues for bug reports related to your hardware

**Trigger for Update**:
- Security fixes in ZMK core
- Bug fixes in ESB module affecting stability
- New features (e.g., improved latency, power management)

### ZMK Core Updates

**Process**:
1. Review ZMK CHANGELOG.md for breaking changes
2. Check ESB module compatibility with new ZMK version
3. Test update in isolated environment before production
4. Update west.yml with new ZMK revision
5. Run `west update` and rebuild all targets
6. Re-validate using Phase 1-3 protocols

**Cadence**: Quarterly for minor updates, immediate for security patches.

### Backup Configuration

**Critical Files to Version Control**:
- config/west.yml (module versions)
- config/build.yaml (build targets)
- config/corne.keymap (keymap)
- config/*.conf (all Kconfig)
- config/boards/shields/corne_dongle/* (dongle shield)

**Repository Setup**:
git init config
git remote add origin https://github.com/yourusername/corne-esb-zmk-config
git add .
git commit -m "Initial ESB dongle configuration"
git push -u origin main

**Tag stable releases**:
git tag -a v1.0-esb-baseline -m "Working BLE dongle baseline"
git tag -a v1.1-esb-transport -m "ESB transport enabled, tested"
git push --tags

---

## Conclusion and Recommendations

### Recommended Approach

1. **Start with BLE dongle (Phase 1)**: Establishes working baseline, validates hardware and shield configuration, de-risks the overall project.

2. **Verify ESB module maintenance**: Before Phase 2, confirm badjeff/zmk-feature-split-esb has commits within 12 months and is compatible with ZMK v0.3.x.

3. **Incremental ESB integration (Phase 2)**: Only proceed if Phase 1 stable and ESB module maintained. Expect configuration challenges adapting from reference implementations.

4. **Measure and validate (Phase 3)**: Quantify latency and battery improvements. If ESB provides minimal benefit or introduces instability, revert to BLE dongle.

### Expected Outcomes

**Latency**:
- BLE dongle: ~7.5ms worst-case split transport + 1ms USB polling = ~8-9ms total
- ESB dongle: <1ms ESB transport + 1ms USB polling = ~2ms total
- **Improvement**: 4-5ms reduction (measurable in high-speed typing, gaming)

**Battery Life**:
- BLE peripheral baseline: ~14-21 days (110mAh battery, moderate use)
- Dongle configuration benefit: No Bluetooth host scanning, potential +30-50% improvement
- ESB transport benefit: Lower protocol overhead, potential +20-30% additional improvement
- **Target**: 30+ days per charge on peripherals

**Reliability**:
- BLE dongle: Proven architecture, stable
- ESB dongle: Community-maintained, may have edge cases, requires thorough testing

### Alternative Paths

If ESB module proves unmaintained or incompatible:

1. **BLE dongle production use**: Accept ~7-8ms latency, gain "no PC Bluetooth" and battery benefits
2. **Wired split option**: ZMK v0.3.0 introduced wired UART split support[17]. Consider TRRS cable between halves + one half as USB central for <1ms split latency, but loses wireless convenience
3. **Contribute to ESB module**: If module abandoned but code quality good, consider community fork/maintenance

### Success Criteria Summary

**Minimum Viable (BLE Dongle)**:
- ✅ No PC Bluetooth required
- ✅ Both halves functional through dongle
- ✅ Peripheral displays working
- ✅ Stable operation for daily use

**Target Success (ESB Dongle)**:
- ✅ All above, plus:
- ✅ Latency <2ms end-to-end
- ✅ Battery life >30 days per charge
- ✅ No disconnects or missed keypresses
- ✅ Maintained module with community support

---

## References

[1] ZMK Firmware. (2025). Releases. Retrieved from https://github.com/zmkfirmware/zmk/releases

[2] Nordic Semiconductor. (2022). Minimum ESB latency. Retrieved from https://devzone.nordicsemi.com/f/nordic-q-a/94577/minimum-esb-latency

[3] Electric UI. (2024). Benchmarking latency across common wireless links for embedded systems. Retrieved from https://electricui.com/blog/latency-comparison

[4] ZMK Firmware. (n.d.). Keyboard Dongle. Retrieved from https://zmk.dev/docs/development/hardware-integration/dongle

[5] ZMK Firmware. (n.d.). Split Keyboards. Retrieved from https://zmk.dev/docs/features/split-keyboards

[6] ZMK Firmware. (2024). Document how to setup a dongle · Issue #2373. Retrieved from https://github.com/zmkfirmware/zmk/issues/2373

[7] Reddit user. (2025). r/ErgoMechKeyboards - ZMK dongle set up. Retrieved from https://www.reddit.com/r/ErgoMechKeyboards/comments/1hst57u/zmk_dongle_set_up/

[8] Nordic Semiconductor. (2022). ESB timing measurements. Retrieved from https://devzone.nordicsemi.com/f/nordic-q-a/94577/minimum-esb-latency

[9] kmobs. (2025). Example config using ESB Split transport to greatly reduce latency on ZMK. Retrieved from https://www.reddit.com/r/MechanicalKeyboards/comments/1os3m1t/example_config_using_esb_split_transport_to/

[10] Reddit user petejohanson. (n.d.). Comment on ESB module. Retrieved from https://www.reddit.com/user/petejohanson/

[11] mctechnology17. (2024). zmk-dongle-display-view. Retrieved from https://github.com/mctechnology17/zmk-dongle-display-view

[12] ZMK Firmware. (n.d.). Split Configuration. Retrieved from https://zmk.dev/docs/config/split

[13] ZMK Firmware. (n.d.). Battery Level. Retrieved from https://zmk.dev/docs/features/battery

[14] Kris Cables. (2025). Nice!Nano and ZMK firmware FAQ. Retrieved from https://kriscables.com/nicenano-faq/

[15] englmaxi. (2024). zmk-dongle-display. Retrieved from https://github.com/englmaxi/zmk-dongle-display

[16] ZMK Firmware. (2022). Boot protocol not enabled · Issue #632. Retrieved from https://github.com/zmkfirmware/zmk/issues/632

[17] ZMK Firmware. (2025). Contributor Sync February 2025. Retrieved from https://zmk.dev/blog/2025/03/20/contributor-sync-1