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

#include "windef.h"
#include "winbase.h"
#include "ntgdi.h"
#include "wine/gdi_driver.h"
#include "unixlib.h"
#include "wine/list.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(broadway);

typedef struct
{
    struct gdi_physdev  dev;
} BROADWAY_PDEVICE;

static inline BROADWAY_PDEVICE *get_broadway_dev(PHYSDEV dev)
{
    return (BROADWAY_PDEVICE*)dev;
}

static const struct user_driver_funcs broadway_funcs =
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
    .dc_funcs.pGetDeviceCaps = X11DRV_GetDeviceCaps,
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
    .pChangeDisplaySettings = X11DRV_ChangeDisplaySettings,
    .pGetCurrentDisplaySettings = X11DRV_GetCurrentDisplaySettings,
    .pGetDisplayDepth = X11DRV_GetDisplayDepth,
    .pUpdateDisplayDevices = X11DRV_UpdateDisplayDevices,
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
    .pSystemParametersInfo = X11DRV_SystemParametersInfo,
    .pwine_get_vulkan_driver = X11DRV_wine_get_vulkan_driver,
    .pwine_get_wgl_driver = X11DRV_wine_get_wgl_driver,
    .pThreadDetach = X11DRV_ThreadDetach,
#endif
};

void init_user_driver(void)
{
    __wine_set_user_driver( &broadway_funcs, WINE_GDI_DRIVER_VERSION );
}

