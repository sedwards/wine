/*
 * RISC-V64 and x86-64 signal and exception transition smoke tests.
 *
 * Copyright 2026
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdarg.h>
#include <string.h>
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winnt.h"
#include "winternl.h"
#include "wine/test.h"

#if defined(__linux__) && defined(__x86_64__) && !defined(__WINE_PE_BUILD)
#include <signal.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <ucontext.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>
#endif

#if defined(__x86_64__) || (defined(__riscv) && __riscv_xlen == 64)
#define TEST_EXCEPTION_CODE 0xe0424242
static unsigned int exception_count;
static unsigned int invalid_exception_contexts;

static ULONG_PTR context_pc( const CONTEXT *context )
{
#ifdef __x86_64__
    return context->Rip;
#else
    return context->Pc;
#endif
}

static ULONG_PTR context_sp( const CONTEXT *context )
{
#ifdef __x86_64__
    return context->Rsp;
#else
    return context->Gpr.X[2];
#endif
}

static LONG WINAPI exception_handler( EXCEPTION_POINTERS *exception )
{
    const CONTEXT *context = exception->ContextRecord;

    if (exception->ExceptionRecord->ExceptionCode != TEST_EXCEPTION_CODE)
        return EXCEPTION_CONTINUE_SEARCH;

    ++exception_count;
    if (!context || !context_pc( context ) || !context_sp( context ))
        ++invalid_exception_contexts;
    return EXCEPTION_CONTINUE_EXECUTION;
}

static void test_signal_exception(void)
{
    PVOID handler = AddVectoredExceptionHandler( 1, exception_handler );
    unsigned int i;

    ok( !!handler, "failed to install vectored exception handler\n" );
    if (!handler) return;

    exception_count = 0;
    invalid_exception_contexts = 0;
    for (i = 0; i < 8; ++i)
        RaiseException( TEST_EXCEPTION_CODE, 0, 0, NULL );

    ok( exception_count == 8, "expected 8 exceptions, received %u\n", exception_count );
    ok( !invalid_exception_contexts, "%u exceptions had invalid PC or SP\n",
        invalid_exception_contexts );
    ok( RemoveVectoredExceptionHandler( handler ), "failed to remove handler\n" );
}

#ifdef __x86_64__
static unsigned int translated_segv_count, translated_trap_count;
static unsigned int translation_mode;

static LONG WINAPI translation_handler( EXCEPTION_POINTERS *exception )
{
    EXCEPTION_RECORD *record = exception->ExceptionRecord;
    CONTEXT *context = exception->ContextRecord;

    if (translation_mode == 1 && record->ExceptionCode == EXCEPTION_ACCESS_VIOLATION)
    {
        ++translated_segv_count;
        /* The faulting instruction is exactly: movb $1,(%rax). */
        if (record->NumberParameters != 2 || record->ExceptionInformation[0] != 1 ||
            record->ExceptionInformation[1] != 0)
            return EXCEPTION_CONTINUE_SEARCH;
        context->Rip += 3;
        return EXCEPTION_CONTINUE_EXECUTION;
    }
    if (translation_mode == 2 && record->ExceptionCode == EXCEPTION_BREAKPOINT)
    {
        ++translated_trap_count;
        /* Wine/Windows may report RIP at the int3 or just after it. */
        if (*(const BYTE *)(ULONG_PTR)context->Rip == 0xcc) ++context->Rip;
        return EXCEPTION_CONTINUE_EXECUTION;
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

static void test_linux_signal_translation(void)
{
    PVOID handler = AddVectoredExceptionHandler( 1, translation_handler );

    ok( !!handler, "failed to install translation handler\n" );
    if (!handler) return;

    translation_mode = 1;
    __asm__ volatile( "xor %%rax, %%rax\n\t.byte 0xc6,0x00,0x01" : : : "rax", "memory" );
    translation_mode = 0;
    ok( translated_segv_count == 1, "expected one translated SIGSEGV, got %u\n",
        translated_segv_count );

    translation_mode = 2;
    __asm__ volatile( "int3" ::: "memory" );
    translation_mode = 0;
    ok( translated_trap_count == 1, "expected one translated SIGTRAP, got %u\n",
        translated_trap_count );

    ok( RemoveVectoredExceptionHandler( handler ), "failed to remove translation handler\n" );
}
#endif

static void test_capture_context(void)
{
    CONTEXT context;

    memset( &context, 0, sizeof(context) );
    RtlCaptureContext( &context );
    ok( context_pc( &context ) != 0, "RtlCaptureContext returned null PC\n" );
    ok( context_sp( &context ) != 0, "RtlCaptureContext returned null SP\n" );
#ifdef __x86_64__
    /* x86-64 call sites need not have a 16-byte-aligned captured RSP. */
    ok( !(context_sp( &context ) & 7), "unaligned SP %Iu\n", context_sp( &context ) );
#else
    ok( !(context_sp( &context ) & 15), "unaligned SP %Iu\n", context_sp( &context ) );
#endif
}
#endif


#if defined(__linux__) && defined(__x86_64__) && !defined(__WINE_PE_BUILD)
/* Run Linux signal probes in a forked child so Wine's handlers in the
 * winetest process remain untouched.  Each bit reports one probe result. */
enum native_probe
{
    PROBE_USR1 = 1, PROBE_CONTEXT = 2, PROBE_ALTSTACK = 4,
    PROBE_MASK = 8, PROBE_NESTED = 16, PROBE_USR2 = 32,
    PROBE_RESTORED = 64, PROBE_TRAP = 128, PROBE_SEGV = 256
};
static volatile sig_atomic_t native_usr1, native_usr2, native_nested;
static volatile sig_atomic_t native_context, native_altstack, native_mask;
static volatile sig_atomic_t native_trap, native_segv, native_mode;
static uintptr_t native_alt_start, native_alt_end;

static void native_signal_handler( int signo, siginfo_t *info, void *ptr )
{
    ucontext_t *uc = ptr;
    sigset_t mask;
    char local;
    (void)info;

    if (signo == SIGUSR1)
    {
        ++native_usr1;
        if ((uintptr_t)&local >= native_alt_start && (uintptr_t)&local < native_alt_end)
            native_altstack = 1;
        if (!sigprocmask( SIG_SETMASK, NULL, &mask ) &&
            sigismember( &mask, SIGUSR1 ) == 1 && sigismember( &mask, SIGUSR2 ) == 1)
            native_mask = 1;
        if (native_mode == 1 &&
            (uint64_t)uc->uc_mcontext.gregs[REG_R12] == UINT64_C(0x12345678aabbccdd) &&
            (uint64_t)uc->uc_mcontext.gregs[REG_R13] == UINT64_C(0x8877665544332211) &&
            uc->uc_mcontext.gregs[REG_RIP] && uc->uc_mcontext.gregs[REG_RSP])
            native_context = 1;
        if (native_mode == 2)
        {
            syscall( SYS_tgkill, getpid(), syscall( SYS_gettid ), SIGUSR2 );
            if (native_usr2) native_nested = 1;
        }
    }
    else if (signo == SIGUSR2) ++native_usr2;
    else if (signo == SIGTRAP) ++native_trap;
    else if (signo == SIGSEGV)
    {
        ++native_segv;
        uc->uc_mcontext.gregs[REG_RIP] += 3;
    }
}

__attribute__((noinline)) static void native_register_delivery(void)
{
    __asm__ volatile(
        "push %%r12\\n\\tpush %%r13\\n\\t"
        "movabs $0x12345678aabbccdd, %%r12\\n\\t"
        "movabs $0x8877665544332211, %%r13\\n\\t"
        "mov $186, %%eax\\n\\tsyscall\\n\\t"
        "mov %%eax, %%esi\\n\\t"
        "mov $39, %%eax\\n\\tsyscall\\n\\t"
        "mov %%eax, %%edi\\n\\t"
        "mov $10, %%edx\\n\\t"
        "mov $234, %%eax\\n\\tsyscall\\n\\t"
        "pop %%r13\\n\\tpop %%r12"
        : : : "rax", "rdi", "rsi", "rdx", "rcx", "r11", "memory", "cc" );
}

static unsigned int native_signal_probes(void)
{
    struct sigaction sa;
    stack_t alt;
    sigset_t mask, current;
    unsigned int result = 0;
    char stack[128 * 1024];

    memset( &alt, 0, sizeof(alt) );
    alt.ss_sp = stack;
    alt.ss_size = sizeof(stack);
    if (sigaltstack( &alt, NULL )) return 0;
    native_alt_start = (uintptr_t)stack;
    native_alt_end = native_alt_start + sizeof(stack);

    memset( &sa, 0, sizeof(sa) );
    sa.sa_sigaction = native_signal_handler;
    sa.sa_flags = SA_SIGINFO | SA_ONSTACK;
    sigemptyset( &sa.sa_mask );
    sigaddset( &sa.sa_mask, SIGUSR2 );
    if (sigaction( SIGUSR1, &sa, NULL )) return 0;
    sigemptyset( &sa.sa_mask );
    if (sigaction( SIGUSR2, &sa, NULL ) ||
        sigaction( SIGTRAP, &sa, NULL ) ||
        sigaction( SIGSEGV, &sa, NULL )) return 0;

    sigemptyset( &mask );
    sigaddset( &mask, SIGUSR1 );
    sigaddset( &mask, SIGUSR2 );
    if (sigprocmask( SIG_UNBLOCK, &mask, NULL )) return 0;

    native_mode = 1;
    native_register_delivery();
    if (native_usr1 == 1) result |= PROBE_USR1;
    if (native_context) result |= PROBE_CONTEXT;
    if (native_altstack) result |= PROBE_ALTSTACK;
    if (native_mask) result |= PROBE_MASK;

    sigemptyset( &sa.sa_mask );
    if (sigaction( SIGUSR1, &sa, NULL )) return result;
    native_mode = 2;
    syscall( SYS_tgkill, getpid(), syscall( SYS_gettid ), SIGUSR1 );
    if (native_nested && native_usr2 == 1) result |= PROBE_NESTED;
    native_mode = 0;
    syscall( SYS_tgkill, getpid(), syscall( SYS_gettid ), SIGUSR2 );
    if (native_usr2 == 2) result |= PROBE_USR2;
    if (!sigprocmask( SIG_SETMASK, NULL, &current ) &&
        sigismember( &current, SIGUSR1 ) == 0 &&
        sigismember( &current, SIGUSR2 ) == 0) result |= PROBE_RESTORED;

    __asm__ volatile( "int3" ::: "memory" );
    if (native_trap == 1) result |= PROBE_TRAP;
    __asm__ volatile( "xor %%rax, %%rax\\n\\t.byte 0xc6,0x00,0x01"
                      : : : "rax", "memory" );
    if (native_segv == 1) result |= PROBE_SEGV;
    return result;
}

static void test_native_linux_signals(void)
{
    static const struct { unsigned int bit; const char *name; } checks[] =
    {
        { PROBE_USR1, "SIGUSR1 delivery/return" },
        { PROBE_CONTEXT, "R12/R13/RIP/RSP ucontext" },
        { PROBE_ALTSTACK, "SA_ONSTACK" },
        { PROBE_MASK, "handler signal mask" },
        { PROBE_NESTED, "nested SIGUSR2" },
        { PROBE_USR2, "SIGUSR2 delivery/return" },
        { PROBE_RESTORED, "signal mask restoration" },
        { PROBE_TRAP, "SIGTRAP int3 return" },
        { PROBE_SEGV, "SIGSEGV ucontext recovery" }
    };
    int fd[2], status;
    pid_t pid;
    unsigned int result = 0, i;
    ssize_t count;

    if (pipe( fd ) == -1)
    {
        skip( "native signal pipe failed: %d\\n", errno );
        return;
    }
    pid = fork();
    if (pid == -1)
    {
        close( fd[0] );
        close( fd[1] );
        skip( "native signal fork failed: %d\\n", errno );
        return;
    }
    if (!pid)
    {
        close( fd[0] );
        result = native_signal_probes();
        write( fd[1], &result, sizeof(result) );
        _exit( 0 );
    }
    close( fd[1] );
    count = read( fd[0], &result, sizeof(result) );
    close( fd[0] );
    if (waitpid( pid, &status, 0 ) != pid)
    {
        ok( 0, "waitpid failed for native signal probe\\n" );
        return;
    }
    ok( WIFEXITED(status) && WEXITSTATUS(status) == 0 && count == sizeof(result),
        "native signal probe terminated unexpectedly (status %#x, bytes %ld)\\n",
        status, (long)count );
    if (!WIFEXITED(status) || WEXITSTATUS(status) || count != sizeof(result)) return;
    for (i = 0; i < ARRAY_SIZE(checks); ++i)
        ok( result & checks[i].bit, "native Linux %s failed\\n", checks[i].name );
}
#endif

START_TEST(signal)
{
#if defined(__x86_64__) || (defined(__riscv) && __riscv_xlen == 64)
    test_capture_context();
    test_signal_exception();
#if defined(__linux__) && defined(__x86_64__) && !defined(__WINE_PE_BUILD)
    test_native_linux_signals();
#endif
#ifdef __x86_64__
    test_linux_signal_translation();
#endif
#else
    win_skip( "native signal transition tests require x86-64 or RISC-V64\n" );
#endif
}
