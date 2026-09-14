#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
bhop="$root/velocity-cs2/project/core/features/movement/impl/bunnyhop.cpp"

grep -q 'bool holding_duck' "$bhop"
grep -q 'const auto duck_amount' "$bhop"
grep -q 'if ( holding_duck && duck_amount > 0.0f )' "$bhop"

clear_line="$(grep -n 'cmd->buttons.value &= ~cstypes::command_buttons::in_jump;' "$bhop" | head -1 | cut -d: -f1)"
landing_line="$(grep -n 'const auto landing = predict_landing_fraction' "$bhop" | head -1 | cut -d: -f1)"
test "$clear_line" -lt "$landing_line"

echo "bunnyhop contract: PASS"
