// SPDX-License-Identifier: MIT
// Copyright 2026, Nick Kennedy
#pragma once
#include <stdint.h>
// Opt-in through KL_GUEST_THREAD_TRACKING=1. Call enter before guest execution,
// body_done on return/exit/cancellation, and hold/drop around guest destructor
// domains. Unknown IDs are never inferred alive from retained kernel threads.
void kl_guest_thread_enter(void);
void kl_guest_thread_body_done(void);
void kl_guest_thread_hold(void);
void kl_guest_thread_drop(void);
int kl_guest_thread_tracking(void);
int kl_guest_thread_alive(uint64_t tid);
