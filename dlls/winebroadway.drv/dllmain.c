/*
 * winebroadway.drv entry points
 *
 * Copyright 2022 Jacek Caban for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "winioctl.h"
#include "ddk/wdm.h"
#include "unixlib.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(broadway);

extern NTSTATUS broadwaydrv_client_func( enum broadwaydrv_client_funcs func, const void *params,
                                    ULONG size ) DECLSPEC_HIDDEN;


extern NTSTATUS CDECL wine_ntoskrnl_main_loop( HANDLE stop_event );
static HANDLE stop_event;
static HANDLE thread;
HMODULE broadwaydrv_module = 0;
#if 0
static NTSTATUS WINAPI ioctl_callback( DEVICE_OBJECT *device, IRP *irp )
{
    NTSTATUS status = STATUS_SUCCESS;
    IoCompleteRequest( irp, IO_NO_INCREMENT );
    return status;
}

static NTSTATUS CALLBACK init_broadway_driver( DRIVER_OBJECT *driver, UNICODE_STRING *name )
{
    static const WCHAR device_nameW[] = {'\\','D','e','v','i','c','e','\\','W','i','n','e','B','r','o','a','d','w','a','y',0 };
    static const WCHAR device_linkW[] = {'\\','?','?','\\','W','i','n','e','B','r','o','a','d','w','a','y',0 };

    UNICODE_STRING nameW, linkW;
    DEVICE_OBJECT *device;
    NTSTATUS status;

    driver->MajorFunction[IRP_MJ_DEVICE_CONTROL] = ioctl_callback;

    RtlInitUnicodeString( &nameW, device_nameW );
    RtlInitUnicodeString( &linkW, device_linkW );

    if ((status = IoCreateDevice( driver, 0, &nameW, 0, 0, FALSE, &device ))) return status;
    return IoCreateSymbolicLink( &linkW, &nameW );
}

static DWORD CALLBACK device_thread( void *arg )
{
    static const WCHAR driver_nameW[] = {'\\','D','r','i','v','e','r','\\','W','i','n','e','B','r','o','a','d','w','a','y',0 };

    HANDLE start_event = arg;
    UNICODE_STRING nameW;
    NTSTATUS status;
    DWORD ret;

    ERR( "device_thread: starting process %lx\n", GetCurrentProcessId() );

    /* not running under Java */
    //if (BROADWAY_CALL( java_init, NULL )) return 0;

    RtlInitUnicodeString( &nameW, driver_nameW );
    if ((status = IoCreateDriver( &nameW, init_broadway_driver )))
    {
        FIXME( "failed to create driver error %lx\n", status );
        return status;
    }

    stop_event = CreateEventW( NULL, TRUE, FALSE, NULL );
    SetEvent( start_event );

    //BROADWAY_CALL( java_uninit, NULL );
    return ret;
}

static NTSTATUS WINAPI broadway_start_device(void *param, ULONG size)
{
    HANDLE handles[2];

    handles[0] = CreateEventW( NULL, TRUE, FALSE, NULL );
    handles[1] = thread = CreateThread( NULL, 0, device_thread, handles[0], 0, NULL );
    WaitForMultipleObjects( 2, handles, FALSE, INFINITE );
    CloseHandle( handles[0] );
    return HandleToULong( thread );
}
#endif

typedef NTSTATUS (*callback_func)( UINT arg );
static const callback_func callback_funcs[] =
{
};

C_ASSERT( ARRAYSIZE(callback_funcs) == client_funcs_count );

static NTSTATUS WINAPI broadwaydrv_callback( void *arg, ULONG size )
{
    struct client_callback_params *params = arg;
    return callback_funcs[params->id]( params->arg );
}

typedef NTSTATUS (WINAPI *kernel_callback)( void *params, ULONG size );
static const kernel_callback kernel_callbacks[] =
{
    broadwaydrv_callback,
};

C_ASSERT( NtUserDriverCallbackFirst + ARRAYSIZE(kernel_callbacks) == client_func_last );

/***********************************************************************
 *       dll initialisation routine
 */
BOOL WINAPI DllMain( HINSTANCE instance, DWORD reason, void *reserved )
{
#if 0
    struct init_params params;
    void **callback_table;
#endif

    void **callback_table;
    struct init_params params =
    {
    };

    if (reason != DLL_PROCESS_ATTACH)
	return TRUE;

    DisableThreadLibraryCalls( instance );
    broadwaydrv_module = instance;

    if (__wine_init_unix_call())
	return FALSE;

    if (BROADWAYDRV_CALL( init, &params ))
	return FALSE;

    callback_table = NtCurrentTeb()->Peb->KernelCallbackTable;
    memcpy( callback_table + NtUserDriverCallbackFirst, kernel_callbacks, sizeof(kernel_callbacks) );

    ERR("At least we got to DLLmain\n");
    return TRUE;
}

