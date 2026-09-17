#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
menu="$repo_root/velocity-cs2/project/core/rendering/impl/imgui_menu.cpp"

grep -Fq 'auto* render_font = font ? font : ImGui::GetFont( );' "$menu"
grep -Fq 'static_cast< float >( std::sin( phase + static_cast< float >( i ) * 1.7f ) * 3.0f )' "$menu"
grep -Fq 'auto* glyph_font = this->m_mono_font ? this->m_mono_font : ImGui::GetFont( );' "$menu"

echo "imgui wordmark compile contract: PASS"
