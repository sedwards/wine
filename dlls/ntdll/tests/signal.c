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
#include "wine/asm.h"

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

#if defined(__riscv) && __riscv_xlen == 64
static ULONG_PTR expected_pc, expected_sp;
static EXCEPTION_RECORD *expected_record;
static unsigned int caller_exception_count, caller_context_errors;

extern void riscv64_raise_from_caller( EXCEPTION_RECORD *record, ULONG_PTR *pc,
                                     ULONG_PTR *sp, void (WINAPI *raise)(EXCEPTION_RECORD *) );
__ASM_GLOBAL_FUNC( riscv64_raise_from_caller,
                   "addi sp, sp, -16\n\t"
                   __ASM_CFI(".cfi_def_cfa_offset 16\n\t")
                   "sd ra, 0(sp)\n\t"
                   __ASM_CFI(".cfi_offset 1, -16\n\t")
                   "lla t0, 1f\n\t"
                   "sd t0, 0(a1)\n\t"
                   "sd sp, 0(a2)\n\t"
                   "jalr ra, a3, 0\n\t"
                   "1: ld ra, 0(sp)\n\t"
                   "addi sp, sp, 16\n\t"
                   __ASM_CFI(".cfi_def_cfa_offset 0\n\t")
                   __ASM_CFI(".cfi_restore 1\n\t")
                   "ret" )

static LONG WINAPI caller_exception_handler( EXCEPTION_POINTERS *exception )
{
    CONTEXT *context = exception->ContextRecord;

    if (exception->ExceptionRecord->ExceptionCode != TEST_EXCEPTION_CODE)
        return EXCEPTION_CONTINUE_SEARCH;
    ++caller_exception_count;
    if (context->Pc != expected_pc || context->Gpr.X[1] != expected_pc ||
        context->Gpr.X[2] != expected_sp ||
        context->Gpr.X[10] != (ULONG_PTR)expected_record ||
        (ULONG_PTR)exception->ExceptionRecord->ExceptionAddress != expected_pc)
    {
        ++caller_context_errors;
        trace( "caller context mismatch: pc=%p ra=%p sp=%p a0=%p expected pc=%p sp=%p record=%p\n",
               (void *)(ULONG_PTR)context->Pc, (void *)(ULONG_PTR)context->Gpr.X[1],
               (void *)(ULONG_PTR)context->Gpr.X[2], (void *)(ULONG_PTR)context->Gpr.X[10],
               (void *)expected_pc, (void *)expected_sp, expected_record );
        /* Report the bad context but avoid recursively raising it forever. */
        context->Pc = expected_pc;
        context->Gpr.X[1] = expected_pc;
        context->Gpr.X[2] = expected_sp;
    }
    return EXCEPTION_CONTINUE_EXECUTION;
}

static void test_raise_caller_context(void)
{
    EXCEPTION_RECORD record = {0};
    PVOID handler;
    unsigned int i;

    record.ExceptionCode = TEST_EXCEPTION_CODE;
    expected_record = &record;
    caller_exception_count = caller_context_errors = 0;
    handler = AddVectoredExceptionHandler( 1, caller_exception_handler );
    ok( !!handler, "failed to install caller-context handler\n" );
    if (!handler) return;

    for (i = 0; i < 8; ++i)
        riscv64_raise_from_caller( &record, &expected_pc, &expected_sp, RtlRaiseException );

    ok( caller_exception_count == 8, "expected 8 caller exceptions, got %u\n", caller_exception_count );
    ok( !caller_context_errors, "%u exceptions captured the wrong caller context\n", caller_context_errors );
    ok( RemoveVectoredExceptionHandler( handler ), "failed to remove caller-context handler\n" );
}
#endif

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



START_TEST(signal)
{
#if defined(__x86_64__) || (defined(__riscv) && __riscv_xlen == 64)
    test_capture_context();
#if defined(__riscv) && __riscv_xlen == 64
    test_raise_caller_context();
#endif
    test_signal_exception();
#ifdef __x86_64__
    test_linux_signal_translation();
#endif
#else
    win_skip( "native signal transition tests require x86-64 or RISC-V64\n" );
#endif
}
