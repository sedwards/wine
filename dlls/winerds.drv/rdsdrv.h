/*
 * X11 driver definitions
 *
 * Copyright 1996 Alexandre Julliard
 * Copyright 1999 Patrik Stridvall
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

#ifndef __WINE_RDSDRV_H
#define __WINE_RDSDRV_H

#ifndef __WINE_CONFIG_H
# error You must include config.h to use this header
#endif

#include <limits.h>
#include <stdarg.h>
#include <stdlib.h>
#endif

#include "windef.h"
#include "winbase.h"
#include "ntgdi.h"
#include "wine/gdi_driver.h"
#include "unixlib.h"
#include "wine/list.h"
#include "wine/debug.h"

#define MAX_DASHLEN 16

#define WINE_XDND_VERSION 5

/* These may prove useful in the future */
#if 0
static inline void reset_bounds( RECT *bounds )
{
    bounds->left = bounds->top = INT_MAX;
    bounds->right = bounds->bottom = INT_MIN;
}

static inline void add_bounds_rect( RECT *bounds, const RECT *rect )
{
    if (rect->left >= rect->right || rect->top >= rect->bottom) return;
    bounds->left   = min( bounds->left, rect->left );
    bounds->top    = min( bounds->top, rect->top );
    bounds->right  = max( bounds->right, rect->right );
    bounds->bottom = max( bounds->bottom, rect->bottom );
}

/* GDI helpers */

static inline BOOL lp_to_dp( HDC hdc, POINT *points, INT count )
{
    return NtGdiTransformPoints( hdc, points, points, count, NtGdiLPtoDP );
}

static inline UINT get_palette_entries( HPALETTE palette, UINT start, UINT count, PALETTEENTRY *entries )
{
    return NtGdiDoPalette( palette, start, count, entries, NtGdiGetPaletteEntries, TRUE );
}

/* user helpers */

static inline LRESULT send_message( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    return NtUserMessageCall( hwnd, msg, wparam, lparam, NULL, NtUserSendMessage, FALSE );
}

static inline LRESULT send_message_timeout( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam,
                                            UINT flags, UINT timeout, PDWORD_PTR res_ptr )
{
    struct send_message_timeout_params params = { .flags = flags, .timeout = timeout };
    LRESULT res = NtUserMessageCall( hwnd, msg, wparam, lparam, &params,
                                     NtUserSendMessageTimeout, FALSE );
    if (res_ptr) *res_ptr = params.result;
    return res;
}

static inline BOOL send_notify_message( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    return NtUserMessageCall( hwnd, msg, wparam, lparam, 0, NtUserSendNotifyMessage, FALSE );
}

static inline HWND get_focus(void)
{
    GUITHREADINFO info;
    info.cbSize = sizeof(info);
    return NtUserGetGUIThreadInfo( GetCurrentThreadId(), &info ) ? info.hwndFocus : 0;
}

static inline HWND get_active_window(void)
{
    GUITHREADINFO info;
    info.cbSize = sizeof(info);
    return NtUserGetGUIThreadInfo( GetCurrentThreadId(), &info ) ? info.hwndActive : 0;
}

static inline BOOL intersect_rect( RECT *dst, const RECT *src1, const RECT *src2 )
{
    dst->left   = max( src1->left, src2->left );
    dst->top    = max( src1->top, src2->top );
    dst->right  = min( src1->right, src2->right );
    dst->bottom = min( src1->bottom, src2->bottom );
    return !IsRectEmpty( dst );
}

/* registry helpers */

extern HKEY open_hkcu_key( const char *name );
extern ULONG query_reg_value( HKEY hkey, const WCHAR *name,
                              KEY_VALUE_PARTIAL_INFORMATION *info, ULONG size );
extern HKEY reg_open_key( HKEY root, const WCHAR *name, ULONG name_len );

/* string helpers */

static inline void ascii_to_unicode( WCHAR *dst, const char *src, size_t len )
{
    while (len--) *dst++ = (unsigned char)*src++;
}

static inline UINT asciiz_to_unicode( WCHAR *dst, const char *src )
{
    WCHAR *p = dst;
    while ((*p++ = *src++));
    return (p - dst) * sizeof(WCHAR);
}

#endif  /* __WINE_RDS_H */
