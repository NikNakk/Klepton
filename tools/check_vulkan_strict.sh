#!/bin/sh
# SPDX-License-Identifier: MIT
set -eu
cd "$(dirname "$0")/.."
work=$(mktemp -d /private/tmp/klepton-vulkan-strict.XXXXXX)
trap 'rm -rf "$work"' EXIT
prefix=${VULKAN_SDK:-/opt/homebrew}
clang -dynamiclib -O1 -I"$prefix/include" -Iruntime -Iruntime/gfx -Iruntime/guest \
 runtime/gfx/kl_vulkan.c runtime/kl_env.c -Wl,-undefined,dynamic_lookup -lz -o "$work/libklvulkan.dylib"
clang -dynamiclib -I"$prefix/include" tests/t_vulkan_strict_driver.c -o "$work/driver.dylib"
clang -I"$prefix/include" -Wl,-export_dynamic tests/t_vulkan_strict.c -o "$work/test"
KL_VK_FOVEATION_TRACE=1 KL_VK_STRICT=1 KL_VULKAN_LIBRARY="$work/driver.dylib" "$work/test" "$work/libklvulkan.dylib" "$work/driver.dylib" >"$work/trace.log" 2>&1
cat "$work/trace.log"
python3 - "$work/trace.log" <<'CHECK'
from pathlib import Path
import sys
s=Path(sys.argv[1]).read_text()
assert 'device-request core word=4 value=1' in s  # geometryShader
assert 'VkPhysicalDeviceFragmentDensityMapFeaturesEXT payloadWord=0 value=1' in s
assert 'VkPhysicalDeviceFragmentDensityMapFeaturesEXT payloadWord=1 value=1' in s
assert 'strict vkCreateDevice driver result=-8' in s
assert 'strict vkCreateDevice extension enumeration result=-3' in s
print('PASS: exact feature payload and driver/enumeration failure diagnostics')
CHECK
