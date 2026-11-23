# RISC-V64 NTDLL thread regression isolation

The normal `ntdll_test.exe.so thread` run executes all thread tests.  On
RISC-V64 it starts with a basic thread smoke test and a suspended-context
round trip.  When an early test hangs, opt in to a single group with
`WINE_RISCV_THREAD_SUITE` to test later groups independently.

From the Wine build directory:

```sh
timeout 60 env WINEDEBUG=-all WINE_RISCV_THREAD_DIAG=1 \
    WINE_RISCV_THREAD_SUITE=smoke \
    ./wine dlls/ntdll/tests/ntdll_test.exe.so thread
timeout 60 env WINEDEBUG=-all WINE_RISCV_THREAD_DIAG=1 \
    WINE_RISCV_THREAD_SUITE=context \
    ./wine dlls/ntdll/tests/ntdll_test.exe.so thread
```

Valid group selectors: `smoke`, `directexit`, `rtlexit`, `startup`, `context`, `hidden`, `teb`,
`errno`, `process`, `terminate`, `freeze`, `apc`, and `attach`.
An unset selector runs the full suite. An unknown selector fails the
test rather than silently claiming success.

`WINE_RISCV_THREAD_DIAG=1` enables unbuffered stderr checkpoints in
the RISC-V64 context and smoke tests.  The last printed checkpoint
helps identify a blocking Wine server operation, even when the harness
does not reach its final summary.

Exit code 124 from `timeout` indicates the test did not finish within
the specified interval. This does **not** prove which API is defective.
Use `WINEDEBUG=+thread,+sync,+server,+seh` or attach GDB to the
stalled process to inspect all native threads.

When the smoke-test worker prints `smoke worker returning` and the process
then exits with signal status 139, compare the raw Windows thread-termination
syscall against the full RTL shutdown path:

```sh
for suite in directexit rtlexit smoke; do
    timeout -k 3s 30s env WINEDEBUG=+thread,+seh \
        WINE_RISCV_THREAD_DIAG=1 WINE_RISCV_THREAD_SUITE="$suite" \
        ./wine dlls/ntdll/tests/ntdll_test.exe.so thread \
        > "/tmp/rv64-$suite.log" 2>&1
    printf '%s exit=%s\n' "$suite" "$?"
    grep -E 'RV64-EXIT|RV64-THREAD-TEST|BaseThreadInitThunk' "/tmp/rv64-$suite.log" | tail -50
done
```

`directexit` calls `NtTerminateThread` from the worker itself and bypasses
`RtlExitUserThread` and loader shutdown. `rtlexit` calls
`RtlExitUserThread` directly, while `smoke` returns normally through
`BaseThreadInitThunk`. These selectors are opt-in; normal thread semantics
are unchanged. If direct exit succeeds but RTL exit fails, inspect the
`RV64-EXIT` checkpoints around the last-thread query and
`LdrShutdownThread`. A status of 139 indicates SIGSEGV and must not be
interpreted as a successful test.

The `startup` group verifies that a suspended RISC-V64 thread stays suspended,
resumes, executes its entry point, and terminates successfully without
calling `NtGetContextThread` or `NtSetContextThread`. It should be run after
`smoke` and before `context` when diagnosing startup hangs.

The RISC-V64 context regression checks x0, integer register x2/SP,
s2, fs0, FCSR and native gp/tp preservation across thread
suspension, context updates and resumption.  It is not a substitute
for on-device validation of the syscall dispatcher or signal-return
assembly.

All fixes must be compiled and tested on a RISC-V64 Linux host; the
source-only checks do not establish runtime correctness.
