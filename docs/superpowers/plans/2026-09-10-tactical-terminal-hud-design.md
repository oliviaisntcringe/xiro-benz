# Tactical-Terminal HUD Design Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the selected in-game HUD surfaces with a single tactical-terminal visual system inspired by the approved RIFK7/XIRO.BENZ reference, while preserving existing settings, data collection, and runtime behavior.

**Architecture:** Add a small rendering-only style layer under `core/rendering` that separates platform-independent palette/layout contracts from Windows/D3D draw helpers. Migrate `widgets`, bomb/spectator overlays, and misc HUD drawing to those primitives; do not alter menu rendering, player ESP, or item labels in this phase.

**Tech Stack:** C++ latest standard, existing `xdraw::draw_list`, `xdraw::color`, `xui` input/settings, existing Pixel7/Inter font assets, Visual Studio `.vcxproj`, GitHub Actions Windows matrix build.

**Spec:** `docs/rifk7-style-design.md`

## Global Constraints

- Use the palette from the spec: canvas `#171717`, panel `#202020`, panel-raised `#292929`, border `#606060`, border-strong `#858585`, text `#E4E4E4`, text-muted `#909090`, accent-green `#77C84A`, accent-green-dim `#3F6F35`, accent-purple `#8D4AB0`, focus `#B4F76A`.
- Use square or near-square geometry; no rounded cards, pills, glassmorphism, blur, large shadows, or decorative gradients in migrated HUD surfaces.
- Use Pixel7/technical monospace for labels and values; preserve stable fixed dimensions so text cannot resize the layout.
- Preserve existing feature settings, calculations, enable/disable behavior, hotkeys, and draw ordering.
- Do not modify `core/rendering/impl/menu/*` in this phase.
- Do not modify `core/features/esp/player/player.overlay.cpp` or `core/features/esp/item/item.overlay.cpp` in this phase.
- The project remains Windows-only; verification must include the GitHub Actions matrix build.

---

### Task 1: Add the retro style primitives and host-side contract test

**Files:**
- Create: `velocity-cs2/project/core/rendering/retro_style_tokens.hpp`
- Create: `velocity-cs2/project/core/rendering/retro_style.hpp`
- Create: `tests/retro_style_contract.cpp`
- Modify: `velocity-cs2/velocity-cs2.vcxproj` only if the style header requires explicit project inclusion.

**Interfaces:**
- Produces `rendering::retro::palette` color constants and `rendering::retro::layout_rect`/`centered_rect` from the platform-independent `retro_style_tokens.hpp`.
- Produces `rendering::retro::draw_frame`, `rendering::retro::draw_rule`, and `rendering::retro::push_font` from the Windows renderer-only `retro_style.hpp`.
- Consumes existing `xdraw::draw_list`, `xdraw::color`, and `rendering::g_fonts` declarations without changing their ownership.

- [ ] **Step 1: Write the failing contract test**

Create a platform-independent test that includes only the pure palette/layout portion of the header and asserts the approved values and geometry:

```cpp
#include "../velocity-cs2/project/core/rendering/retro_style_tokens.hpp"
#include <cassert>

int main() {
    using namespace rendering::retro;
    assert(palette::accent_green.r == 0x77);
    assert(palette::accent_green.g == 0xC8);
    assert(palette::accent_green.b == 0x4A);
    const auto r = centered_rect(1920.0f, 620.0f, 110.0f, 420.0f);
    assert(r.x == 650.0f && r.y == 110.0f && r.w == 620.0f && r.h == 420.0f);
    assert(palette::corner_radius == 0.0f);
}
```

- [ ] **Step 2: Run the test to verify it fails**

Run: `c++ -std=c++20 -I. tests/retro_style_contract.cpp -o /tmp/retro_style_contract`

Expected: FAIL because `retro_style_tokens.hpp` and its contract symbols do not yet exist.

- [ ] **Step 3: Implement the minimal style contract and drawing helpers**

Define the exact colors above, a platform-independent `retro::color` with byte channels, a `layout_rect` with `float x/y/w/h`, `centered_rect(viewport_w, panel_w, y, panel_h)`, and `palette::corner_radius = 0.0f` in `retro_style_tokens.hpp`. Define the xdraw-backed draw helpers in `retro_style.hpp`, which includes the token header and existing xdraw/rendering declarations; keep the token header free of Windows/D3D dependencies.

`draw_frame` must draw a flat fill plus a 1 px outline through `draw_list.rect_filled` and `draw_list.rect` with `xdraw::corner_radius{0.0f}`. `draw_rule` must call `draw_list.line` with the requested token color and 1 px thickness.

- [ ] **Step 4: Run the contract test to verify it passes**

Run: `c++ -std=c++20 -I. tests/retro_style_contract.cpp -o /tmp/retro_style_contract && /tmp/retro_style_contract`

Expected: exit code 0 with no output.

- [ ] **Step 5: Commit the isolated style layer**

```bash
git add velocity-cs2/project/core/rendering/retro_style_tokens.hpp velocity-cs2/project/core/rendering/retro_style.hpp tests/retro_style_contract.cpp velocity-cs2/velocity-cs2.vcxproj
git commit -m "feat: add tactical terminal style primitives"
```

### Task 2: Rebuild watermark and keybind widgets

**Files:**
- Modify: `velocity-cs2/project/core/rendering/impl/widgets.cpp`
- Read-only reference: `velocity-cs2/project/core/settings.hpp` watermark/widget settings and existing bind registry.

**Interfaces:**
- Consumes `rendering::retro` from Task 1 and existing `settings::g_misc.m_watermark`, `xui::binds::all()`, and combat state.
- Produces the same `widgets::draw()` behavior and the same visible telemetry values, but rendered as framed terminal blocks.

- [ ] **Step 1: Add a failing source-contract check**

Add a small shell assertion used during development:

```bash
! rg -n 'rect_filled_blurred|corner_radius\{ r \}|corner_radius\{ inner_r \}' velocity-cs2/project/core/rendering/impl/widgets.cpp
```

Run it before editing and confirm it fails because the old widget renderer still uses blur and rounded pills.

- [ ] **Step 2: Replace the watermark composition**

Keep the existing FPS, ping, map, tick, velocity, time, and user calculations. Replace the dynamic pill strip with one fixed-height framed panel at the top-right. Use a terminal header line (`xiro.benz // telemetry`) and fixed-width stat columns with green values, muted units, and 1 px green-dim rules. Use the Pixel7 normal font while drawing values; do not add blur or rounded geometry.

- [ ] **Step 3: Replace the keybind composition**

Keep the existing filtering and fade/spring state. Render a single left-side framed block with a green `> keybinds` header, one row per active bind, a fixed key/value column, and no icon tile or pill. Toggle binds use muted text; held binds use accent-green; numeric override values use accent-green with a muted unit.

- [ ] **Step 4: Run source-contract checks**

Run:

```bash
! rg -n 'rect_filled_blurred|corner_radius\{ r \}|corner_radius\{ inner_r \}' velocity-cs2/project/core/rendering/impl/widgets.cpp
rg -n 'retro::|Pixel7|keybinds|telemetry' velocity-cs2/project/core/rendering/impl/widgets.cpp
```

Expected: no old blur/rounded calls and positive matches for the new primitives and terminal labels.

- [ ] **Step 5: Commit widget migration**

```bash
git add velocity-cs2/project/core/rendering/impl/widgets.cpp
git commit -m "feat: rebuild widgets in tactical terminal style"
```

### Task 3: Rebuild bomb and spectator overlays

**Files:**
- Modify: `velocity-cs2/project/core/features/esp/other/other.overlay.cpp`

**Interfaces:**
- Preserve `overlay::on_render`, bomb damage/timer calculations, spectator filtering, avatars, and visibility rules.
- Consume the Task 1 style primitives and use the existing Pixel7 font for labels.

- [ ] **Step 1: Add a failing source-contract check**

Run:

```bash
! rg -n 'rect_filled_blurred|corner_radius\{ r \}|corner_radius\{ inner_r \}' velocity-cs2/project/core/features/esp/other/other.overlay.cpp
```

Expected: FAIL before migration because both panels currently use blur and rounded pills.

- [ ] **Step 2: Replace bomb rendering**

Retain all data reads and damage math. Render a compact centered frame with a technical header (`> bomb status`) and fixed rows for `SITE`, `TIME`, `DAMAGE`, and `DEFUSE`. Use accent-green for normal status, amber/red only for imminent explosion or lethal damage, and accent-purple only for a secondary status marker. Remove the old pill width calculations, blur, and rounded rectangles.

- [ ] **Step 3: Replace spectator rendering**

Retain spectator collection and avatar lookup. Render a left-aligned framed list with `> spectators` header, one fixed-height row per name, muted avatar treatment, and a green active marker. Keep the list compact and readable over the game scene; do not use cards nested inside the panel.

- [ ] **Step 4: Run source-contract checks**

Run:

```bash
! rg -n 'rect_filled_blurred|corner_radius\{ r \}|corner_radius\{ inner_r \}' velocity-cs2/project/core/features/esp/other/other.overlay.cpp
rg -n 'bomb status|spectators|retro::|draw_frame' velocity-cs2/project/core/features/esp/other/other.overlay.cpp
```

- [ ] **Step 5: Commit overlay migration**

```bash
git add velocity-cs2/project/core/features/esp/other/other.overlay.cpp
git commit -m "feat: rebuild bomb and spectator overlays"
```

### Task 4: Rebuild misc HUD primitives

**Files:**
- Modify: `velocity-cs2/project/core/features/misc/impl/hud.cpp`

**Interfaces:**
- Preserve `hud::on_render`, scope spread calculations, hat projection geometry, velocity smoothing/history, and all existing settings.
- Consume Task 1 palette/drawing helpers.

- [ ] **Step 1: Add a failing source-contract check**

Run:

```bash
rg -n 'get_glow|rect_filled_blurred|corner_radius' velocity-cs2/project/core/features/misc/impl/hud.cpp
```

Expected: matches exist before migration for scope/hat glow and velocity rounded panels.

- [ ] **Step 2: Rebuild crosshair and scope**

Keep enable checks and scope animation. Draw crisp 1 px/2 px terminal lines in accent-green, with an optional dim secondary line instead of glow. Preserve the spread gap and fade timing, but stop emitting glow-layer geometry.

- [ ] **Step 3: Rebuild hat**

Keep world projection and both hat shapes. Draw the projected rings/spokes with thin palette colors directly on the main draw list; remove the glow pass and use accent-purple only for secondary stitch/spoke details.

- [ ] **Step 4: Rebuild velocity counter/chart**

Keep smoothing/history and configuration. Replace the rounded blurred panel and filled gradient chart with a square framed telemetry block, green baseline/rule, green polyline, and flat low-alpha fill only if it remains legible. Keep fixed minimum dimensions from the existing settings clamps.

- [ ] **Step 5: Run source-contract checks**

Run:

```bash
! rg -n 'get_glow|rect_filled_blurred|corner_radius' velocity-cs2/project/core/features/misc/impl/hud.cpp
rg -n 'retro::|accent_green|accent_purple|velocity' velocity-cs2/project/core/features/misc/impl/hud.cpp
```

- [ ] **Step 6: Commit misc HUD migration**

```bash
git add velocity-cs2/project/core/features/misc/impl/hud.cpp
git commit -m "feat: rebuild misc hud in tactical terminal style"
```

### Task 5: CI verification and scope guard

**Files:**
- Modify: `.github/workflows/windows-build.yml` only if the build exposes a concrete issue.
- Test: `tests/retro_style_contract.cpp`.

**Interfaces:**
- No new runtime behavior; this task verifies all migrated surfaces compile together.

- [ ] **Step 1: Run the host-side contract test**

```bash
c++ -std=c++20 -I. tests/retro_style_contract.cpp -o /tmp/retro_style_contract && /tmp/retro_style_contract
```

- [ ] **Step 2: Run scope checks**

```bash
git diff --name-only origin/main...HEAD
```

Expected migrated files: `retro_style_tokens.hpp`, `retro_style.hpp`, `tests/retro_style_contract.cpp`, `widgets.cpp`, `other.overlay.cpp`, `hud.cpp`; no menu/player/item ESP files.

- [ ] **Step 3: Push and inspect both GitHub Actions jobs**

```bash
git push origin main
gh run list --limit 1 --json databaseId,status,conclusion
gh run watch <run-id> --exit-status
```

Expected: `Release | x64` and `Development | x64` both complete successfully, and artifact upload remains successful.
