/*
 * RDSDRV initialization code - Registry-based approach
 *
 * Wine drivers are loaded by registry entries, not by calling
 * __wine_set_user_driver directly. We need to provide the standard
 * entry points that Wine expects.
 */

#include "rdsdrv_dll.h"
#include "rdsgdi_driver.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(rdsdrv);

/* Forward declarations for functions from other files */
UINT RDS_UpdateDisplayDevices(const struct gdi_device_manager *device_manager, void *param);
LONG RDS_ChangeDisplaySettings(LPDEVMODEW devmode, LPCWSTR device_name, HWND hwnd, DWORD flags, LPVOID lpvoid);
BOOL RDS_CreateDesktop(const WCHAR *name, UINT width, UINT height);
BOOL RDS_ProcessEvents(DWORD timeout);

LRESULT RDS_DesktopWindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
void RDS_SetDesktopWindow(HWND hwnd);
BOOL RDS_CreateWindow(HWND hwnd);
void RDS_DestroyWindow(HWND hwnd);
void RDS_GetDC(HDC hdc, HWND hwnd, HWND top_win, const RECT *win_rect, const RECT *top_rect, DWORD flags);
void RDS_ReleaseDC(HWND hwnd, HDC hdc);
LRESULT RDS_WindowMessage(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
BOOL RDS_WindowPosChanging(HWND hwnd, UINT flags, BOOL shaped, const struct window_rects *window_rects);
void RDS_WindowPosChanged(HWND hwnd, HWND insert_after, HWND owner, UINT flags, BOOL shaped, const struct window_rects *window_rects, struct window_surface *surface);
BOOL RDS_CreateWindowSurface(HWND hwnd, BOOL layered, const RECT *surface_rect, struct window_surface **surface);
UINT RDS_ShowWindow(HWND hwnd, INT cmd, RECT *rect, UINT swp);
void RDS_SetWindowStyle(HWND hwnd, INT type, STYLESTRUCT *style);
void RDS_SetParent(HWND hwnd, HWND parent, HWND old_parent);

/* Forward declarations for GDI functions */
BOOL RDS_MoveTo(PHYSDEV dev, INT x, INT y);
BOOL RDS_LineTo(PHYSDEV dev, INT x, INT y);
BOOL RDS_Rectangle(PHYSDEV dev, INT left, INT top, INT right, INT bottom);
BOOL RDS_ExtTextOut(PHYSDEV dev, INT x, INT y, UINT flags, const RECT *rect, LPCWSTR str, UINT count, const INT *dx);

/* Forward declarations for GDI initialization from gdi_funcs.c */
void RDS_InitializeGDI(void);
void RDS_CleanupGDI(void);

/* Stub functions for required but unused functionality */
static BOOL RDS_GetCursorPos(LPPOINT pos)
{
    if (pos) {
        pos->x = 0;
        pos->y = 0;
    }
    return TRUE;
}

static BOOL RDS_SetCursorPos(INT x, INT y)
{
    return TRUE;
}

static void RDS_SetCursor(HWND hwnd, HCURSOR cursor)
{
}

static BOOL RDS_ClipCursor(const RECT *rect, BOOL reset)
{
    return TRUE;
}

/* GDI function table */
static const struct gdi_dc_funcs rds_dc_funcs =
{
    .pMoveTo = RDS_MoveTo,
    .pLineTo = RDS_LineTo,
    .pRectangle = RDS_Rectangle,
    .pExtTextOut = RDS_ExtTextOut,
    .priority = GDI_PRIORITY_GRAPHICS_DRV,
};

/* Main driver function table */
static const struct user_driver_funcs rdsdrv_funcs =
{
    .dc_funcs = rds_dc_funcs,
    
    /* Display management */
    .pUpdateDisplayDevices = RDS_UpdateDisplayDevices,
    .pChangeDisplaySettings = RDS_ChangeDisplaySettings,
    .pCreateDesktop = RDS_CreateDesktop,
    .pProcessEvents = RDS_ProcessEvents,
    
    /* Desktop and window management */
    .pDesktopWindowProc = RDS_DesktopWindowProc,
    .pSetDesktopWindow = RDS_SetDesktopWindow,
    .pCreateWindow = RDS_CreateWindow,
    .pDestroyWindow = RDS_DestroyWindow,
    .pGetDC = RDS_GetDC,
    .pReleaseDC = RDS_ReleaseDC,
    .pWindowMessage = RDS_WindowMessage,
    .pWindowPosChanging = RDS_WindowPosChanging,
    .pWindowPosChanged = RDS_WindowPosChanged,
    .pCreateWindowSurface = RDS_CreateWindowSurface,
    .pShowWindow = RDS_ShowWindow,
    .pSetWindowStyle = RDS_SetWindowStyle,
    .pSetParent = RDS_SetParent,
    
    /* Cursor management (stubs) */
    .pGetCursorPos = RDS_GetCursorPos,
    .pSetCursorPos = RDS_SetCursorPos,
    .pSetCursor = RDS_SetCursor,
    .pClipCursor = RDS_ClipCursor,
};

#if 0
/***********************************************************************
 *           wine_get_user_driver    (winerds.drv.@)
 *
 * This is the standard entry point that Wine calls to get driver functions.
 * This is how winex11.drv and winewayland.drv work.
 */
const struct user_driver_funcs * CDECL wine_get_user_driver( UINT version )
{
    TRACE("wine_get_user_driver called with version %u\n", version);
    
    if (version != WINE_GDI_DRIVER_VERSION)
    {
        ERR("Invalid driver version %u, expected %u\n", version, WINE_GDI_DRIVER_VERSION);
        return NULL;
    }
    
    TRACE("Returning RDS driver function table\n");
    return &rdsdrv_funcs;
}

/***********************************************************************
 *           DllMain
 *
 * Driver initialization - this runs in Win32 context
 */
BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, void *reserved)
{
    switch (reason)
    {
        case DLL_PROCESS_ATTACH:
            TRACE("RDS driver initializing (registry-based mode)\n");
            
            /* Initialize GDI subsystem */
            RDS_InitializeGDI();
            
            TRACE("RDS driver initialized successfully\n");
            break;
            
        case DLL_PROCESS_DETACH:
            TRACE("RDS driver unloading\n");
            
            /* Cleanup GDI subsystem */
            RDS_CleanupGDI();
            break;
    }
    
    return TRUE;
}
#endif

// Update your EXISTING wine_get_user_driver function in winerdsdrv_main.c:
// Replace the current one with this version:

const struct user_driver_funcs * CDECL wine_get_user_driver( UINT version )
{
    printf("*** WINE_GET_USER_DRIVER CALLED! Version: %u ***\n", version);
    
    TRACE("wine_get_user_driver called with version %u\n", version);
    
    if (version != WINE_GDI_DRIVER_VERSION)
    {
        ERR("Invalid driver version %u, expected %u\n", version, WINE_GDI_DRIVER_VERSION);
        return NULL;
    }
    
    printf("*** RETURNING RDS DRIVER FUNCTION TABLE ***\n");
    TRACE("Returning RDS driver function table\n");
    return &rdsdrv_funcs;
}

// Update your EXISTING DllMain function in winerdsdrv_main.c:
// Replace the current one with this version:

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, void *reserved)
{
    switch (reason)
    {
        case DLL_PROCESS_ATTACH:
            printf("*** WINERDS.DRV IS LOADING! ***\n");
            
            TRACE("RDS driver initializing\n");
            RDS_InitializeGDI();
            TRACE("RDS driver initialized successfully\n");
            break;
            
        case DLL_PROCESS_DETACH:
            printf("*** WINERDS.DRV IS UNLOADING! ***\n");
            TRACE("RDS driver unloading\n");
            RDS_CleanupGDI();
            break;
    }
    
    return TRUE;
}

// Now test with:
// 1. Register driver:
//    wine reg add "HKEY_CURRENT_USER\\Software\\Wine\\Drivers" /v Graphics /t REG_SZ /d "winerds" /f
//
// 2. Run with force loading:
//    WINEDLLOVERRIDES="winerds=n,b" wine programs/termsrv/x86_64-windows/termsrv.exe
//
// You should see:
//    *** WINERDS.DRV IS LOADING! ***
//    *** WINE_GET_USER_DRIVER CALLED! Version: 104 ***
//    *** RETURNING RDS DRIVER FUNCTION TABLE ***
//
