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
signal-safe `RV64-ABI fault` line showing `s0-s11/sp/gp/tp/ra`
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


## Opt-in glibc malloc tcache isolation (RISC-V64)

The default Wine allocator behavior is unchanged.  On glibc/Linux RV64,
`WINE_RISCV_DISABLE_TCACHE=1` makes the NTDLL loader re-execute the current
Wine loader once with `GLIBC_TUNABLES=glibc.malloc.tcache_count=0`.  This
is necessary because glibc reads the tunable before `main()`; calling
`setenv()` alone after startup would **not** disable malloc tcache.

Other entries in `GLIBC_TUNABLES` are preserved.  An existing
`glibc.malloc.tcache_count=...` entry is replaced.  The setting is inherited
by child Wine processes (including `services.exe`).  A
`WINE_RISCV_TCACHE_REEXECED` marker prevents the re-exec from looping.
Do not set that marker yourself.

From the build directory, run the same test with and without the
diagnostic, using distinct log paths:

```sh
timeout -k 3s 120s env WINEDEBUG=+thread,+seh WINE_RISCV_THREAD_DIAG=1 \
    ./wine dlls/ntdll/tests/ntdll_test.exe.so thread \
    > /tmp/rv64-tcache-default.log 2>&1
printf 'normal exit=%s\n' "$?"

timeout -k 3s 120s env WINE_RISCV_DISABLE_TCACHE=1 \
    WINEDEBUG=+thread,+seh WINE_RISCV_THREAD_DIAG=1 \
    ./wine dlls/ntdll/tests/ntdll_test.exe.so thread \
    > /tmp/rv64-tcache-disabled.log 2>&1
printf 'no-tcache exit=%s\n' "$?"

grep -E 'RV64 diagnostic|unaligned tcache|malloc\\(\\)|RV64-EXIT|RV64-THREAD-TEST|tests executed' \
    /tmp/rv64-tcache-disabled.log | tail -100
```

The one-time loader message
`wine: RV64 diagnostic: re-exec with glibc malloc tcache disabled`
confirms the re-exec was requested.  No-tcache mode is diagnostic only:
if the heap-corruption symptoms disappear or change, it is evidence that
native allocation/free lifetime needs investigation, not evidence the
underlying bug is fixed.  Expect an allocation-performance penalty.
