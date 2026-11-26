# Wine on Linux RISC-V64: getting started and development guide

> **Experimental port — work in progress.** This guide describes the `risc-v` development branch of this fork, not an upstream-supported Wine configuration. Code merged into the branch is not necessarily verified on RISC-V hardware.

## Scope and project goals

The immediate goal is to build and run Wine's **native Linux RISC-V64 / Winelib (ELF)** path, then make thread startup, syscall dispatch, exception handling, context restoration, and unwinding reliable. This is distinct from running existing x86/x86-64 Windows binaries.

- **Current development branch:** `risc-v`. The historical `riscv64-context-experimental` branch was incorporated into it.
- **Primary target:** a Linux RISC-V64 machine, such as the Orange Pi RV, with a native GCC toolchain.
- **Near-term objective:** stabilize native Winelib process creation, NTDLL regression tests, exception continuation, callbacks and GUI startup on hardware.
- **Future possibilities:** native RISC-V64 Windows PE/COFF binaries, RISC-V vector support, and x86/x86-64 translation. None is a prerequisite for the ELF bring-up.

Prefer small, reviewable architecture-specific changes and preserve Wine's shared server protocol ABI. Do not disable assertions or assume a successful compile proves exception correctness.

## 1. Prepare the build machine

On Debian-derived RISC-V64 distributions, install the standard build dependencies required by your checkout and configuration. Useful diagnostic tools include:

```sh
sudo apt update
sudo apt install build-essential git pkg-config gdb binutils strace   autoconf automake libtool
```

Optional tools for investigating PE/COFF and repository history:

```sh
sudo apt install clang lld llvm gh xxd tree
apt-cache policy clang lld llvm
```

Package names and availability vary by distribution. `libc6-dev-riscv64-cross` is for cross-compilation and is **not required** for a native RISC-V64 build.

Confirm the native architecture:

```sh
uname -m
gcc -dumpmachine
gcc -dM -E - </dev/null | grep -E '__riscv|__SIZEOF_POINTER__'
```

A native 64-bit RISC-V compiler should report `__riscv_xlen 64`.

## 2. Check out and build

```sh
git clone https://github.com/sedwards/wine.git
cd wine
git switch risc-v
```

For an existing checkout:

```sh
git fetch origin
git switch risc-v
git pull --ff-only origin risc-v
```

Use a **separate build directory** when practical, so generated files and configuration changes are isolated:

```sh
mkdir -p ../wine-riscv-build
cd ../wine-riscv-build
../wine/configure --without-mingw
make -j2 2>&1 | tee /tmp/wine-riscv-build.log
```

The in-tree build used during bring-up is also supported by this fork's existing workflow:

```sh
cd ~/source/wine
./configure --without-mingw
make -j2 2>&1 | tee /tmp/wine-riscv-build.log
```

Choose **one** build layout and use it consistently. Do not run `make clean` or `make install` as part of routine debugging; first establish that the build and tests work. `--enable-archs=riscv64` was experimented with in older notes but is **not the default recommended configuration** for this native ELF bring-up. Confirm support in `./configure --help` before using it.

### Build one object while debugging

From the configured build directory:

```sh
make -B dlls/ntdll/unix/signal_riscv64.o
make -j2 2>&1 | tee /tmp/wine-riscv-build.log
```

Use the build system's generated compiler flags rather than replacing `CFLAGS` wholesale. The historical `-fshort-wchar` workaround is not a default requirement.

If `configure` itself was changed, regenerate it using the project's documented build procedure before reconfiguring. `autoreconf -fi` appeared in early experiments but should not be applied automatically to an otherwise configured tree.

### Whole-program ELF build coverage

The acceptance target is parity with the same Wine revision on native x86-64/AArch64,
including GUI programs and services. Existing PE binaries and CPU translation are not
required. There are currently 116 program modules: 111 native ELF targets, four Win16
exclusions (`rundll.exe16`, `winevdm`, `winhelp.exe16`, `winoldap.mod16`), and `dpnsvr`,
which upstream also excludes on native ARM64. Tests beneath program directories are
additional targets, not counted as application modules.

After building, audit the configured rules and actual ELF files from the source root:

```sh
python3 tools/riscv64-programs ../wine-riscv-build > /tmp/wine-riscv-programs.json
```

The command exits nonzero for missing rules, missing outputs, or incorrect ELF targets.
Its JSON inventory explicitly separates build status from runtime validation. PE
placeholders do not count as built ELF applications. This audit does not certify
application behavior: each applicable program still needs functional testing against
the reference build, first under full-system QEMU and then on native hardware.

For cross-builds, prepare `__tooldeps__` **and** `nls/all` in the native host-tools build;
otherwise resource compilers can fail to load `locale.nls`. Use a disk-backed build
directory with enough space for the complete build; the initial debug build exhausted
a small temporary filesystem. `--disable-tests` can prioritize application bring-up,
but it also disables `winetest`; the audit reports that as `disabled-by-config`,
not an exclusion. A test-enabled build is required for final acceptance.

Earlier validation established loader and wineserver version commands under QEMU user emulation. Native Orange Pi RV builds have since executed Wine initialization, command-line programs and NTDLL thread, synchronization and pipe tests (see section 7). Whole-program ELF build coverage and functional parity remain unverified; do not interpret the inventory or a successful version command as application acceptance. QEMU user emulation without binfmt integration cannot transparently execute RISC-V child loaders; service/process acceptance requires a complete target environment with local Unix sockets.

## 3. Smoke-test startup

From the **configured build directory**, using its Wine launcher:

```sh
export WINEPREFIX="$HOME/.wine-riscv-context-test"
WINEDEBUG=+timestamp,+pid,+seh,+process \
  timeout -s TERM -k 10s 120s ./wine cmd.exe /c ver \
  > /tmp/wine-riscv-context.log 2>&1

printf 'exit status: %s\n' "$?"
grep -E 'LdrInitializeThunk|NtContinue|err:|SIG' /tmp/wine-riscv-context.log
```

This command is a **diagnostic**, not a guarantee that a Windows PE program will start on this port. Test a small native Winelib ELF executable separately; inspect its format with `file` and `readelf -h`. Use a fresh `WINEPREFIX` to isolate initialization failures. A wineserver exit or clean shutdown **does not** by itself establish that Wine initialized successfully.

Only enable a debug channel if it exists in the current source. In particular, `+riscv64` is not guaranteed to be registered.

## 4. Current implementation status

Status as of **November 25, 2026**: the native ELF port builds and executes substantial portions of NTDLL on Orange Pi RV (glibc 2.36). Distinguish source implementation, observed runtime progress and full conformance. The callback return correction has now been runtime-validated by a successful native Notepad launch in X11/TigerVNC; broader GUI and callback conformance remains unverified.

| Area | Status / next validation |
| --- | --- |
| Native pthread/TLS stack separation | Implemented in `27f3efd05073`. Separate glibc-managed pthread and Wine syscall stacks eliminated the previously observed allocator errors in subsequent runs. Full lifecycle stress testing remains necessary. |
| Native thread initialization and termination | Thread workers start, execute, detach and reach Linux thread exit in observed runs. Some CreateThread calls still fail; raw Linux thread exit bypasses normal pthread cleanup. Restore safe native pthread exit and validate join/reclamation. |
| Syscall and Unix-call dispatch | Implemented; thread and sync suites reach summaries. Audit argument counts, output parameters, return values, last-error handling and nested/cross-stack transitions. |
| Native Unix protected exception recovery | `3838dfcedf0d` adds native libc longjmp recovery via data->jmp_buf. No protected recovery was observed in the supplied Notepad diagnostic run; exercise it explicitly. |
| Unprotected syscall fault recovery | `24517db42f6f` restores SP to the saved syscall frame. This fallback may abandon native cleanup and leave locks held; it does not establish correct exception recovery. |
| User-mode callbacks | `KeUserModeCallback` and `NtCallbackReturn` implemented experimentally. `9000c5b1ef3b` corrects a confirmed 768-byte return-frame displacement and adds C layout assertions; the corrected path enabled Notepad to open under X11/VNC. Nested callbacks and broader USER-lock behavior still require validation. |
| Stack layout documentation | Completed in `b2af3b3eb1c2`; see [NTDLL RISC-V64 stack layout](dlls/ntdll/RISCV64-stack-layout.md). |
| Context capture and restoration | RtlCaptureContext, RtlRestoreContext, NtContinue and SIGUSR2/rt_sigreturn transitions implemented experimentally. Latest signal test loops on a continuable software exception (0xe0424242); verify PC/RA/SP and progress after continuation. |
| Floating-point context | LP64D syscall/Unix-call entry captures all 32 FP registers and FCSR; normal returns restore callee-saved FP state and FCSR. Full hardware register-preservation coverage pending. |
| Nested syscall frames | Frame chaining and nonlocal-continuation cleanup implemented; complex exception paths and nested callback ownership remain unverified. |
| Native ELF/DWARF unwind | Connected to native exception dispatch and frame walking. Earlier isolated QEMU CFI tests passed; full runtime/hardware validation pending. |
| Windows RtlVirtualUnwind / RtlUnwindEx / SEH | Incomplete; ELF unwind support does not establish full Windows SEH. |
| APC and remote thread context | SIGUSR1 suspension and remote context exchange implemented; dispatcher-boundary races and APC integration still require validation. |
| Windows-side setjmp / longjmp | Initial RV64 implementation; validate jump-buffer layout and nonlocal transfers separately from native libc longjmp. |
| Native Winelib child process creation | Outstanding: thread and sync tests report CreateProcess error 2; the developer confirms native executables require explicit Unix paths for reliable launching. Notepad logs also show start.exe's bare-name GetBinaryTypeW and CreateProcessW attempts failing with error 2. Verify path resolution and child launch separately from ELF image recognition. |
| Named pipes and RPC | Pipe suite executes many sections but has numerous failures; audit I/O arguments, output parameters, status conversion and APC/completion paths before rewriting shared pipe logic. |
| GUI / Common-Controls | Notepad now opens under X11/VNC after the callback correction. The latest log has no USER-lock assertion or Common-Controls error, but contains EGL warnings, a separate SIGSEGV and bare-name launch errors. Common-Controls v6 and broader GUI functionality still require validation. |
| RISC-V vector extension, native Windows PE/COFF and x86 translation | Future/exploratory; not prerequisites for native ELF bring-up. |

### Recent architectural fixes

The original pthread allocation overlapped the address range used for Wine's root syscall frame. Giving glibc its own native stack avoids that conflicting ownership. The callback return assembly had a separate confirmed error: syscall_cfa is the allocation base plus 768, while the return assembly had treated it as the base. The new return path subtracts 768 before restoring SP, retrieving callback outputs and restoring saved registers.

Do not remove the USER-lock assertion to work around GUI failures. Validate the callback correction first; investigate any remaining faults at their first native instruction.

### RISC-V64 server-context layout

Wine's `struct context_data` protocol has a fixed **1712-byte** layout. Early RISC-V64 code serialized all 32 integer registers into the integer union, growing the structure to **1728 bytes** and triggering the server's static assertion.

The branch now uses **30 transmitted integer registers**: `x1` and `x3`–`x31`. `x0` is architecturally zero; `x2` (SP) is represented by the control-register union. The corresponding packing/unpacking code lives in `dlls/ntdll/unix/thread.c`.

**Do not change or comment out the assertion** in `server/request_handlers.h`. The goal is to retain the shared protocol layout. If the build still reports a size mismatch, verify that generated headers match the current `server/protocol.def`, and measure the layout rather than changing the expected size.

After changing `server/protocol.def`, regenerate its checked-in headers from the source root:

```sh
tools/make_requests
```

The generated `include/wine/server_protocol.h` must also contain `x[30]`. Building against an older `x[32]` header still violates the layout assertion even when the protocol source is correct. Rebuild both Wine and wineserver together after a protocol version change.

## 5. Debugging and validation

Useful starting points:

```sh
# Confirm object and executable formats
file dlls/ntdll/unix/signal_riscv64.o
readelf -h dlls/ntdll/unix/signal_riscv64.o

# Inspect native unwind metadata
readelf --debug-dump=frames dlls/ntdll/unix/signal_riscv64.o

# Trace process and signal activity
strace -f -o /tmp/wine-riscv-strace.log ./wine --version

# Inspect the first build error
grep -n -m 10 -E 'error:|undefined reference|Error [0-9]' /tmp/wine-riscv-build.log
```

For a reproducible crash, collect the executable name, exact command, build commit, signal, `gdb bt`, `info registers`, and relevant `x/i` and `x/gx` output. For context and unwind defects, test leaf and non-leaf functions, saved `ra`/`s0`–`s11`, FP state, nested syscall callbacks, and nonlocal exception exits. Distinguish **native ELF DWARF unwinding** from **Windows PE unwind metadata**.

### Thread/context regression coverage

The RISC-V64 cases in `dlls/ntdll/tests/thread.c` check suspended startup, suspension of a running register loop, remote integer/FP updates, integer-only SP/PC preservation, FCSR, and native `gp`/`tp` preservation. With a working target Wine runtime, run:

```sh
make dlls/ntdll/tests/thread.ok
# Clear cached results before repeating:
make dlls/ntdll/tests/testclean
```

Cross-compiling the test object is only a compilation check. Isolated QEMU checks of dispatcher assembly and Linux signal-context helpers do not establish full Wine startup, suspension at every dispatcher instruction, Windows SEH, or callback correctness.

### ELF frame walking

Native exception dispatch and `RtlWalkFrameChain` now consult ELF unwind information
through the Unix builtin-unwind bridge before falling back to leaf unwinding. A saved
return address is looked up one byte before its PC to handle compressed instructions;
an interrupted instruction uses its exact PC. Leaf frames may preserve SP if PC advances,
and an undefined DWARF return register terminates the walk.

`dlls/ntdll/tests/exception.c` includes a RISC-V64 recursive frame-walk test covering
frame counts, skip flags, and output bounds. Run `make dlls/ntdll/tests/exception.ok`
with a working target runtime. The changed signal objects and exception test have
cross-compiled; isolated tests using assembler-generated ELF CFI passed under QEMU.
This does not complete `RtlUnwindEx`, language-specific handlers, or collided unwind.

### Development priorities

1. Validate the new RtlRaiseException caller-context correction with the signal suite; investigate any remaining NtContinue PC/RA/SP restoration or exception continuation failures.
2. Correct native Winelib executable resolution and child process creation; rerun thread and sync tests with a consistent prefix and build.
3. Symbolize the remaining GUI access violation and investigate possible 64-bit pointer truncation before changing pointer arithmetic.
4. Validate nested callbacks, callback output pointers, syscall frame ownership and native protected fault recovery; expand GUI coverage beyond the successful Notepad launch (menus, file dialogs and other programs).
5. Audit syscall argument marshalling, return/status conversion, TEB last-error values, named pipes, APCs and asynchronous I/O completion.
6. Restore safe native pthread termination and validate pthread_join, TLS destructors and resource reclamation; retire diagnostic bypasses only after verification.
7. Resolve Common-Controls activation contexts and remaining GUI/graphics issues.
8. Complete Windows SEH/unwind support and broader register, dispatcher-boundary and regression coverage. Preserve shared server protocol ABI.

## 6. Historical experiments and optional future work

The following notes are **retained for research**, not recommended build steps.

### LLVM / RISC-V64 COFF experiments

An earlier attempt to generate Windows-style PE/COFF files with Clang and LLD did not establish a working Wine PE runtime. Compiler support for a target triple, COFF object generation, PE linking, Wine loader recognition, and executable startup are **separate questions**. Support varies with installed LLVM versions.

To check whether a particular Clang accepts a COFF target:

```sh
clang --version
ld.lld --version
command -v lld-link

cat >/tmp/riscv-pe-test.c <<'EOF'
int main(void) { return 0; }
EOF

clang --target=riscv64-windows-coff -c /tmp/riscv-pe-test.c \
  -o /tmp/riscv-pe-test.obj
file /tmp/riscv-pe-test.obj
```

Even a successful `.obj` compilation **does not** imply that `clang --target=riscv64-windows-coff -fuse-ld=lld ...` can link a usable executable, or that Wine can load it. Earlier bring-up logs included `STATUS_INVALID_IMAGE_WIN_64 (c000035a)`; investigate machine validation separately from compiler/linker support. Claims of full upstream LLVM RISC-V Windows COFF support in old notes were not independently verified for the installed toolchain.

### Historical local workarounds — do not apply automatically

The old scratch notes recorded these commands:

```sh
# Historical linker workaround, not a default build setting:
make -j4 LDFLAGS="-Wl,-u,__wine_riscv64_get_teb -latomic"

# Historical investigation of preloader placement:
make -C loader wine-preloader \
  WINEPRELOADER_LDFLAGS="-nostartfiles -nodefaultlibs -static -Wl,-Ttext=0x200100000" \
  -B V=1

# Historical workaround for a missing build-tool path:
ln -s ../winebuild/winebuild tools/winegcc/winebuild
```

Do **not** create the symlink, override linker flags, or force a preloader address without first reproducing and understanding the underlying issue on a clean tree. These are debugging artifacts, not project requirements.

### Future architecture options

- Native RISC-V64 PE/COFF generation and loader support.
- RISC-V vector register context and unwind handling.
- WOW64 or x86/x86-64 binary translation through a separately designed emulation layer.

None should block the native Linux/Winelib ELF milestone.

## Contributing and reporting results

Keep changes small, architecture-scoped, and consistent with existing x86-64/AArch64 Wine patterns. Prefer protocol-neutral solutions; include tests and explain ABI assumptions. For a useful bug report, record the commit (`git rev-parse HEAD`), toolchain versions, configure options, failing command, first error, and GDB trace where applicable.

This guide reflects the development branch as of **November 25, 2026**. Record source changes separately from hardware validation; update observed results with the tested commit, environment and exit status.

## 7. Current hardware smoke-test results (November 25, 2026)

Target: Orange Pi RV, native Linux RISC-V64 ELF/Winelib build, glibc 2.36. Regression summaries below predate callback fix `9000c5b1ef3b`; the Notepad result includes the subsequent hardware retest. These are not a certification of the latest branch.

| Test | Observed result |
| --- | --- |
| wineboot | Explicit `./wine programs/wineboot/wineboot.exe.so --init` previously returned 0 and created system.reg and user.reg. Short-name resolution has not been consistently reliable. |
| cmd | Native ELF command prompt reached; interactive console I/O remains unverified. |
| thread | After stack separation, reached summary; subsequent jmp_buf build reported 63 assertions, 19 failures, 0 skipped. Failures include child process creation, process termination statuses and CreateThread error 203. No matched allocator-corruption diagnostics in the separate-stack run. |
| sync | 433 assertions, 2 failures, 1 todo, 0 skipped. Both ordinary failures concern child creation/wait; not full synchronization conformance. |
| pipe | Many test sections executed, with numerous failures in creation, writes, cancellation and completion. No final summary supplied. |
| signal | Initially empty output; diagnostic rerun repeatedly delivered software exception 0xe0424242 and resumed via NtContinue/SIGUSR2 without reaching a summary. Treat as a continuation loop, not a pass. Earlier reported successful signal runs do not establish the current result. |
| Notepad / GUI | Successfully opened under X11/TigerVNC after the callback offset correction. Earlier repeated callback result-store faults and USER-lock assertions are absent from the new log. Remaining EGL warnings, one different SIGSEGV and bare-name launch failures require investigation. |
| Native thread shutdown | Observed FLS cleanup, DLL_THREAD_DETACH, NtTerminateThread and raw native exit. Complete process shutdown, pthread join and reclamation are not established. |

No direct x86-64 comparison is available for these runs. Do not infer parity from assertion counts or presume every failure is architecture-specific. Exit status was not supplied for all runs; reaching a summary does not prove clean process shutdown.

A TigerVNC/Openbox/xterm session and glxgears previously worked on the target. This does not prove Wine GUI correctness. Test using a known initialized prefix to separate application startup from wineboot and service initialization.

### Native GUI milestone and executable lookup limitation

The developer confirmed that Notepad ran in the X11/VNC session after the
callback correction. A fresh prefix was created in that run. No successful
menu/file-dialog test or clean exit status was supplied.

The developer also confirms that native Winelib binaries are not reliably
found unless an explicit Unix path is supplied. From the configured build
directory, use the actual built ELF path when bare Windows names fail:

```sh
./wine programs/notepad/notepad.exe.so
```

The latest log contains:

```text
RV64-LAUNCH start: file=notepad.exe recognized=0 type=0 error=2
RV64-LAUNCH start: CreateProcessW failed file=notepad.exe error=2
```

The current start.exe fallback attempts CreateProcessW after GetBinaryTypeW
fails, then allows ShellExecuteEx to run. The log alone does not establish
which path ultimately opened Notepad. Audit executable search, DOS/Unix path
conversion, builtin ELF discovery and child process launch. Do not assume
that changing image recognition alone will fix path resolution.

A separate SIGSEGV accessed 0x750b2d2b while nearby registers contained
addresses such as 0x5750b2cd0 and 0x5750b2cf4. Possible loss of upper pointer
bits is a hypothesis, not a confirmed cause; symbolize the faulting PC and
inspect its instruction before changing code. The EGL hardware-device/software
rendering warnings are also unresolved but did not prevent the observed launch.

### Repeatable native regression runs

From the configured build directory:

```sh
export WINEPREFIX="$HOME/.wine-riscv64"
unset GLIBC_TUNABLES WINE_RISCV_SKIP_PTHREAD_JOIN WINE_RISCV_DIAG
./server/wineserver -k
./server/wineserver -w

for test in thread sync pipe signal; do
    timeout -k 5s 180s env WINEDEBUG=-all \
        ./wine dlls/ntdll/tests/ntdll_test.exe.so "$test" \
        > "/tmp/rv64-$test-results.log" 2> "/tmp/rv64-$test-diagnostics.log"
    printf '%s exit status: %s\n' "$test" "$?"
done
```

Use the same prefix and build revision when comparing results; isolate each test with a fresh wineserver if startup or shutdown contaminates subsequent runs. A timeout (124) or empty log is not a pass. Enable WINE_RISCV_DIAG only for targeted debugging: the per-syscall logging can generate millions of lines and affect scheduling.

For callback validation:

```sh
objdump -d --disassemble=riscv64_user_mode_callback_return dlls/ntdll/ntdll.so
timeout -k 5s 120s env WINEDEBUG=-all ./wine notepad.exe \
    > /tmp/rv64-notepad.log 2>&1
printf 'exit status: %s\n' "$?"
```

Verify the subtraction of 768 before restoring SP. Run the GUI command from a working X11/VNC session. If bare-name lookup fails, use the explicit built ELF path shown above. Close Notepad normally to assess process exit; a timeout while a working window remains open is not itself a GUI failure.

## 8. RISC-V64 syscall-frame lifecycle tracing

The experimental `signal_riscv64.c` implementation now emits frame lifecycle markers when `WINE_RISCV_DIAG=1`:

| Marker | Meaning |
| --- | --- |
| `RV64-FRAME prepare ROOT` | Cross-stack entry reuses the current root frame; includes entry SP, frame, predecessor, CFA and target SP. |
| `RV64-FRAME prepare NESTED` | A nested frame is allocated on the Unix stack and linked to its predecessor. |
| `RV64-FRAME finish POP` | Completed nested frame is popped and its predecessor restored as active. |
| `RV64-FRAME finish RETAIN` | Active frame remains selected at the end of the call. |

The retained `RV64-FLOW continue REQUEST` and `RV64-FLOW sigreturn APPLY` markers provide PC, SP, RA, flags and frame-link information. The former `RV64-FRAME sigreturn CONSUME` and redundant checkpoints were removed during diagnostic consolidation. With `WINE_RISCV_DIAG` unset, SIGSEGV logging is limited to compact `RV64-FAULT` records (up to eight per process); opt in for full register dumps. SIGUSR2 handler logging uses the existing bounded, `write(2)`-based diagnostic formatter; do not add Wine TRACE/WARN, malloc, or pthread calls inside the signal handler.

```sh
cd ~/source/wine
WINE_RISCV_DIAG=1 WINEDEBUG=-all,+seh \
  timeout -k 5s 60s ./wine dlls/ntdll/tests/ntdll_test.exe.so syscall \
  > /tmp/rv64-syscall.log 2>&1
grep -E 'RV64-FRAME|RV64-FLOW|err:|Assertion' /tmp/rv64-syscall.log
```

Frame logs are diagnostic and **do not prove correct ownership**. In particular, the cross-stack root reuse and nonlocal continuation cleanup still require targeted stress tests. Avoid changing frame ownership or the USER lock solely to silence assertions.

## 9. New NTDLL smoke tests

The `dlls/ntdll/tests/Makefile.in` registers two architecture-focused smoke tests:

- `signal.c` (`signal`): on RISC-V64, capture a CONTEXT and check nonzero PC/SP and stack alignment, then exercise vectored exception delivery with a continuable software exception. On other architectures it reports a Wine `win_skip`.
- `syscall.c` (`syscall`): on RISC-V64, repeat `NtQueryInformationThread(ThreadBasicInformation)` and compare Wine thread identity; query `NtQueryInformationProcess(ProcessBasicInformation)` and compare process identity. On other architectures it reports `win_skip`.

```sh
make dlls/ntdll/tests/ntdll_test.exe.so
./wine dlls/ntdll/tests/ntdll_test.exe.so syscall
./wine dlls/ntdll/tests/ntdll_test.exe.so signal
```

These are **initial smoke tests**, not complete syscall argument-count, nested callback, asynchronous signal, APC, register preservation, or nonlocal NtContinue tests. The pre-fix signal diagnostic run loops on a continuable exception (section 7). A caller-context correction and an exact PC/SP/RA/argument regression test have since been added; hardware validation is pending. Current syscall smoke-test results have not been supplied. Record exit codes, assertions and branch commit.

## 10. Focused debugging commands

```sh
# Capture current process/thread wait states.
ps -eo pid,ppid,stat,wchan:25,etime,comm,args | grep -E 'wine|services|rpcss'

# Inspect an idle wineserver; epoll_pwait2 is a normal wait location.
sudo gdb -q -p <WINESERVER_PID> -batch \
    -ex 'set pagination off' -ex 'thread apply all bt 12'

# Find the USER-lock assertion caller in the process that actually aborts.
# GDB may need to attach to a child process rather than the wineboot launcher.
git grep -n user_check_not_lock dlls/win32u
```

For the USER assertion, record the full stack, `user_lock_thread`, `user_lock_rec`, current Wine thread ID, native TID, and `tp`. The USER mutex is configured recursive in `sysparams_init()`; the assertion is not evidence of a nonrecursive-mutex configuration mistake.

For reproducible results always record `git rev-parse HEAD`, the exact command and environment, exit code, and whether the run used SSH, VNC, or Xvfb.

### Optional Explorer desktop diagnostic

From the configured build directory in an X11 session:

```sh
./wine explorer /desktop=shell,800x600 null.exe
```

Explorer functionality remains unverified; use this only as a GUI diagnostic.

## 11. Post-Notepad cleanup review against master

Review baseline: master `274fcfa9691bf3b7f24c0833ad33e9d57c3df65f`,
common ancestor `97027f879ba9a36dbe594720d4710818536e5433`.
The comparison reported 413 branch commits ahead and 112 behind before
this documentation update. Master integration is a separate task from
cleanup; do not replace shared files wholesale with current master.

Preserve the working Notepad source baseline (requested tag:
`rv64-notepad`) before changing runtime code.

### Low-risk diagnostic cleanup

Remove temporary services.exe trap, kernel32 thread startup instrumentation,
RPC wait/listen diagnostics, boot and TLS environment diagnostics,
per-request server logging, FLS and DLL shutdown tracing, and obsolete TLS
exit checkpoints. Consolidate signal/syscall tracing separately, keeping
useful frame checks and signal-safe diagnostics for the outstanding
continuation failure. Preserve regression assertions and executable lookup
diagnostics until the associated defects are resolved.

### Behavioral changes requiring separate validation

| Area | Review recommendation |
| --- | --- |
| Virtual memory initialization | Compare with master's single protection-table allocation and write-execute probe. Branch appears to duplicate allocations on other 64-bit CPUs; verify RV64 address limits before changing. |
| Executable lookup | Audit suppression of fake PE placeholders in tools/makedep.c and prefix installation, then SearchPathW/CreateFileW and builtin ELF discovery. Missing placeholders are a hypothesis, not a confirmed cause. Retain launch fallback until a replacement works. |
| IMM32/Win32u detach | Test whether the no-create IME query is still needed; revert coordinated API changes together only after validation. |
| DLL entry ABI wrapper | Retain required calling convention and CFI; reevaluate speculative register repair separately. |
| Exception termination limits | Replace arbitrary repeat-count exits with validated stack/context handling after fixing the signal loop. |
| Native thread termination | Restore normal pthread cleanup on the original native stack; do not blindly replace raw SYS_exit. |
| TLS fallback registry | Test standard pthread lookup and signal-safe identity before removing the registry or join quarantine. |
| Master integration | Merge separately and regenerate build artifacts; validate other architectures as well. |

Retain the verified pthread stack separation and callback offset correction,
native longjmp recovery, syscall frame restoration, architecture definitions,
dispatchers, context handling, ELF unwind support and regression tests.
After each cleanup group rebuild and test explicit-path CMD, thread, sync,
pipe, signal and Notepad under X11. Compare against the recorded baseline;
existing failures are not new regressions unless the result changes.

## 12. Signal cleanup and Mount Manager investigation (November 26, 2026)

Hardware observations after the first diagnostic cleanup: Explorer, Notepad,
WordPad, Task Manager and OLE Viewer run under X11/VNC, according to the
developer. Mount Manager fails, and clients depending on it also fail.
Thread (63 assertions, 19 failures) and sync (433 assertions, 2 failures)
summaries remain unchanged; pipe failures persist and no pipe summary was
supplied. No new signal result was supplied.

The second cleanup series removes hardcoded syscall-number diagnostics,
repetitive syscall/TLS tracing and duplicate signal continuation records.
Frame validation, callback restoration, native longjmp recovery and
nonvolatile preservation are retained.

The former C RtlRaiseException captured its own internal continuation
rather than its caller. Commit `0be31122ef11` introduces an assembly wrapper
that reconstructs the caller PC, RA, SP and a0 in a CONTEXT and follows the
debugger-aware dispatch pattern used on other architectures. The new test
in `dlls/ntdll/tests/signal.c` compares these fields with an assembly caller's
known values. The change is source-verified, not yet hardware-validated;
the earlier signal loop is not yet established as resolved.

Mount Manager startup requires builtin driver and Unix-library discovery,
__wine_init_unix_call, device and symbolic-link creation, harddisk driver
initialization and worker thread creation. Its failure cannot yet be
attributed to exception handling. Capture +mountmgr,+ntoskrnl,+service,+seh
and +module traces from wineboot with the corrected runtime before changing
the driver. Preserve the executable lookup investigation, since missing
Windows-visible files may also affect service and driver startup.

Next steps: rebuild and run signal independently with a timeout, then
capture Mount Manager startup and device-control failures. Retest the
working GUI applications and thread/sync/pipe suites. Keep signal
diagnostics opt-in and retain the known pre-cleanup baseline.
