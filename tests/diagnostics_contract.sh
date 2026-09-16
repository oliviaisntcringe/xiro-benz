#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
entry="$repo_root/velocity-cs2/project/entry.cpp"
diag="$repo_root/velocity-cs2/project/utilities/diag.hpp"

grep -q "install_exception_handlers" "$entry"
grep -q "ShellExecuteW" "$diag"
grep -q "g_crash_report_enabled" "$diag"
grep -q "g_log_sequence" "$diag"
grep -q "XI_BENZ_LOG_DIR" "$diag"
grep -q "phase=%s" "$diag"
grep -q "hook enter name=%s" "$diag"
grep -q '"\[pattern\] resolved' "$repo_root/velocity-cs2/project/utilities/memory/memory.cpp"
grep -q '"\[hook\] enabled' "$repo_root/velocity-cs2/project/utilities/hooking/impl/manager.cpp"

attach_line="$(grep -n "^[[:space:]]*install_exception_handlers( );" "$entry" | head -n1 | cut -d: -f1)"
thread_line="$(grep -n "CreateThread( nullptr, 0, init_thread" "$entry" | head -n1 | cut -d: -f1)"
test "$attach_line" -lt "$thread_line"

crt_line="$(grep -n "_CRT_INIT( module_handle, reason, reserved )" "$entry" | head -n1 | cut -d: -f1)"
test "$crt_line" -lt "$attach_line"

echo "diagnostics contract: PASS"
