#!/bin/sh
# SPDX-License-Identifier: MIT
set -eu
cd "$(dirname "$0")/.."
work=$(mktemp -d /private/tmp/klepton-vulkan-real.XXXXXX)
trap 'rm -rf "$work"' EXIT
prefix=${VULKAN_SDK:-/opt/homebrew}
clang -dynamiclib -O1 -I"$prefix/include" -Iruntime -Iruntime/gfx -Iruntime/guest \
 runtime/gfx/kl_vulkan.c runtime/kl_env.c -Wl,-undefined,dynamic_lookup -lz -o "$work/libklvulkan.dylib"
clang -I"$prefix/include" -Wl,-export_dynamic tests/t_vulkan_strict_real.c -o "$work/test"
KL_VK_STRICT=1 "$work/test" "$work/libklvulkan.dylib"
