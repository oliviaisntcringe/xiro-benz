#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
menu="$repo_root/velocity-cs2/project/core/rendering/impl/imgui_menu.cpp"

grep -Fq 'trida.benz' "$menu"
grep -Fq 'draw_left_rail' "$menu"
grep -Fq 'Rage' "$menu"
grep -Fq 'Legit' "$menu"
grep -Fq 'Player' "$menu"
grep -Fq 'Visuals' "$menu"
grep -Fq 'Misc' "$menu"
grep -Fq 'Skins' "$menu"
grep -Fq 'Person' "$menu"
grep -Fq 'Config' "$menu"
! grep -Fq '##xiro_top_tabs' "$menu"

echo "imgui navigation contract: PASS"
