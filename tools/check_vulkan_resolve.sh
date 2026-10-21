#!/bin/sh
# Copyright 2026, Nick Kennedy
# SPDX-License-Identifier: MIT
set -eu
cd "$(dirname "$0")/.."
work=$(mktemp -d /private/tmp/klepton-resolve.XXXXXX)
trap 'rm -rf "$work"' EXIT
prefix=${VULKAN_SDK:-/opt/homebrew}
clang -dynamiclib -O1 -I"$prefix/include" -Iruntime -Iruntime/gfx -Iruntime/guest \
 runtime/gfx/kl_vulkan.c runtime/kl_env.c -Wl,-undefined,dynamic_lookup -lz -o "$work/shim.dylib"
clang -dynamiclib -I"$prefix/include" tests/t_vulkan_resolve_driver.c -o "$work/driver.dylib"
clang -I"$prefix/include" -Wl,-export_dynamic tests/t_vulkan_resolve.c -o "$work/test"
for resolve in 0 1; do
 KL_VK_STRICT=1 KL_VK_CUSTOM_RESOLVE_TRACE=$resolve KL_VULKAN_LIBRARY="$work/driver.dylib" \
 "$work/test" "$work/shim.dylib" "$work/driver.dylib" "$resolve" > "$work/$resolve.log" 2>&1
 cat "$work/$resolve.log"
done
KL_VK_STRICT=1 KL_VK_CUSTOM_RESOLVE_TRACE=1 KL_VK_FOVEATION_TRACE=1 KL_VK_FOVEATION_OBSERVE=1 \
 KL_VULKAN_LIBRARY="$work/driver.dylib" "$work/test" "$work/shim.dylib" "$work/driver.dylib" 1 > "$work/both.log" 2>&1
cat "$work/both.log"
python3 - "$work" <<'CHECK'
from pathlib import Path
import sys
p=Path(sys.argv[1])
off=(p/'0.log').read_text()
on=(p/'1.log').read_text()
both=(p/'both.log').read_text()
assert 'resolve-observe' not in off
for text in (on,both):
    assert text.count('resolve-observe custom CmdDraw ')==128
    assert 'budget-exhausted limit=128' in text
    for required in ('custom=1 colors=1','color=0 input-index=0','flags=0x80','mode=0x20',
                     'custom CmdDraw','native_target_changed','subpass-image id=1 array=0 ms=1 sampled=2',
                     'decoration target=3 kind=43 value=0','image-reads=1 image-fetches=0'):
        assert required in text,required
print('PASS: resolve metadata decoded; opt-out is native; resolve and FDM observers compose')
CHECK
