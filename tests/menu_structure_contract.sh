#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
project="$repo_root/velocity-cs2/velocity-cs2.vcxproj"
xui_dir="$repo_root/velocity-cs2/project/core/rendering/impl/menu_xui"
imgui_dir="$repo_root/velocity-cs2/project/core/rendering/impl/imgui_menu"

test -d "$xui_dir"
test ! -e "$repo_root/velocity-cs2/project/core/rendering/impl/menu"

for entry in \
	"visuals:draw_visuals_tab" \
	"misc:draw_misc_tab" \
	"skins:draw_skins_tab" \
	"legit:draw_legit_tab" \
	"rage:draw_rage_tab" \
	"personal:draw_personal_tab" \
	"config:draw_config_tab"; do
	tab="${entry%%:*}"
	method="${entry#*:}"
	file="$imgui_dir/imgui_menu.$tab.cpp"
	test -f "$file"
	grep -Fq "imgui_menu::$method" "$file"
	grep -Fq "project\core\rendering\impl\imgui_menu\imgui_menu.$tab.cpp" "$project"
done

grep -Fq 'project\core\rendering\impl\imgui_menu.cpp' "$project"
grep -Fq 'project\core\rendering\impl\menu_xui\menu.rifk.cpp' "$project"

echo "menu structure contract: PASS"
