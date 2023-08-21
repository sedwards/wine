/*
 * Mac graphics driver initialisation functions
 *
 * Copyright 1996 Alexandre Julliard
 * Copyright 2011, 2012, 2013 Ken Thomases for CodeWeavers, Inc.
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

#include "config.h"

#include <stdarg.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "ntgdi.h"
#include "wine/gdi_driver.h"

#include "broadwaydrv.h"

#include "unixlib.h"
#include "wine/list.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(broadway);

BOOL BROADWAYDRV_UpdateDisplayDevices( const struct gdi_device_manager *device_manager, BOOL force, void *param );
LONG BROADWAYDRV_ChangeDisplaySettings( LPDEVMODEW displays, LPCWSTR primary_name, HWND hwnd, DWORD flags, LPVOID lpvoid );
BOOL BROADWAYDRV_GetCurrentDisplaySettings( LPCWSTR name, BOOL is_primary, LPDEVMODEW devmode );
BOOL BROADWAYDRV_CreateDC(PHYSDEV *pdev, LPCWSTR device, LPCWSTR output, const DEVMODEW* initData);
BOOL BROADWAYDRV_CreateCompatibleDC(PHYSDEV orig, PHYSDEV *pdev);
BOOL BROADWAYDRV_DeleteDC(PHYSDEV dev);
INT BROADWAYDRV_GetDeviceCaps(PHYSDEV dev, INT cap);

unsigned int screen_width = 1024;
unsigned int screen_height = 768;
static const unsigned int screen_bpp = 32;  /* we don't support other modes */
static RECT monitor_rc_work;
RECT virtual_screen_rect = { 0, 0, 0, 0 };

//PNTAPCFUNC register_window_callback;

typedef struct
{
    struct gdi_physdev  dev;
} BROADWAY_PDEVICE;

static inline BROADWAY_PDEVICE *get_broadway_dev(PHYSDEV dev)
{
    return (BROADWAY_PDEVICE*)dev;
}
static const struct user_driver_funcs broadwaydrv_funcs;

static const struct user_driver_funcs broadwaydrv_funcs =
{
#if 0
    .dc_funcs.pArc = X11DRV_Arc,
    .dc_funcs.pChord = X11DRV_Chord,
    .dc_funcs.pCreateCompatibleDC = X11DRV_CreateCompatibleDC,
    .dc_funcs.pCreateDC = X11DRV_CreateDC,
    .dc_funcs.pDeleteDC = X11DRV_DeleteDC,
    .dc_funcs.pEllipse = X11DRV_Ellipse,
    .dc_funcs.pExtEscape = X11DRV_ExtEscape,
    .dc_funcs.pExtFloodFill = X11DRV_ExtFloodFill,
    .dc_funcs.pFillPath = X11DRV_FillPath,
    .dc_funcs.pGetDeviceGammaRamp = X11DRV_GetDeviceGammaRamp,
    .dc_funcs.pGetICMProfile = X11DRV_GetICMProfile,
    .dc_funcs.pGetImage = X11DRV_GetImage,
    .dc_funcs.pGetNearestColor = X11DRV_GetNearestColor,
    .dc_funcs.pGetSystemPaletteEntries = X11DRV_GetSystemPaletteEntries,
    .dc_funcs.pGradientFill = X11DRV_GradientFill,
    .dc_funcs.pLineTo = X11DRV_LineTo,
    .dc_funcs.pPaintRgn = X11DRV_PaintRgn,
    .dc_funcs.pPatBlt = X11DRV_PatBlt,
    .dc_funcs.pPie = X11DRV_Pie,
    .dc_funcs.pPolyPolygon = X11DRV_PolyPolygon,
    .dc_funcs.pPolyPolyline = X11DRV_PolyPolyline,
    .dc_funcs.pPutImage = X11DRV_PutImage,
    .dc_funcs.pRealizeDefaultPalette = X11DRV_RealizeDefaultPalette,
    .dc_funcs.pRealizePalette = X11DRV_RealizePalette,
    .dc_funcs.pRectangle = X11DRV_Rectangle,
    .dc_funcs.pRoundRect = X11DRV_RoundRect,
    .dc_funcs.pSelectBrush = X11DRV_SelectBrush,
    .dc_funcs.pSelectFont = X11DRV_SelectFont,
    .dc_funcs.pSelectPen = X11DRV_SelectPen,
    .dc_funcs.pSetBoundsRect = X11DRV_SetBoundsRect,
    .dc_funcs.pSetDCBrushColor = X11DRV_SetDCBrushColor,
    .dc_funcs.pSetDCPenColor = X11DRV_SetDCPenColor,
    .dc_funcs.pSetDeviceClipping = X11DRV_SetDeviceClipping,
    .dc_funcs.pSetDeviceGammaRamp = X11DRV_SetDeviceGammaRamp,
    .dc_funcs.pSetPixel = X11DRV_SetPixel,
    .dc_funcs.pStretchBlt = X11DRV_StretchBlt,
    .dc_funcs.pStrokeAndFillPath = X11DRV_StrokeAndFillPath,
    .dc_funcs.pStrokePath = X11DRV_StrokePath,
    .dc_funcs.pUnrealizePalette = X11DRV_UnrealizePalette,
    .dc_funcs.pD3DKMTCheckVidPnExclusiveOwnership = X11DRV_D3DKMTCheckVidPnExclusiveOwnership,
    .dc_funcs.pD3DKMTCloseAdapter = X11DRV_D3DKMTCloseAdapter,
    .dc_funcs.pD3DKMTOpenAdapterFromLuid = X11DRV_D3DKMTOpenAdapterFromLuid,
    .dc_funcs.pD3DKMTQueryVideoMemoryInfo = X11DRV_D3DKMTQueryVideoMemoryInfo,
    .dc_funcs.pD3DKMTSetVidPnSourceOwner = X11DRV_D3DKMTSetVidPnSourceOwner,
    .dc_funcs.priority = GDI_PRIORITY_GRAPHICS_DRV,

    .pActivateKeyboardLayout = X11DRV_ActivateKeyboardLayout,
    .pBeep = X11DRV_Beep,
    .pGetKeyNameText = X11DRV_GetKeyNameText,
    .pMapVirtualKeyEx = X11DRV_MapVirtualKeyEx,
    .pToUnicodeEx = X11DRV_ToUnicodeEx,
    .pVkKeyScanEx = X11DRV_VkKeyScanEx,
    .pImeToAsciiEx = X11DRV_ImeToAsciiEx,
    .pNotifyIMEStatus = X11DRV_NotifyIMEStatus,
    .pDestroyCursorIcon = X11DRV_DestroyCursorIcon,
    .pSetCursor = X11DRV_SetCursor,
    .pGetCursorPos = X11DRV_GetCursorPos,
    .pSetCursorPos = X11DRV_SetCursorPos,
    .pClipCursor = X11DRV_ClipCursor,
    .pGetCurrentDisplaySettings = X11DRV_GetCurrentDisplaySettings,
    .pGetDisplayDepth = X11DRV_GetDisplayDepth,
    .pCreateDesktop = X11DRV_CreateDesktop,
    .pCreateWindow = X11DRV_CreateWindow,
    .pDesktopWindowProc = X11DRV_DesktopWindowProc,
    .pDestroyWindow = X11DRV_DestroyWindow,
    .pFlashWindowEx = X11DRV_FlashWindowEx,
    .pGetDC = X11DRV_GetDC,
    .pProcessEvents = X11DRV_ProcessEvents,
    .pReleaseDC = X11DRV_ReleaseDC,
    .pScrollDC = X11DRV_ScrollDC,
    .pSetCapture = X11DRV_SetCapture,
    .pSetDesktopWindow = X11DRV_SetDesktopWindow,
    .pSetFocus = X11DRV_SetFocus,
    .pSetLayeredWindowAttributes = X11DRV_SetLayeredWindowAttributes,
    .pSetParent = X11DRV_SetParent,
    .pSetWindowIcon = X11DRV_SetWindowIcon,
    .pSetWindowRgn = X11DRV_SetWindowRgn,
    .pSetWindowStyle = X11DRV_SetWindowStyle,
    .pSetWindowText = X11DRV_SetWindowText,
    .pShowWindow = X11DRV_ShowWindow,
    .pSysCommand = X11DRV_SysCommand,
    .pClipboardWindowProc = X11DRV_ClipboardWindowProc,
    .pUpdateClipboard = X11DRV_UpdateClipboard,
    .pUpdateLayeredWindow = X11DRV_UpdateLayeredWindow,
    .pWindowMessage = X11DRV_WindowMessage,
    .pWindowPosChanging = X11DRV_WindowPosChanging,
    .pWindowPosChanged = X11DRV_WindowPosChanged,
    .pwine_get_wgl_driver = X11DRV_wine_get_wgl_driver,
    .pThreadDetach = X11DRV_ThreadDetach,
#endif
    .dc_funcs.pCreateCompatibleDC = BROADWAYDRV_CreateCompatibleDC,
    .dc_funcs.pCreateDC = BROADWAYDRV_CreateDC,
    .dc_funcs.pDeleteDC = BROADWAYDRV_DeleteDC,
    .dc_funcs.pGetDeviceCaps = BROADWAYDRV_GetDeviceCaps,
    .dc_funcs.priority = GDI_PRIORITY_GRAPHICS_DRV,

    .pChangeDisplaySettings = BROADWAYDRV_ChangeDisplaySettings,
    .pGetCurrentDisplaySettings = BROADWAYDRV_GetCurrentDisplaySettings,
    .pUpdateDisplayDevices = BROADWAYDRV_UpdateDisplayDevices,
};

void init_user_driver(void)
{
    ERR("Inside init_user_driver\n");
    __wine_set_user_driver( &broadwaydrv_funcs, WINE_GDI_DRIVER_VERSION );
}

/***********************************************************************
 *           BROADWAYDRV_UpdateDisplayDevices
 */
BOOL BROADWAYDRV_UpdateDisplayDevices( const struct gdi_device_manager *device_manager, BOOL force, void *param )
{
    if (force)
    {
        static const struct gdi_gpu gpu;
        static const struct gdi_adapter adapter =
        {
            .state_flags = DISPLAY_DEVICE_ATTACHED_TO_DESKTOP | DISPLAY_DEVICE_PRIMARY_DEVICE | DISPLAY_DEVICE_VGA_COMPATIBLE,
        };
        struct gdi_monitor gdi_monitor =
        {
            .rc_monitor = virtual_screen_rect,
            .rc_work = monitor_rc_work,
            .state_flags = DISPLAY_DEVICE_ACTIVE | DISPLAY_DEVICE_ATTACHED,
        };
        const DEVMODEW mode =
        {
            .dmFields = DM_DISPLAYORIENTATION | DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL |
                        DM_DISPLAYFLAGS | DM_DISPLAYFREQUENCY | DM_POSITION,
            .dmBitsPerPel = screen_bpp, .dmPelsWidth = screen_width, .dmPelsHeight = screen_height, .dmDisplayFrequency = 60,
        };
        device_manager->add_gpu( &gpu, param );
        device_manager->add_adapter( &adapter, param );
        device_manager->add_monitor( &gdi_monitor, param );
        device_manager->add_mode( &mode, TRUE, param );
    }

    return TRUE;
}

/***********************************************************************
 *           BROADWAYDRV_GetCurrentDisplaySettings
 */
BOOL BROADWAYDRV_GetCurrentDisplaySettings( LPCWSTR name, BOOL is_primary, LPDEVMODEW devmode )
{
    devmode->dmDisplayFlags = 0;
    devmode->dmPosition.x = 0;
    devmode->dmPosition.y = 0;
    devmode->dmDisplayOrientation = 0;
    devmode->dmDisplayFixedOutput = 0;
    devmode->dmPelsWidth = screen_width;
    devmode->dmPelsHeight = screen_height;
    devmode->dmBitsPerPel = screen_bpp;
    devmode->dmDisplayFrequency = 60;
    devmode->dmFields = DM_POSITION | DM_DISPLAYORIENTATION | DM_PELSWIDTH | DM_PELSHEIGHT |
                        DM_BITSPERPEL | DM_DISPLAYFLAGS | DM_DISPLAYFREQUENCY;
    TRACE( "current mode -- %dx%d %d bpp @%d Hz\n",
           (int)devmode->dmPelsWidth, (int)devmode->dmPelsHeight,
           (int)devmode->dmBitsPerPel, (int)devmode->dmDisplayFrequency );
    return TRUE;
}

/***********************************************************************
 *           BROADWAYDRV_ChangeDisplaySettings
 */
LONG BROADWAYDRV_ChangeDisplaySettings( LPDEVMODEW displays, LPCWSTR primary_name, HWND hwnd, DWORD flags, LPVOID lpvoid )
{
    FIXME( "(%p,%s,%p,0x%08x,%p)\n", displays, debugstr_w(primary_name), hwnd, (int)flags, lpvoid );
    return DISP_CHANGE_SUCCESSFUL;
}

