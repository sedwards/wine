/*
 * winex11.drv entry points
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

#include "x11drv_dll.h"
#include "wine/debug.h"


HMODULE x11drv_module = 0;

typedef NTSTATUS (*callback_func)( UINT arg );
static const callback_func callback_funcs[] =
{
    x11drv_dnd_drop_event,
    x11drv_dnd_leave_event,
};

C_ASSERT( ARRAYSIZE(callback_funcs) == client_funcs_count );

static NTSTATUS WINAPI x11drv_callback( void *arg, ULONG size )
{
    struct client_callback_params *params = arg;
    return callback_funcs[params->id]( params->arg );
}

typedef NTSTATUS (WINAPI *kernel_callback)( void *params, ULONG size );
static const kernel_callback kernel_callbacks[] =
{
    x11drv_callback,
    x11drv_dnd_enter_event,
    x11drv_dnd_position_event,
    x11drv_dnd_post_drop,
    x11drv_systray_change_owner,
};

C_ASSERT( NtUserDriverCallbackFirst + ARRAYSIZE(kernel_callbacks) == client_func_last );

#if 0
BOOL WINAPI DllMain( HINSTANCE instance, DWORD reason, void *reserved )
{
    void **callback_table;
    struct init_params params =
    {
        foreign_window_proc,
        &show_systray,
    };

    if (reason != DLL_PROCESS_ATTACH) return TRUE;

    DisableThreadLibraryCalls( instance );
    x11drv_module = instance;
    if (__wine_init_unix_call()) return FALSE;
    if (X11DRV_CALL( init, &params )) return FALSE;

    callback_table = NtCurrentTeb()->Peb->KernelCallbackTable;
    memcpy( callback_table + NtUserDriverCallbackFirst, kernel_callbacks, sizeof(kernel_callbacks) );
    return TRUE;
}

/***********************************************************************
 *           AttachEventQueueToTablet (winex11.@)
 */
int CDECL X11DRV_AttachEventQueueToTablet( HWND owner )
{
    return X11DRV_CALL( tablet_attach_queue, owner );
}

/***********************************************************************
 *           GetCurrentPacket (winex11.@)
 */
int CDECL X11DRV_GetCurrentPacket( void *packet )
{
    return X11DRV_CALL( tablet_get_packet, packet );
}

/***********************************************************************
 *           LoadTabletInfo (winex11.@)
 */
BOOL CDECL X11DRV_LoadTabletInfo( HWND hwnd )
{
    return X11DRV_CALL( tablet_load_info, hwnd );
}

/***********************************************************************
 *          WTInfoW (winex11.@)
 */
UINT CDECL X11DRV_WTInfoW( UINT category, UINT index, void *output )
{
    struct tablet_info_params params;
    params.category = category;
    params.index = index;
    params.output = output;
    return X11DRV_CALL( tablet_info, &params );
}
#endif

//#define WINE_UNIX_LIB

//#include <stdarg.h>
//#include "windef.h"
//#include "winbase.h"
///#include "ntgdi.h"
//#include "unixlib.h"

//#include <wine/debug.h>
//#include <wine/gdi_driver.h>
//#include <rdpapi.h>

//#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(winegdixdrv);

struct rdp_api
{
    // Function pointers to the rdp API callbacks
    //void (*callback1)(...);
    //void (*callback2)(...);
    // ...

    // Other members specific to the rdp API
    //int member1;
    //char member2[256];
    // ...
};

typedef struct rdp_api* XRDPAPI_HANDLE;

static XRDPAPI_HANDLE g_rdp_handle;

static void wine_rdpdrv_init(void)
{
    /* Initialize the rdp library */
    g_rdp_handle = rdpapi_init();
    if (!g_rdp_handle)
    {
        ERR("Failed to initialize rdp library\n");
        return;
    }

    /* Additional initialization steps specific to the Wine graphics driver */
    /* ... */
}

static void wine_rdpdrv_destroy(void)
{
    /* Perform cleanup and release resources */
    /* ... */

    /* Shutdown the rdp library */
    if (g_rdp_handle)
    {
        rdpapi_exit(g_rdp_handle);
        g_rdp_handle = NULL;
    }
}

#if 0
static BOOL WINAPI wine_rdpdrv_InitDriverInfo(DRIVERINFO *info, DWORD version, LPWSTR driver, LPWSTR device)
{
    /* Initialize the DRIVERINFO structure */
    /* ... */

    return TRUE;
}
#endif


typedef struct _DRIVER_INITIALIZATION_DATA {
  ULONG        Version;
  const short unsigned int          *DriverName
  ULONE	       size;
}DRIVER_INITIALIZATION_DATA;

static void WINAPI wine_rdpdrv_DriverEntry(ULONG_PTR arg)
{
    DRIVER_INITIALIZATION_DATA data;
    /* Perform driver-specific initialization */
    /* ... */

    wine_rdpdrv_init();

    /* Register the driver's capabilities and entry points */
    memset(&data, 0, sizeof(data));

    data.size = sizeof(data);
    //data.version = WINE_GDI_DRIVER_VERSION;
    data.DriverName = L"winerdp.drv";  // Set the driver name accordingly
    //data.pDispatchTable = &wine_rdpdrv_DispatchTable;
    //data.pdrvfn = &wine_rdpdrv_DrvFuncs;
    //data.entry_point = &wine_rdpdrv_DriverEntry;

    /* Register the driver with Wine's graphics driver manager */
    //GDI_DriverEntry(&data);

    /* Perform cleanup and shutdown tasks */
    wine_rdpdrv_destroy();
}

/* Entry point for the Wine graphics driver library */
BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hInstDLL);
            break;

        case DLL_PROCESS_DETACH:
            /* Perform any necessary cleanup */
            break;
    }

    return TRUE;
}

