// SPDX-License-Identifier: MIT
// Copyright 2026, Nick Kennedy
#include "kl_guest_threads.h"
#include "kl_env.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct entry {
    struct entry *next;
    uint64_t tid;
    unsigned holds;
    int body_done;
} entry;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_once_t once = PTHREAD_ONCE_INIT;
static int enabled;
static entry *live;
static uint64_t self_id(void) {
    uint64_t tid = 0;
    if (pthread_threadid_np(NULL, &tid) || !tid) abort();
    return tid;
}
// Avoid Darwin C thread-local storage here: its storage can be torn down
// before pthread-key destructors, which still need this bookkeeping.
static entry *find(uint64_t tid) {
    for (entry *e = live; e; e = e->next) if (e->tid == tid) return e;
    return NULL;
}
static void configure(void) { enabled = kl_env_on("KL_GUEST_THREAD_TRACKING", 0); }
int kl_guest_thread_tracking(void) { pthread_once(&once, configure); return enabled; }

void kl_guest_thread_enter(void) {
    if (!kl_guest_thread_tracking()) return;
    uint64_t tid = self_id();
    pthread_mutex_lock(&mutex);
    if (!find(tid)) {
        entry *e = calloc(1, sizeof *e);
        if (!e) {
            fprintf(stderr, "[guest-threads] cannot register thread; refusing incomplete tracking\n");
            abort();
        }
        e->tid = tid; e->next = live; live = e;
    }
    pthread_mutex_unlock(&mutex);
}
// Called with mutex held; no tombstones or fixed thread-count ceiling.
static void finish(entry *e) {
    if (!e->body_done || e->holds) return;
    entry **p = &live;
    while (*p != e) p = &(*p)->next;
    *p = e->next;
    free(e);
}
void kl_guest_thread_body_done(void) {
    if (!kl_guest_thread_tracking()) return;
    uint64_t tid = self_id();
    pthread_mutex_lock(&mutex);
    entry *e = find(tid);
    if (e) { e->body_done = 1; finish(e); }
    pthread_mutex_unlock(&mutex);
}
void kl_guest_thread_hold(void) {
    if (!kl_guest_thread_tracking()) return;
    kl_guest_thread_enter();
    uint64_t tid = self_id();
    pthread_mutex_lock(&mutex); find(tid)->holds++; pthread_mutex_unlock(&mutex);
}
void kl_guest_thread_drop(void) {
    if (!kl_guest_thread_tracking()) return;
    uint64_t tid = self_id();
    pthread_mutex_lock(&mutex);
    entry *e = find(tid);
    if (!e || !e->holds) abort();
    e->holds--; finish(e);
    pthread_mutex_unlock(&mutex);
}
int kl_guest_thread_alive(uint64_t tid) {
    if (!kl_guest_thread_tracking() || !tid) return 0;
    pthread_mutex_lock(&mutex);
    int found = 0;
    for (entry *e = live; e; e = e->next) if (e->tid == tid) { found = 1; break; }
    pthread_mutex_unlock(&mutex);
    return found;
}
