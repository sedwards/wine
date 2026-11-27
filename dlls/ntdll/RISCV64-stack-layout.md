# RISC-V64 NTDLL stack layout

This document describes the Linux RV64 native Wine implementation in
`unix/thread.c`, `unix/unix_private.h` and `unix/signal_riscv64.c`.

## Stack ownership

New Linux RV64 pthreads use a separate glibc-managed native stack (currently
1 MiB with a 4096-byte guard request). Wine's kernel stack is a separate
allocation, with the root syscall frame at its upper end. The Windows
application stack is separate again and is described by the TEB StackBase
and StackLimit. The signal alternate stack is managed by Wine.

Do not place glibc pthread metadata or ELF TLS in the Wine syscall stack.
The earlier shared allocation caused suspected overlap with the root frame;
separating the stacks eliminated the allocator errors in observed tests.
The native thread pointer (tp) remains reserved for ELF TLS.

## User-mode callback host frame

`riscv64_call_user_mode_callback` reserves 1024 bytes on the current Wine
Unix stack, aligned to 16 bytes. Let B denote sp immediately after subtracting
1024. The lower 768 bytes contain the callback syscall frame and padding;
the upper 256 bytes hold the suspended Unix caller's state.

Offsets below are decimal bytes relative to B; the stack grows downward.

| Offset | Size | Contents |
| ---: | ---: | --- |
| 0 | 704 | Current RV64 syscall_frame |
| 704 | 64 | Reserved padding |
| 768 | 96 | s0 through s11, eight bytes each |
| 864 | 96 | fs0 through fs11, eight bytes each |
| 960 | 8 | Native return address (ra) |
| 968 | 8 | Address of caller's return-buffer pointer (void **) |
| 976 | 8 | Address of caller's return-length variable (ULONG *) |
| 984 | 8 | Windows callback stack pointer |
| 992 | 8 | Windows callback dispatcher address |
| 1000 | 24 | Reserved padding |
| 1024 | -- | Original Unix sp before allocation |

```
higher addresses
 B+1024  original native sp
         padding / dispatcher / Windows sp
 B+976   saved ret_len
 B+968   saved ret_ptr
 B+960   saved ra
 B+864   fs0..fs11
 B+768   s0..s11        <-- syscall_cfa / host_save
         padding
 B       callback syscall_frame
lower addresses
```

The syscall frame's prev_frame field (offset 552) links to the suspended
outer syscall frame. Its syscall_cfa field (offset 560) is a pointer to
the host save area at B+768, not to B. It also marks a live callback.
The entry helper publishes the callback as the active syscall frame before
switching to the Windows callback stack.

## Callback return invariant

NtCallbackReturn restores the previous active syscall frame and transfers
control to riscv64_user_mode_callback_return. The return assembly must:

1. Load syscall_cfa from offset 560 of the callback syscall frame.
2. Subtract 768 to recover B before assigning sp.
3. Write the returned pointer through the saved void ** at B+968 and the
   returned length (32 bits) through the saved ULONG * at B+976.
4. Restore s0..s11, fs0..fs11 and ra from the offsets above, place callback
   status in a0, add 1024 to sp, and return to the suspended Unix caller.

The previous implementation assigned sp = syscall_cfa without subtracting
768. It consequently read beyond the allocation (ret_ptr at B+1736,
ret_len at B+1744) and restored the wrong registers and stack pointer.
This matches the reported Notepad faults at the two callback result stores.

The C host-save layout and compile-time assertions in signal_riscv64.c
document the assembly contract; update both entry and return assembly and
this document together if the layout changes. These assertions check C
layout constants, not the assembled instruction stream.

## Validation

Rebuild ntdll.so and disassemble both callback functions with objdump.
Verify the return sequence includes the subtraction of 768 before restoring
sp. Run Notepad in an X11 session and the ntdll callback/exception, thread,
sync and signal tests. Check for callback result-store faults, USER lock
assertions, incorrect register restoration, and nested callback regressions.
The offset correction is source-verified; runtime validation remains
outstanding. The separate signal continuation loop is not addressed here.

## Exception delivery and SEH search

Both the synchronous Linux signal path and NtRaiseException's first-chance
dispatcher use the same 16-byte-aligned Windows exception frame:

| Offset (bytes) | Contents |
| ---: | --- |
| 0 | RISCV64 CONTEXT (736 bytes) |
| 736 | EXCEPTION_RECORD |
| end of record | Structure alignment padding, if needed |

The signal path preserves the record returned by virtual_handle_fault(),
uses virtual_setup_exception() to prepare the Windows stack, and delivers
directly to KiUserExceptionDispatcher after debugger notification. The
dispatcher passes the record and context to dispatch_exception(). The
older intermediate signal trampoline has been removed.

The dispatcher is not an ordinary LP64D call frame. The Windows-side
virtual_unwind() recognizes its address range and restores the saved
CONTEXT after validating the frame and interrupted SP against the TEB
stack bounds. In particular, the interrupted PC and native RA are separate
values. This is an internal Wine unwind rule; it does not provide external
debuggers with equivalent DWARF CFI for the dispatcher.

Native ELF SEH search handler calls use a temporary registered TEB
exception frame to report nested exceptions. RtlRestoreContext() removes
registrations below the destination SP when restoring an unwound context.
Full protected unwind-handler invocation and collided-unwind handling
remain outstanding.

These changes are source-reviewed, not yet validated on hardware. See
the root [development guide](../../risc-v.md) for the current checkpoint
and regression status.
