# RISC-V64 NTDLL thread regression tests

The RISC-V64 build runs the normal NTDLL thread test groups, followed by
native RISC-V64 smoke, direct thread exit, RTL thread exit, suspended-startup,
and register-context regression tests.  No environment variable is required
to select or enable the complete suite.

From the Wine build directory:

```sh
timeout -k 3s 120s env WINEDEBUG=-all WINE_RISCV_THREAD_DIAG=1 \
    ./wine dlls/ntdll/tests/ntdll_test.exe.so thread \
    > /tmp/rv64-thread-full.log 2>&1
printf 'exit status: %s\n' "$?"
grep -E 'RV64-THREAD-TEST|tests executed|failures|skipped' /tmp/rv64-thread-full.log
```

`WINE_RISCV_THREAD_DIAG=1` enables stderr checkpoints; it does **not**
change test selection.  To trace the RISC-V64 termination and loader cleanup,
use `WINEDEBUG=+thread,+seh` and inspect the `RV64-EXIT`,
`RV64-SHUTDOWN`, and `RV64-FLS` messages.

The RISC-V64 tests exercise:

- `smoke`: CreateThread, worker return through BaseThreadInitThunk, wait,
  and the expected 0x1234abcd exit code.
- `directexit`: worker calls NtTerminateThread on itself, without entering
  RtlExitUserThread or loader shutdown.
- `rtlexit`: worker calls RtlExitUserThread directly.
- `startup`: suspended creation, resume, and successful completion.
- `context`: x0, x2/SP, s2, fs0, FCSR, and native gp/tp preservation
  across context updates and resumption.

An unexpected return from either thread-termination function is a test
failure, rather than a diagnostic message followed by false success.

On Linux RISC-V64, the Unix termination path currently uses the native
`exit` syscall to end a single task after Wine server cleanup, because
glibc `pthread_exit()` forces an unwind across the port's hand-written
Windows-to-Unix syscall stack switch.  This is a targeted workaround, not
a complete libc-unwinder integration: pthread TLS destructors and other
libc cleanup callbacks do not run on this path.  The RISC-V native
thread-data registry is unregistered explicitly.  A long-term solution
should add unwinding-safe stack-switch integration and restore normal
pthread exit semantics.

An exit code of 124 from `timeout` means the suite did not complete.
Status 139 indicates SIGSEGV, and status 134 indicates SIGABRT; none
indicates a completed passing test suite.  Source changes must be built
and tested on an actual RV64 Linux host.

## RISC-V64 register ABI checks during DLL thread detach

The RV64 DLL entry-point trampoline verifies callee-saved registers across
calls to `DllMain` and TLS callbacks, and restores the original values
before returning to the loader.  The syscall and Unix-call dispatchers
likewise verify their saved state before returning to the Windows stack.
A mismatch produces `RV64-ABI` errors without requiring a debugger.

The hexadecimal mismatch mask uses these bits:

- `0..11`: `s0..s11` respectively.
- `12..23`: `fs0..fs11` respectively.
- `24`: `gp` changed.
- `25`: `tp` changed.
- `26`: DLL wrapper stack pointer changed or saved Windows stack pointer
  was not properly aligned (dispatcher check).
- `27`: DLL wrapper return address changed or the saved syscall return
  address was null/misaligned.

A missing `RV64-ABI` mismatch error does not prove the registers are
correct if a callback crashes before returning to the verifier.  During
`DLL_THREAD_DETACH`, `RV64-ABI DLL pre-call` records `s0/sp/gp/tp/ra`
on the calling side; the first eight SIGSEGVs per process also emit a
signal-safe `RV64-ABI fault` line showing PID/TID, `s0-s11`, argument
and temporary registers, and `sp/gp/tp/ra`
from the kernel's interrupted ucontext.  These two snapshots can be
compared even if the callback never returns.  Note that `ra` normally
changes during nested calls, and `s0` is not required to contain a
frame pointer at every instruction.  The signal tests validate signal
handling separately from callee-saved ABI conformance.

A focused test without GDB:

```sh
timeout -k 3s 120s env WINEDEBUG=+thread,+seh WINE_RISCV_THREAD_DIAG=1 \
    ./wine dlls/ntdll/tests/ntdll_test.exe.so thread \
    > /tmp/rv64-thread-abi.log 2>&1
printf 'exit=%s\n' "$?"
grep -E 'RV64-ABI|RV64-DLL|RV64-SHUTDOWN|RV64-THREAD-TEST|err:' \
    /tmp/rv64-thread-abi.log | head -160
```


## Native glibc TLS teardown fault: current investigation (2026-11-25)

**This is not yet established to be a glibc bug.** GDB caught a fault in
`free()` while the ELF dynamic linker was walking a native TLS/DTV cleanup
array. The pointer passed to `free()` was `0x133c`, and the first metadata
read, `ld a5,-8(a0)`, faulted at `0x1334`. The containing executable
mappings identified glibc `libc.so.6` for the fault PC and
`ld-linux-riscv64-lp64d.so.1` for the return address. The suspected
`_dl_deallocate_tls` call chain is **consistent with** the loop in glibc
that frees DTV `pointer.to_free` entries; the initiating Wine caller is
not yet definitively established.

Reproduction during `ntdll_test.exe.so thread`:
- Normal `WINEDEBUG=-all` test runs reach the `hidden thread enter`
  checkpoint but exit with code 1 after the SIGSEGV.
- The original heavy `WINEDEBUG=+thread,+seh` runs sometimes hung
  (timeout 124); debug traffic changes timing and should not be confused
  with the native TLS invalid-pointer fault.
- Setting `GLIBC_TUNABLES=glibc.malloc.tcache_count=0` did not solve the
  earlier allocator failure. A disabled tcache does not validate glibc
  internal TLS pointers and is not an appropriate workaround.
- The native `ThreadAmILastThread` short-circuit and last-thread server
  bypass did not resolve the root cause; the last-thread query has been
  restored to upstream Wine behavior.
- The RISC-V-only thread-name query/logging shortcut was removed and
  upstream `ThreadNameInformation` handling restored. The temporary
  signal-mask probes were removed. The one-time malloc-tcache loader
  re-exec was removed.

### Experimental RISC-V64 native TLS quarantine

The only new bypass is `WINE_RISCV_SKIP_PTHREAD_JOIN=1`, enabled explicitly
in the environment of the Wine process. On Linux RISC-V64, when
`exit_thread()` would normally join a **previously retired** native
pthread, the opt-in path skips `pthread_join()` **and** skips
`virtual_free_thread_data()` for that pthread. It prints
`RV64-TLS-QUARANTINE skipped pthread_join; retaining native TLS and Wine thread_data`
to stderr.

The normal implementation still performs the standard
`pthread_join()` and `virtual_free_thread_data()`; other
architectures are unchanged. The quarantine path does not call
`pthread_detach()`, because that may release the same glibc TLS
resources we are trying to preserve for this experiment. No
interception of `free()` is performed: glibc's internal allocator calls
must not be silently suppressed.

**WARNING: This is strictly a short-lived diagnosis.** Joinable pthread
resources, native TLS, Wine thread stacks, and thread_data are deliberately
not reclaimed. Repeated thread creation can exhaust memory and thread
resources, especially on an Orange Pi with limited RAM. Do not use it in
services, production applications, unattended workloads, or long stress
tests. It does not synchronize with a terminating thread, and can only
establish whether avoiding the suspected join/reclamation phase changes
the observed fault. It does not fix TLS corruption.

First rebuild and run a baseline without the switch:

```sh
git pull --ff-only origin risc-v
make -j4

timeout -k 3s 120s env -u WINE_RISCV_SKIP_PTHREAD_JOIN \
    -u WINE_RISCV_LASTTHREAD_NO_SERVER -u WINE_RISCV_EXIT_DIAG \
    WINEDEBUG=-all WINE_RISCV_THREAD_DIAG=1 \
    ./wine dlls/ntdll/tests/ntdll_test.exe.so thread \
    > /tmp/rv64-thread-baseline.log 2>&1
printf 'baseline exit=%s\n' "$?"
grep -E 'RV64-ABI fault|RV64-THREAD-TEST|malloc\(\)|tests executed|failures' \
    /tmp/rv64-thread-baseline.log | tail -120
```

Then run the **short-lived** quarantine experiment:

```sh
timeout -k 3s 45s env -u WINE_RISCV_LASTTHREAD_NO_SERVER \
    -u WINE_RISCV_EXIT_DIAG WINE_RISCV_SKIP_PTHREAD_JOIN=1 \
    WINEDEBUG=-all WINE_RISCV_THREAD_DIAG=1 \
    ./wine dlls/ntdll/tests/ntdll_test.exe.so thread \
    > /tmp/rv64-thread-quarantine.log 2>&1
printf 'quarantine exit=%s\n' "$?"
grep -E 'RV64-TLS-QUARANTINE|RV64-ABI fault|RV64-THREAD-TEST|malloc\(\)|tests executed|failures' \
    /tmp/rv64-thread-quarantine.log | tail -120
```

The result is meaningful **only if** the quarantine marker appears. If
the fault persists with no marker, the crashing `free()` may be reached
through another cleanup path, or may occur before this join site.
If the fault disappears and execution proceeds, the skipped join is a
stronger candidate for the *trigger*, not proof that glibc is at fault.
Test-suite failures still count as failures even if the process exits
without a signal.

### First post-cleanup A/B result (2026-11-25)

Both runs with `WINEDEBUG=-all WINE_RISCV_THREAD_DIAG=1` returned
**134 (SIGABRT)**, whether `WINE_RISCV_SKIP_PTHREAD_JOIN=1` was set or
not. Both reached `RV64-THREAD-TEST: hidden thread enter` and printed
`malloc(): unaligned tcache chunk detected` twice; both included
`RV64-ABI fault` reports from an early native thread. **Neither run
showed the `RV64-TLS-QUARANTINE skipped pthread_join` marker.**

These failures do **not** demonstrate a failure of the join-skip
mechanism *once reached*. Its guard requires an earlier non-null
`prev_data` with a non-zero `pthread_id`. The crash may occur before
that branch, including on the first worker exit or in a different Wine
process. We must identify the crashing PID/TID and call site rather than
assuming `pthread_join()` ran.

The post-cleanup commands did not set `GLIBC_TUNABLES`. Because the
previous re-exec workaround was removed, this also changes the glibc
allocator configuration relative to earlier tests. Compare baseline and
quarantine *under identical, explicitly pre-exported*
`GLIBC_TUNABLES=glibc.malloc.tcache_count=0` before drawing conclusions
about the earlier `free(0x133c)` fault.

```sh
timeout -k 3s 60s env -u WINE_RISCV_SKIP_PTHREAD_JOIN \
    GLIBC_TUNABLES=glibc.malloc.tcache_count=0 \
    WINEDEBUG=-all WINE_RISCV_THREAD_DIAG=1 \
    ./wine dlls/ntdll/tests/ntdll_test.exe.so thread \
    > /tmp/rv64-tcache0-baseline.log 2>&1
printf 'tcache0 baseline exit=%s\n' "$?"

timeout -k 3s 45s env GLIBC_TUNABLES=glibc.malloc.tcache_count=0 \
    WINE_RISCV_SKIP_PTHREAD_JOIN=1 \
    WINEDEBUG=-all WINE_RISCV_THREAD_DIAG=1 \
    ./wine dlls/ntdll/tests/ntdll_test.exe.so thread \
    > /tmp/rv64-tcache0-quarantine.log 2>&1
printf 'tcache0 quarantine exit=%s\n' "$?"

grep -E 'RV64-TLS-QUARANTINE|RV64-ABI fault|RV64-THREAD-TEST|malloc\(\)|tests executed|failures' \
    /tmp/rv64-tcache0-baseline.log /tmp/rv64-tcache0-quarantine.log | tail -120
```

This intentionally avoids recreating the removed loader-specific tcache
re-exec; glibc reads the tunable before startup in each directly launched
Wine process. If malloc's tcache error persists, investigate whether the
specific crashing process received and applied the setting.

### Native startup/exit location probes (after the failed tcache0 A/B)

The reproducible pair of runs on 2026-11-25 still returned 134,
even with `GLIBC_TUNABLES=glibc.malloc.tcache_count=0` pre-exported:
the hidden-thread test reached its entry checkpoint, both logs
showed `malloc(): unaligned tcache chunk detected`, and neither
emitted the join-quarantine marker. Native SIGSEGV reports occurred
on another Linux TID before the test's startup messages. It remains
unproven whether that signal is causally related to the malloc abort.

Use opt-in `WINE_RISCV_TLS_DIAG=1` to locate the failure *without*
changing the thread-join decision:

- `RV64-TLS-ENV pid=... tid=... process enter ...` records the
  incoming `GLIBC_TUNABLES` and `WINE_RISCV_SKIP_PTHREAD_JOIN`
  environment in each Wine process.
- `RV64-TLS-ENV ... thread enter ...` marks each native Wine worker
  before `pthread_setspecific` and Wine thread registration.
- `RV64-TLS-EXIT ... exit_thread enter` precedes decrementing the
  native thread count and any thread-join attempt.
- `RV64-TLS-EXIT ... previous thread available`, `before
  pthread_join`, `after pthread_join`, `skipping pthread_join`, and
  `exit_thread native exit` identify the subsequent branches.

These opt-in markers use a direct `write(2,...)` instead of Wine's
formatter. `GLIBC_TUNABLES` appearing in this trace only confirms
environment propagation; it **does not** confirm glibc's internal tcache
state.

```sh
timeout -k 3s 45s env -u WINE_RISCV_SKIP_PTHREAD_JOIN \
    GLIBC_TUNABLES=glibc.malloc.tcache_count=0 \
    WINE_RISCV_TLS_DIAG=1 WINE_RISCV_THREAD_DIAG=1 WINEDEBUG=-all \
    ./wine dlls/ntdll/tests/ntdll_test.exe.so thread \
    > /tmp/rv64-tls-location.log 2>&1
printf 'exit=%s\n' "$?"
grep -E 'RV64-TLS-ENV|RV64-TLS-EXIT|RV64-ABI fault|RV64-THREAD-TEST|malloc\(\)' \
    /tmp/rv64-tls-location.log | tail -150
```

Match `pid=` and `tid=` from the raw startup/exit probes to the
`RV64-ABI fault` report. glibc itself does not include PID in the
`malloc(): ...` error, so process attribution of that message still
requires a debugger or process-specific tracing.

If the failed process has no `RV64-TLS-EXIT` marker, the opt-in
`pthread_join` bypass cannot help that failure. Prefer localizing
the invalid pointer in thread initialization or native TLS allocation,
rather than intercepting arbitrary glibc `free()` calls.

### Workarounds removed; preserved differences

Removed from `risc-v` because the experiments did not resolve the fault:

- `WINE_RISCV_DISABLE_TCACHE` and `WINE_RISCV_TCACHE_REEXECED` handling
  in the Wine loader. These switches no longer have any Wine effect.
  Explicit shell `GLIBC_TUNABLES` is still a glibc feature, but not a
  confirmed fix.
- `WINE_RISCV_LASTTHREAD_NO_SERVER` and the native `nb_threads` shortcut
  for `ThreadAmILastThread`, along with `WINE_RISCV_EXIT_DIAG` and its
  `RV64-LAST` / `RV64-SERVER-MASK` checkpoints.
- The early-thread-name special case that skipped an ordinary
  `NtQueryInformationThread(ThreadBasicInformation)` request and
  emitted unconditional `RV64-NAME` logs.

Preserved pending separate regression tests:

- The RISC-V64 direct Linux `SYS_exit` worker exit path is still in use.
  It bypasses normal `pthread_exit()` unwinding across the current Wine
  stack switch and therefore also bypasses usual pthread TLS destructors.
  It is **not** equivalent to upstream Wine's pthread lifecycle. It may
  be involved in DTV corruption; replacing it requires an unwind-safe
  transition, not merely deleting the branch.
- Optional `WINE_RISCV_DIAG` server and syscall diagnostics and the
  `RV64-ABI fault` register snapshot remain available for investigation.
  The signal snapshot records Linux PID/TID, signal/code, PC, fault
  address, stack, GP/TP, preserved registers, temporaries, and
  `a0`–`a7` before any Wine TLS lookup.

The proper long-term correction is to establish who populates or damages
the glibc DTV `pointer.to_free` value, then restore correct native TLS,
stack, and pthread teardown. Never globally stub `free()` or turn
this quarantine into the default.
