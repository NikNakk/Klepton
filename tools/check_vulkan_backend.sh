#!/bin/sh
# SPDX-License-Identifier: MIT
# Driver-boundary test only: no game, OpenXR service or guest binary translation.
set -eu
cd "$(dirname "$0")/.."
work=$(mktemp -d /private/tmp/klepton-vulkan-check.XXXXXX)
trap 'rm -rf "$work"' EXIT
prefix=${VULKAN_SDK:-/opt/homebrew}
clang -dynamiclib -O1 -I"$prefix/include" -Iruntime -Iruntime/gfx -Iruntime/guest \
    runtime/gfx/kl_vulkan.c runtime/kl_env.c -Wl,-undefined,dynamic_lookup -lz -o "$work/libklvulkan.dylib"
clang -I"$prefix/include" -Wl,-export_dynamic tests/t_vulkan_backend.c -o "$work/test"
"$work/test" "$work/libklvulkan.dylib"
