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
#include "shlobj.h"
#include "wine/unixlib.h"

enum rdsdrv_unix_funcs
{
    rdsdrv_unix_func_init,
    rdsdrv_unix_func_read_events,
    rdsdrv_unix_func_init_clipboard,
    rdsdrv_unix_func_count,
};

#define RDSDRV_CALL(func, params) WINE_UNIX_CALL( unix_ ## func, params )

