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

#include "broadwaydrv_dll.h"
#include "wine/debug.h"


HMODULE broadwaydrv_module = 0;


typedef NTSTATUS (*callback_func)( UINT arg );
static const callback_func callback_funcs[] =
{
    broadwaydrv_dnd_drop_event,
    broadwaydrv_dnd_leave_event,
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
    broadwaydrv_dnd_enter_event,
    broadwaydrv_dnd_position_event,
    broadwaydrv_dnd_post_drop,
    broadwaydrv_systray_change_owner,
};

C_ASSERT( NtUserDriverCallbackFirst + ARRAYSIZE(kernel_callbacks) == client_func_last );


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
    broadwaydrv_module = instance;
    if (__wine_init_unix_call()) return FALSE;
    if (BROADWAYDRV_CALL( init, &params )) return FALSE;

    callback_table = NtCurrentTeb()->Peb->KernelCallbackTable;
    memcpy( callback_table + NtUserDriverCallbackFirst, kernel_callbacks, sizeof(kernel_callbacks) );
    return TRUE;
}

/***********************************************************************
 *           AttachEventQueueToTablet (winex11.@)
 */
int CDECL BROADWAYDRV_AttachEventQueueToTablet( HWND owner )
{
    return BROADWAYDRV_CALL( tablet_attach_queue, owner );
}

/***********************************************************************
 *           GetCurrentPacket (winex11.@)
 */
int CDECL BROADWAYDRV_GetCurrentPacket( void *packet )
{
    return BROADWAYDRV_CALL( tablet_get_packet, packet );
}

/***********************************************************************
 *           LoadTabletInfo (winex11.@)
 */
BOOL CDECL BROADWAYDRV_LoadTabletInfo( HWND hwnd )
{
    return BROADWAYDRV_CALL( tablet_load_info, hwnd );
}

/***********************************************************************
 *          WTInfoW (winex11.@)
 */
UINT CDECL BROADWAYDRV_WTInfoW( UINT category, UINT index, void *output )
{
    struct tablet_info_params params;
    params.category = category;
    params.index = index;
    params.output = output;
    return BROADWAYDRV_CALL( tablet_info, &params );
}
