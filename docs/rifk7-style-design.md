# RIFK7 Retro Overlay - Visual Design Reference

Status: reference specification only. This document does not prescribe changes to the existing source tree.

## Reference source

- Repository: https://github.com/CSGOLeaks/Rifk7
- Primary screenshot: https://i.imgur.com/O8B4cs3.jpeg
- Context: a CS:GO overlay from approximately August 2019.

The screenshot is the source of truth for the visual direction. Public forks appear to preserve the same snapshot, so they are useful for provenance but not separate visual variants.

## Direction

Build a dense, technical in-game overlay with a retro underground-computing tone and a restrained LulzSec-adjacent signal. It should feel like a compact tool assembled for operators, not a polished SaaS dashboard.

The LulzSec cue is indirect: terminal-like typography, anonymous utility, black-and-green contrast, terse labels, and a small green/purple brand mark. Do not copy LulzSec logos, slogans, masks, or other protected identity elements.

### Principles

1. Keep the game visible. The overlay floats above the game and occupies only the working area.
2. Prefer information density over decoration.
3. Use borders, spacing, and color to establish hierarchy; avoid gradients and heavy shadows.
4. Keep geometry rectangular and deliberate, with small chamfers only where they help the retro hardware feel.
5. Make every accent functional: active state, section label, focus, or status.

## Palette

| Token | Hex | Use |
| --- | --- | --- |
| `canvas` | `#171717` | darkest overlay surface |
| `panel` | `#202020` | primary window fill |
| `panel-raised` | `#292929` | header and focused controls |
| `border` | `#606060` | default 1 px outlines |
| `border-strong` | `#858585` | selected tab and outer frame |
| `text` | `#E4E4E4` | primary labels and values |
| `text-muted` | `#909090` | secondary metadata |
| `accent-green` | `#77C84A` | active state, section titles, separators |
| `accent-green-dim` | `#3F6F35` | low-emphasis green rules |
| `accent-purple` | `#8D4AB0` | limited brand highlight |
| `focus` | `#B4F76A` | keyboard or pointer focus |

Green is the dominant accent. Purple is reserved for the wordmark and rare status details; it must never become a second full UI theme.

## Typography

- Primary family: a narrow bitmap or technical monospace face.
- Suggested stack: `ProFont, IBM Plex Mono, Cascadia Mono, monospace`.
- Body and control labels: 12-13 px at 1920x1080.
- Section labels: 11-12 px, green, sentence case or short title case.
- Top navigation: 13-14 px, high contrast.
- Logo: custom outlined wordmark asset, not ordinary text.
- Line height: approximately 1.1-1.2.
- Letter spacing: `0`.
- Avoid large display headings, rounded modern UI fonts, and italic cyberpunk faces.

## Composition

The reference uses a game menu as the background and a compact overlay near the upper center/right.

- Target overlay width at 1920x1080: approximately 620 px.
- Header height: approximately 110 px.
- Settings surface height: approximately 420 px.
- Gap between header and settings surface: 10-14 px.
- Settings surface: two columns and four compact rows of fieldsets.
- Outer frame and fieldsets: 1 px borders.
- Internal spacing: 8-12 px; keep labels and values aligned to a fixed grid.
- Preserve visible game context around the panel; do not use a full-screen opaque backdrop.
- At smaller viewports, scale the whole surface down or stack the two columns while keeping controls at a stable minimum size.

### Navigation

The top rail contains five tabs:

`Visuals` / `Aiming` / `Misc` / `Skins` / `Config`

Each tab is a flat rectangular cell with a subtle diagonal chamfer at the outer corners. The selected tab uses green text or a green lower rule; inactive tabs remain light gray. No pill shapes.

### Header and wordmark

Use a black raised header with a centered wireframe `RIFK7` wordmark. The mark is primarily green with a small purple terminal accent. A thin green technical divider and tiny build metadata can sit below it. Metadata is optional and should stay quiet.

### Settings fieldsets

Use compact fieldsets with the title sitting on or cutting through the top border. A representative two-column matrix is:

- Left: `Legit`, `Auto Sniper`, `AWP`, `Heavy`.
- Right: `Rage`, `Scout`, `Pistol`, `Other`.

Rows should read like a settings console: label on the left, control on the right, consistent baselines, no explanatory paragraphs.

## Controls

- Checkbox: approximately 10x10 px, square outline; green fill or check only when enabled.
- Numeric stepper: fixed value cell with a left arrow cell and a right arrow cell.
- Arrow cells: small square hit areas with visible `<` and `>` glyphs.
- Dropdown: flat framed value cell, with a compact chevron or arrow affordance.
- Focus: green border or green text; do not add a glow halo.
- Disabled: muted text and border, reduced contrast, still legible.

## Interaction and motion

- Hover and focus transitions: color or opacity only, 80-120 ms.
- Tab switching should update the panel in place without a large animation.
- Keyboard focus must be visible through the green focus token.
- Avoid bounce, parallax, blur transitions, particle effects, and scanline overlays that reduce readability.

## LulzSec-adjacent details

Use these as small signals rather than decoration:

- terse operator labels and abbreviated values;
- a tiny build/date signature in the header;
- green technical rules or divider lines;
- an outlined, slightly irregular wordmark;
- one purple accent that reads as a terminal artifact.

Avoid literal skulls, masks, pirate flags, copied slogans, noisy glitch filters, and neon gradients. The mood should be underground and dry, not theatrical.

## Do / do not

### Do

- Keep the palette mostly black, gray, and white with green as the working accent.
- Align values and controls into repeatable columns.
- Use 1 px rules and small spacing increments.
- Let the game scene remain visible behind the overlay.
- Treat the logo as the only strongly branded visual.

### Do not

- Use rounded cards, glassmorphism, drop-shadow-heavy panels, or a full-screen modal shell.
- Fill every surface with green or purple.
- Replace the technical font with a wide geometric sans-serif.
- Add decorative copy or marketing language.
- Make the logo a direct copy of another group's identity.

## Acceptance checklist

- [ ] The first impression reads as a retro 2019 in-game utility overlay.
- [ ] The game background remains visible around and behind the working surface.
- [ ] Navigation, section titles, and active controls use green consistently.
- [ ] Purple appears only as a restrained brand accent.
- [ ] All controls fit in a dense two-column grid without overlap.
- [ ] Borders, typography, and spacing remain crisp at the target resolution.
- [ ] No literal LulzSec branding or copied identity assets are used.

