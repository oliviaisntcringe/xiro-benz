#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
menu="$repo_root/velocity-cs2/project/core/rendering/impl/imgui_menu.cpp"
widgets="$repo_root/velocity-cs2/project/core/rendering/impl/widgets.cpp"
menu_header="$repo_root/velocity-cs2/project/core/rendering/imgui_menu.hpp"
rendering_header="$repo_root/velocity-cs2/project/core/rendering/rendering.hpp"
context="$repo_root/velocity-cs2/project/core/rendering/impl/context.cpp"
hud="$repo_root/velocity-cs2/project/core/features/misc/impl/hud.cpp"

grep -Fq "draw_watermark" "$widgets"
grep -Fq "draw_keybinds" "$widgets"
grep -Fq "draw_spectators" "$menu"
grep -Fq "void draw_watermark" "$rendering_header"
grep -Fq "void draw_keybinds" "$rendering_header"
grep -Fq "void draw_spectators" "$menu_header"
! grep -Fq "g_widgets.draw" "$context"
! grep -Fq "add_spectators( draw_list )" "$repo_root/velocity-cs2/project/core/features/esp/other/other.overlay.cpp"
! grep -Fq "this->do_velocity" "$hud"

echo "imgui widgets contract: PASS"
