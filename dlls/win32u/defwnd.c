/*
 * Default window procedure
 *
 * Copyright 1993, 1996 Alexandre Julliard
 * Copyright 1995 Alex Korobka
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

#if 0
#pragma makedep unix
#endif

#include "win32u_private.h"
#include "ntuser_private.h"
#include "wine/server.h"

WINE_DEFAULT_DEBUG_CHANNEL(win);


/***********************************************************************
 *           AdjustWindowRectEx (win32u.so)
 */
BOOL WINAPI AdjustWindowRectEx( RECT *rect, DWORD style, BOOL menu, DWORD ex_style )
{
    NONCLIENTMETRICSW ncm;
    int adjust = 0;

    ncm.cbSize = sizeof(ncm);
    NtUserSystemParametersInfo( SPI_GETNONCLIENTMETRICS, 0, &ncm, 0 );

    if ((ex_style & (WS_EX_STATICEDGE|WS_EX_DLGMODALFRAME)) == WS_EX_STATICEDGE)
        adjust = 1; /* for the outer frame always present */
    else if ((ex_style & WS_EX_DLGMODALFRAME) || (style & (WS_THICKFRAME|WS_DLGFRAME)))
        adjust = 2; /* outer */

    if (style & WS_THICKFRAME)
        adjust += ncm.iBorderWidth + ncm.iPaddedBorderWidth; /* The resize border */

    if ((style & (WS_BORDER|WS_DLGFRAME)) || (ex_style & WS_EX_DLGMODALFRAME))
        adjust++; /* The other border */

    InflateRect( rect, adjust, adjust );

    if ((style & WS_CAPTION) == WS_CAPTION)
    {
        if (ex_style & WS_EX_TOOLWINDOW)
            rect->top -= ncm.iSmCaptionHeight + 1;
        else
            rect->top -= ncm.iCaptionHeight + 1;
    }
    if (menu) rect->top -= ncm.iMenuHeight + 1;

    if (ex_style & WS_EX_CLIENTEDGE)
        InflateRect( rect, get_system_metrics(SM_CXEDGE), get_system_metrics(SM_CYEDGE) );
    return TRUE;
}

static BOOL set_window_text( HWND hwnd, const void *text, BOOL ansi )
{
    static const WCHAR emptyW[] = { 0 };
    WCHAR *str;
    WND *win;

    /* check for string, as static icons, bitmaps (SS_ICON, SS_BITMAP)
     * may have child window IDs instead of window name */
    if (text && IS_INTRESOURCE(text)) return FALSE;

    if (text)
    {
        if (ansi) str = towstr( text );
        else str = wcsdup( text );
        if (!str) return FALSE;
    }
    else str = NULL;

    TRACE( "%s\n", debugstr_w(str) );

    if (!(win = get_win_ptr( hwnd )))
    {
        free( str );
        return FALSE;
    }

    free( win->text );
    win->text = str;
    SERVER_START_REQ( set_window_text )
    {
        req->handle = wine_server_user_handle( hwnd );
        if (str) wine_server_add_data( req, str, lstrlenW( str ) * sizeof(WCHAR) );
        wine_server_call( req );
    }
    SERVER_END_REQ;

    release_win_ptr( win );

    user_driver->pSetWindowText( hwnd, str ? str : emptyW );

    return TRUE;
}

static HICON get_window_icon( HWND hwnd, WPARAM type )
{
    HICON ret;
    WND *win;

    if (!(win = get_win_ptr( hwnd ))) return 0;

    switch(type)
    {
    case ICON_SMALL:
        ret = win->hIconSmall;
        break;
    case ICON_BIG:
        ret = win->hIcon;
        break;
    case ICON_SMALL2:
        ret = win->hIconSmall ? win->hIconSmall : win->hIconSmall2;
        break;
    default:
        ret = 0;
        break;
    }

    release_win_ptr( win );
    return ret;
}

static HICON set_window_icon( HWND hwnd, WPARAM type, HICON icon )
{
    HICON ret = 0;
    WND *win;

    if (!(win = get_win_ptr( hwnd ))) return 0;

    switch (type)
    {
    case ICON_SMALL:
        ret = win->hIconSmall;
        if (ret && !icon && win->hIcon)
        {
            win->hIconSmall2 = CopyImage( win->hIcon, IMAGE_ICON,
                                          get_system_metrics( SM_CXSMICON ),
                                          get_system_metrics( SM_CYSMICON ), 0 );
        }
        else if (icon && win->hIconSmall2)
        {
            NtUserDestroyCursor( win->hIconSmall2, 0 );
            win->hIconSmall2 = NULL;
        }
        win->hIconSmall = icon;
        break;

    case ICON_BIG:
        ret = win->hIcon;
        if (win->hIconSmall2)
        {
            NtUserDestroyCursor( win->hIconSmall2, 0 );
            win->hIconSmall2 = NULL;
        }
        if (icon && !win->hIconSmall)
        {
            win->hIconSmall2 = CopyImage( icon, IMAGE_ICON,
                                          get_system_metrics( SM_CXSMICON ),
                                          get_system_metrics( SM_CYSMICON ), 0 );
        }
        win->hIcon = icon;
        break;
    }
    release_win_ptr( win );

    user_driver->pSetWindowIcon( hwnd, type, icon );
    return ret;
}

static LRESULT handle_sys_command( HWND hwnd, WPARAM wparam, LPARAM lparam )
{
    if (!is_window_enabled( hwnd )) return 0;

    if (call_hooks( WH_CBT, HCBT_SYSCOMMAND, wparam, lparam, TRUE ))
        return 0;

    if (!user_driver->pSysCommand( hwnd, wparam, lparam ))
        return 0;

    switch (wparam & 0xfff0)
    {
    case SC_MINIMIZE:
        show_owned_popups( hwnd, FALSE );
        NtUserShowWindow( hwnd, SW_MINIMIZE );
        break;

    case SC_MAXIMIZE:
        if (is_iconic(hwnd)) show_owned_popups( hwnd, TRUE );
        NtUserShowWindow( hwnd, SW_MAXIMIZE );
        break;

    case SC_RESTORE:
        if (is_iconic( hwnd )) show_owned_popups( hwnd, TRUE );
        NtUserShowWindow( hwnd, SW_RESTORE );
        break;

    default:
        return 1; /* handle on client side */
    }
    return 0;
}

LRESULT default_window_proc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, BOOL ansi )
{
    LRESULT result = 0;

    switch (msg)
    {
    case WM_NCCREATE:
        if (lparam)
        {
            CREATESTRUCTW *cs = (CREATESTRUCTW *)lparam;
            set_window_text( hwnd, cs->lpszName, ansi );
            result = 1;
        }
        break;

    case WM_NCDESTROY:
        {
            WND *win = get_win_ptr( hwnd );
            if (!win) return 0;
            free( win->text );
            win->text = NULL;
            if (user_callbacks) user_callbacks->free_win_ptr( win );
            win->pScroll = NULL;
            release_win_ptr( win );
            break;
        }

    case WM_PAINTICON:
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = NtUserBeginPaint( hwnd, &ps );
            if (hdc)
            {
                HICON icon;
                if (is_iconic(hwnd) && ((icon = UlongToHandle( get_class_long( hwnd, GCLP_HICON, FALSE )))))
                {
                    RECT rc;
                    int x, y;

                    get_client_rect( hwnd, &rc );
                    x = (rc.right - rc.left - get_system_metrics( SM_CXICON )) / 2;
                    y = (rc.bottom - rc.top - get_system_metrics( SM_CYICON )) / 2;
                    TRACE( "Painting class icon: vis rect=(%s)\n", wine_dbgstr_rect(&ps.rcPaint) );
                    NtUserDrawIconEx( hdc, x, y, icon, 0, 0, 0, 0, DI_NORMAL | DI_COMPAT | DI_DEFAULTSIZE );
                }
                NtUserEndPaint( hwnd, &ps );
            }
            break;
        }

    case WM_SETTEXT:
        result = set_window_text( hwnd, (void *)lparam, ansi );
        break;

    case WM_SETICON:
        result = (LRESULT)set_window_icon( hwnd, wparam, (HICON)lparam );
        break;

    case WM_GETICON:
        result = (LRESULT)get_window_icon( hwnd, wparam );
        break;

    case WM_SYSCOMMAND:
        result = handle_sys_command( hwnd, wparam, lparam );
        break;
    }

    return result;
}
