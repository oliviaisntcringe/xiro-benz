#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
hook="$root/velocity-cs2/project/core/hooks/impl/cheat.cpp"

grep -q 'std::try_to_lock' "$hook"
grep -q 'if ( !backtrack_lock.owns_lock( ) || !onshot_lock.owns_lock( ) )' "$hook"
grep -q 'g_scene_object_mutation' "$hook"
grep -q 'scene_object_mutation_scope' "$root/velocity-cs2/project/core/features/esp/player/player.chams.cpp"

echo "generate primitives contract: PASS"
