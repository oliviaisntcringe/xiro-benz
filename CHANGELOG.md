# Changelog

## 2026-09-12

- Redesigned the watermark as a compact XI/RO.BENZ status rail with a live marker and left accent rail.
- Added a one-shot slot-style brand animation that assembles `xiro.benz` character by character.
- Extended widget scaling up to 2x for high-resolution displays and selected the large Pixel7 tier on 4K-style viewports.
- Added resolution-aware HUD scaling based on a 1920x1080 reference viewport.
- Reduced watermark height and moved telemetry values upward inside the panel.
- Scaled watermark and keybind geometry with the viewport.
- Added Windows handoff documentation in `docs/windows-handoff.md`.

## 2026-09-11

- Added the tactical-terminal retro style primitives.
- Rebuilt watermark, keybinds, bomb, spectator, and misc HUD surfaces.
- Added weapon telemetry, animated match header, terminal killfeed, and weapon switch transition.
- Added the `LOCAL` menu tab with preview, presets, and layers sections.
- Fixed Windows namespace and menu API compile issues.
