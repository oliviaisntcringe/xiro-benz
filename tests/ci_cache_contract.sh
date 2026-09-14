#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
workflow="$repo_root/.github/workflows/windows-build.yml"

grep -Fq "id: build-cache" "$workflow"
grep -Fq 'id: toolset' "$workflow"
grep -Fq 'platform_toolset=$platformToolset' "$workflow"
grep -Fq 'velocity-cs2/bin/${{ matrix.configuration }}' "$workflow"
grep -Fq "steps.build-cache.outputs.cache-hit != 'true'" "$workflow"
grep -Fq "hashFiles('velocity-cs2/project/**'" "$workflow"

echo "ci cache contract: PASS"
