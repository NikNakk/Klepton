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
KL_VK_STRICT=1 KL_VULKAN_LIBRARY="$work/driver.dylib" "$work/test" "$work/libklvulkan.dylib" "$work/driver.dylib"
