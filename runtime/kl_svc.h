// SPDX-License-Identifier: MIT
// Copyright 2026, Nick Kennedy
#pragma once
#include <stdint.h>
// Fail-closed private-memory ABI translation. This does NOT emulate a syscall.
#define KL_SVC_TRAP_WORD (0xd4200000u | (0x4b4cu << 5))
static inline int kl_svc_to_trap(uint32_t *word) {
    if ((*word & 0xffe0001fu) != 0xd4000001u) return 0;
    *word = KL_SVC_TRAP_WORD;
    return 1;
}
