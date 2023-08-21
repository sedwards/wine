#include "config.h"

#define WIN32_NO_STATUS

#include "wingdi.h"
#include "wine/gdi_driver.h"
#include "driver.h"

void initDesktop(HWND hwnd) {
    unsigned int width, height;

    TRACE("%p\n", hwnd);

    /* retrieve the real size of the desktop */
    SERVER_START_REQ(get_window_rectangles)
    {
        req->handle = wine_server_user_handle(hwnd);
        req->relative = COORDS_CLIENT;
        wine_server_call(req);
        width  = reply->window.right;
        height = reply->window.bottom;
    }
    SERVER_END_REQ;

    if (!width && !height)  /* not initialized yet */
    {
        width = internal_GetDeviceCaps(DESKTOPHORZRES);
        height = internal_GetDeviceCaps(DESKTOPVERTRES);

        SERVER_START_REQ(set_window_pos)
        {
            req->handle        = wine_server_user_handle(hwnd);
            req->previous      = 0;
            req->swp_flags     = SWP_NOZORDER;
            req->window.left   = 0;
            req->window.top    = 0;
            req->window.right  = width;
            req->window.bottom = height;
            req->client        = req->window;
            wine_server_call(req);
        }
        SERVER_END_REQ;
    }
}

BOOL WINAPI boxeddrv_CreateDesktopWindow(HWND hwnd) {
    int result;
    initDesktop(hwnd);
    return (BOOL)result;
}

BOOL WINAPI boxeddrv_CreateWindow(HWND hwnd) {
    int result;
    TRACE("hwnd=%p result=%d\n", hwnd, result);
    return (BOOL)result;
}

/***********************************************************************
 *           X11DRV_CreateDesktop
 *
 * Create the X11 desktop window for the desktop mode.
 */
BOOL X11DRV_CreateDesktop( const WCHAR *name, UINT width, UINT height )
{
    XSetWindowAttributes win_attr;
    Window win;
    Display *display = thread_init_display();

    TRACE( "%s %ux%u\n", debugstr_w(name), width, height );

    X11DRV_init_desktop( win, width, height );
    return TRUE;
}
#endif
