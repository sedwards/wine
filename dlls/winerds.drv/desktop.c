/*
 * RDS Desktop and Window Management - Win32 Version
 *
 * Minimal window management for virtual desktop
 */

#include "rdsdrv_dll.h"
#include "rdsgdi_driver.h" 
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(rdsdrv);

static HWND rds_desktop_window = NULL;

/***********************************************************************
 *           RDS_DesktopWindowProc
 *
 * Desktop window procedure
 */
LRESULT RDS_DesktopWindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    TRACE("DesktopWindowProc: hwnd=%p, msg=0x%x, wp=0x%lx, lp=0x%lx\n", 
          hwnd, msg, wp, lp);

    switch (msg)
    {
        case WM_NCCREATE:
            TRACE("Desktop window created: %p\n", hwnd);
            return TRUE;

        case WM_DISPLAYCHANGE:
            TRACE("Display change notification\n");
            return 0;

        case WM_SETTINGCHANGE:
            TRACE("Setting change notification\n");
            return 0;
    }

    return DefWindowProcW(hwnd, msg, wp, lp);
}

/***********************************************************************
 *           RDS_SetDesktopWindow
 *
 * Set the desktop window
 */
void RDS_SetDesktopWindow(HWND hwnd)
{
    TRACE("SetDesktopWindow: %p\n", hwnd);
    rds_desktop_window = hwnd;
}

/***********************************************************************
 *           RDS_CreateWindow
 *
 * Handle window creation
 */
BOOL RDS_CreateWindow(HWND hwnd)
{
    TRACE("CreateWindow: %p\n", hwnd);
    return TRUE;
}

/***********************************************************************
 *           RDS_DestroyWindow
 *
 * Handle window destruction
 */
void RDS_DestroyWindow(HWND hwnd)
{
    TRACE("DestroyWindow: %p\n", hwnd);
}

/***********************************************************************
 *           RDS_GetDC
 *
 * Get device context for window
 */
void RDS_GetDC(HDC hdc, HWND hwnd, HWND top_win, const RECT *win_rect,
               const RECT *top_rect, DWORD flags)
{
    TRACE("GetDC: hdc=%p, hwnd=%p\n", hdc, hwnd);
}

/***********************************************************************
 *           RDS_ReleaseDC
 *
 * Release device context
 */
void RDS_ReleaseDC(HWND hwnd, HDC hdc)
{
    TRACE("ReleaseDC: hwnd=%p, hdc=%p\n", hwnd, hdc);
}

/***********************************************************************
 *           RDS_WindowMessage
 *
 * Handle window messages
 */
LRESULT RDS_WindowMessage(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    TRACE("WindowMessage: hwnd=%p, msg=0x%x\n", hwnd, msg);
    return 0;
}

/***********************************************************************
 *           RDS_WindowPosChanging
 *
 * Handle window position changes
 */
BOOL RDS_WindowPosChanging(HWND hwnd, UINT flags, BOOL shaped,
                           const struct window_rects *window_rects)
{
    TRACE("WindowPosChanging: hwnd=%p, flags=0x%x\n", hwnd, flags);
    return TRUE;
}

/***********************************************************************
 *           RDS_WindowPosChanged
 *
 * Handle window position changed
 */
void RDS_WindowPosChanged(HWND hwnd, HWND insert_after, HWND owner,
                          UINT flags, BOOL shaped,
                          const struct window_rects *window_rects,
                          struct window_surface *surface)
{
    TRACE("WindowPosChanged: hwnd=%p\n", hwnd);
}

/***********************************************************************
 *           RDS_CreateWindowSurface
 *
 * Create a window surface
 */
BOOL RDS_CreateWindowSurface(HWND hwnd, BOOL layered, const RECT *surface_rect,
                             struct window_surface **surface)
{
    TRACE("CreateWindowSurface: hwnd=%p, layered=%d\n", hwnd, layered);
    
    *surface = NULL;  /* No surface needed for our virtual driver */
    return TRUE;
}

/***********************************************************************
 *           RDS_ShowWindow
 *
 * Show/hide window
 */
UINT RDS_ShowWindow(HWND hwnd, INT cmd, RECT *rect, UINT swp)
{
    TRACE("ShowWindow: hwnd=%p, cmd=%d\n", hwnd, cmd);
    return swp;
}

/***********************************************************************
 *           RDS_SetWindowStyle
 *
 * Set window style
 */
void RDS_SetWindowStyle(HWND hwnd, INT type, STYLESTRUCT *style)
{
    TRACE("SetWindowStyle: hwnd=%p, type=%d\n", hwnd, type);
}

/***********************************************************************
 *           RDS_SetParent
 *
 * Set window parent
 */
void RDS_SetParent(HWND hwnd, HWND parent, HWND old_parent)
{
    TRACE("SetParent: hwnd=%p, parent=%p, old_parent=%p\n", hwnd, parent, old_parent);
}

