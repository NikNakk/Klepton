# Experimental guest thread lifetime tracking

`KL_GUEST_THREAD_TRACKING=1` enables a process-local registry. Default is off.
The native Alyx host consumes it for `tkill`/`tgkill` signal-zero queries. This
is not a complete Linux threading or syscall implementation.

Managed pthreads enter before guest execution. A bottom cleanup handler marks
the body finished on return, pthread_exit or deferred cancellation, after nested
cleanup handlers. Guest pthread-key tables and ELF/C++ TLS blocks hold the entry
until their destructor domains have drained. Dead entries are removed; unknown
IDs are not inferred live from macOS kernel thread existence. gettid entry paths
also register the calling thread, including the process entry thread.

The registry uses a mutex and a list keyed by the current native thread ID. It
has no fixed thread ceiling or growing tombstone list. It does not store state
in C `_Thread_local` storage: Darwin can dismantle that storage before pthread
key destructors, which must still access the registry. Allocation failure aborts
rather than silently providing incomplete liveness information.

The diagnostic mutex-owner registry remains separate. Its trampoline cleanup now
also unregisters cancelled threads. TLS key allocation no longer reads the
uninitialized output key when choosing a destructor.

Tests in VeRantyx `NativeARM/host/tests/guest_threads.c` link the real runtime
and host shims, using host-owned callbacks. They exercise normal return,
pthread_exit, deferred cancellation, C++ TLS + pthread-key destructors, repeated
key destructors, nested/cross-domain destructor registration, detach,
interposed syscall(exit), signal-zero tkill/tgkill,
unknown IDs and sequential fast return/exit cycles. No ELF/game is loaded.

Limits before gameplay:

- Only managed guest entry/teardown paths are covered. Host-created callbacks
  that enter guest code need explicit entry/exit bracketing. Native gettid-only
  entries without a managed exit can remain conservatively live.
- Tests use host-owned C callbacks. Guest glibc cancellation/unwind/cleanup ABI,
  asynchronous cancellation and fork are not validated.
- Native IDs currently share the existing gettid ABI, including its int-width
  limitation. No virtual Linux TID namespace is implemented.
- Queries define completion of managed guest work/destructors, not the final
  kernel teardown instruction. No raw clone, clear_child_tid or robust-futex
  exit semantics are added.
- The host supports signal-zero queries only for its own process in tracking
  mode. Other tgkill process targets return explicit ENOSYS. Nonzero-signal
  translation remains a separate audit.
- Host interposed syscall(exit) follows pthread_exit when tracking is enabled;
  inline raw SVC execution/translation is not addressed by that change.

Keep the constrained host's guest-execution gate until the remaining syscall,
Vulkan and Lua lifecycle audits pass. Passing these fixtures does not establish
that the legacy video text patch is safe to retire for actual Alyx playback.
