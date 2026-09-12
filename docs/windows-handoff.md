# Windows Handoff

## Repository

- Remote: `https://github.com/oliviaisntcringe/xiro-benz.git`
- Branch: `feature/tactical-terminal-hud`
- Latest commit: `fa1e121 fix: use layout same line api`
- Main branch is not the active redesign branch.

## Open in Windows

```powershell
git clone --branch feature/tactical-terminal-hud --single-branch https://github.com/oliviaisntcringe/xiro-benz.git
cd xiro-benz
code .
```

If the repository is already cloned:

```powershell
git fetch origin
git switch feature/tactical-terminal-hud
git pull --ff-only
```

Open `velocity-cs2/velocity-cs2.slnx` or the Visual Studio project and build `Development | x64` or `Release | x64`.

## Current work

The branch contains the tactical-terminal HUD redesign, including retro style helpers, widgets, bomb/spectator overlays, misc HUD, weapon telemetry, animated match header, terminal killfeed, weapon switch transition, and the `LOCAL` menu tab with Samurai/Bucket preview controls.

`LOCAL -> preview` currently uses a procedural 2D silhouette. The real `C_CSGO_PreviewPlayer` render-target preview is not connected yet.

## Chat continuity

Use the same GitHub account for VS Code and GitHub Copilot, and enable Settings Sync. The repository state will sync through GitHub. The exact Copilot conversation may not appear on another machine, so attach this file or tell Copilot: "Continue from docs/windows-handoff.md on feature/tactical-terminal-hud." 
