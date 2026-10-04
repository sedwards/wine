#if 0
#pragma makedep unix
#endif

#if defined(__riscv) && (__riscv_xlen == 64)

#include "config.h"

#include <assert.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif
#ifdef HAVE_SYSCALL_H
# include <syscall.h>
#else
# ifdef HAVE_SYS_SYSCALL_H
#  include <sys/syscall.h>
# endif
#endif
#ifdef HAVE_SYS_SIGNAL_H
# include <sys/signal.h>
#endif
#ifdef HAVE_SYS_UCONTEXT_H
# include <sys/ucontext.h>
#endif

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winnt.h"
#include "winternl.h"
#include "wine/asm.h"
#include "unix_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(seh);

#define REGn_sig(reg, context) ((context)->uc_mcontext.__gregs[(reg)])

#define PC_sig(context) REGn_sig( REG_PC, context )
#define RA_sig(context) REGn_sig( REG_RA, context )
#define SP_sig(context) REGn_sig( REG_SP, context )

struct syscall_frame
{
    ULONG64 x[32];
    ULONG64 pc;
    ULONG64 sp;
    ULONG64 ra;

    ULONG64 f[32];
    ULONG64 fcsr;

    ULONG restore_flags;

    struct syscall_frame *prev_frame;
    void *syscall_cfa;
    ULONG syscall_id;
};

C_ASSERT( sizeof( struct syscall_frame ) % 16 == 0 );

static void init_syscall_frame( TEB *teb, CONTEXT *context )
{
    struct thread_data *data = get_thread_data();
    struct syscall_frame *frame = get_syscall_frame( data );

    memset( frame, 0, sizeof(*frame) );

    frame->pc = context->Pc;
    frame->sp = context->Gpr.X[2];
    frame->ra = context->Gpr.X[1];

    memcpy( frame->x, context->Gpr.X, sizeof(frame->x) );

    frame->fcsr = context->Fcsr;
    memcpy( frame->f, context->F, sizeof(frame->f) );

    frame->restore_flags = context->ContextFlags;
}

void set_process_instrumentation_callback( void *callback )
{
}

NTSTATUS signal_set_full_context( CONTEXT *context )
{
    struct thread_data *data = get_thread_data();
    struct syscall_frame *frame = get_syscall_frame( data );
    NTSTATUS status;

    status = NtSetContextThread( GetCurrentThread(), context );

    if (!status && frame && (context->ContextFlags & CONTEXT_INTEGER))
        frame->restore_flags |= CONTEXT_INTEGER;

    return status;
}

NTSTATUS WINAPI NtGetContextThread( HANDLE handle, CONTEXT *context )
{
    struct thread_data *data = get_thread_data();
    struct syscall_frame *frame = get_syscall_frame( data );
    DWORD flags = context->ContextFlags & ~CONTEXT_RISCV64;

    if (handle != GetCurrentThread())
        return STATUS_NOT_IMPLEMENTED;

    if (!frame)
        return STATUS_ACCESS_DENIED;

    if (flags & CONTEXT_INTEGER)
        memcpy( context->Gpr.X, frame->x, sizeof(frame->x) );

    if (flags & CONTEXT_CONTROL)
    {
        context->Pc = frame->pc;
        context->Gpr.X[2] = frame->sp;
        context->Gpr.X[1] = frame->ra;
    }

    return STATUS_SUCCESS;
}

NTSTATUS WINAPI NtSetContextThread( HANDLE handle, const CONTEXT *context )
{
    struct thread_data *data = get_thread_data();
    struct syscall_frame *frame = get_syscall_frame( data );
    DWORD flags = context->ContextFlags & ~CONTEXT_RISCV64;

    if (handle != GetCurrentThread())
        return STATUS_NOT_IMPLEMENTED;

    if (!frame)
        return STATUS_ACCESS_DENIED;

    if (flags & CONTEXT_INTEGER)
        memcpy( frame->x, context->Gpr.X, sizeof(frame->x) );

    if (flags & CONTEXT_CONTROL)
    {
        frame->pc = context->Pc;
        frame->sp = context->Gpr.X[2];
        frame->ra = context->Gpr.X[1];
    }

    frame->restore_flags |= flags & ~CONTEXT_INTEGER;

    return STATUS_SUCCESS;
}

NTSTATUS set_thread_wow64_context( HANDLE handle, const void *ctx, ULONG size )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS get_thread_wow64_context( HANDLE handle, void *ctx, ULONG size )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS call_user_apc_dispatcher( CONTEXT *context_ptr, unsigned int flags,
                                   ULONG_PTR arg1, ULONG_PTR arg2, ULONG_PTR arg3,
                                   void (*func)(ULONG_PTR, ULONG_PTR, ULONG_PTR),
                                   NTSTATUS status )
{
    return STATUS_NOT_IMPLEMENTED;
}

void call_raise_user_exception_dispatcher( struct thread_data *data )
{
}

NTSTATUS call_user_exception_dispatcher( struct thread_data *data,
                                         EXCEPTION_RECORD *rec,
                                         CONTEXT *context )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS WINAPI NtCallbackReturn( void *ret_ptr, ULONG ret_len, NTSTATUS status )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS get_thread_ldt_entry( HANDLE handle,
                               THREAD_DESCRIPTOR_INFORMATION *info,
                               ULONG len )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS signal_alloc_thread( TEB *teb )
{
    return STATUS_SUCCESS;
}

void signal_free_thread( TEB *teb )
{
}

void signal_init_process( TEB *teb )
{
    alloc_syscall_frame( sizeof(struct syscall_frame) );
}

void DECLSPEC_NORETURN signal_start_thread( PRTL_THREAD_START_ROUTINE entry,
                                            void *arg, TEB *teb )
{
    abort();
}

void __wine_syscall_dispatcher(void)
{
    abort();
}

void __wine_unix_call_dispatcher(void)
{
    abort();
}

void *get_native_context( CONTEXT *context )
{
    return NULL;
}

void *get_wow_context( CONTEXT *context )
{
    return NULL;
}

#endif /* defined(__riscv) && (__riscv_xlen == 64) */

