#!/bin/sh
# SPDX-License-Identifier: MIT
set -eu
cd "$(dirname "$0")/.."
work=$(mktemp -d /private/tmp/klepton-observe.XXXXXX)
trap 'rm -rf "$work"' EXIT
prefix=${VULKAN_SDK:-/opt/homebrew}
clang -dynamiclib -O1 -I"$prefix/include" -Iruntime -Iruntime/gfx -Iruntime/guest \
 runtime/gfx/kl_vulkan.c runtime/kl_env.c -Wl,-undefined,dynamic_lookup -lz -o "$work/libklvulkan.dylib"
clang -dynamiclib -I"$prefix/include" tests/t_vulkan_observe_driver.c -o "$work/driver.dylib"
clang -I"$prefix/include" -Wl,-export_dynamic tests/t_vulkan_observe.c -o "$work/test"
for observe in 0 1; do
 KL_VK_STRICT=1 KL_VK_FOVEATION_TRACE=1 KL_VK_FOVEATION_OBSERVE=$observe KL_VULKAN_LIBRARY="$work/driver.dylib" \
 "$work/test" "$work/libklvulkan.dylib" "$work/driver.dylib" "$observe" > "$work/trace-$observe.log" 2>&1
 cat "$work/trace-$observe.log"
done
python3 - "$work/trace-1.log" "$work/trace-0.log" <<'CHECK'
import sys
from pathlib import Path
trace=Path(sys.argv[1]).read_text();off=Path(sys.argv[2]).read_text()
assert 'offset-eye=1 x=-32 y=-8' in trace
assert 'offset-granularity=16x8' in trace
assert 'features map=0 dynamic=0 nonSubsampled=0' in trace
assert 'shader capability=5291' in trace and 'shader builtin=5292' in trace
assert 'fdm-observe shader' not in off
print('PASS: decoded FDM query answers, alias offsets and original SPIR-V metadata; observation disabled path remains native')
CHECK
