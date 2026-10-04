#if 0
#pragma makedep unix
#endif

#if defined(__riscv) || (__riscv_xlen == 64)

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
#include "windef.h"
#include "winnt.h"
#include "winternl.h"
#include "wine/asm.h"
#include "unix_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(seh);

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "winternl.h"
#include "unix_private.h"


void set_process_instrumentation_callback( void *callback )
{
}


NTSTATUS signal_set_full_context( CONTEXT *context )
{
    return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS WINAPI NtGetContextThread( HANDLE handle, CONTEXT *context )
{
    return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS WINAPI NtSetContextThread( HANDLE handle, const CONTEXT *context )
{
    return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS set_thread_wow64_context( HANDLE handle, const void *ctx, ULONG size )
{
    return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS get_thread_wow64_context( HANDLE handle, void *ctx, ULONG size )
{
    return STATUS_NOT_IMPLEMENTED;
}


//NTSTATUS call_user_apc_dispatcher( CONTEXT *context_ptr, unsigned int flags,
//                                   ULONG_PTR arg1, ULONG_PTR arg2, ULONG_PTR arg3,
//                                   ULONG_PTR *ret_ptr )
//{
//    return STATUS_NOT_IMPLEMENTED;
//}


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
}


void DECLSPEC_NORETURN signal_start_thread( PRTL_THREAD_START_ROUTINE entry,
                                            void *arg, TEB *teb )
{
    /* Stub: this will need real RISC-V thread startup eventually. */
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


//void init_shared_data_cpuinfo( struct _KUSER_SHARED_DATA *data )
//{
//}

#endif
/* defined(__riscv) || (__riscv_xlen == 64) */
