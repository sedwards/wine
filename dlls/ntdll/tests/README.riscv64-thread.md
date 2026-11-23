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
