/*
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

#include "ntuser.h"
#include "wine/unixlib.h"

//UNIX_CFLAGS  = $(GSTREAMER_CFLAGS)
//UNIX_LIBS    = $(GSTREAMER_LIBS) $(PTHREAD_LIBS)

enum broadwaydrv_funcs
{
    unix_init,
    unix_funcs_count
};

#define BROADWAYDRV_CALL(func, params) WINE_UNIX_CALL( unix_ ## func, params )

/* broadwaydrv_init params */
struct init_params
{
    WNDPROC foreign_window_proc;
};

/* driver client callbacks exposed with KernelCallbackTable interface */
enum broadwaydrv_client_funcs
{
    client_func_callback = NtUserDriverCallbackFirst,
    client_func_last
};

C_ASSERT( client_func_last <= NtUserDriverCallbackLast + 1 );

/* simplified interface for client callbacks requiring only a single UINT parameter */
enum client_callback
{
    client_funcs_count
};

/* broadwaydrv_callback params */
struct client_callback_params
{
    UINT id;
    UINT arg;
};

