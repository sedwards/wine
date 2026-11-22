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

Valid group selectors: `smoke`, `startup`, `context`, `hidden`, `teb`,
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
