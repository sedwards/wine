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
#include <stdarg.h>
#include <string.h>
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winnt.h"
#include "winternl.h"
#include "wine/test.h"

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
    test_signal_exception();
#else
    win_skip( "native signal transition tests require x86-64 or RISC-V64\n" );
#endif
}
