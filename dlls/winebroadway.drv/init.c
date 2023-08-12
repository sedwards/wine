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
    .dc_funcs.pCreateCompatibleDC = broadway_CreateCompatibleDC,
    .dc_funcs.pCreateDC = broadway_CreateDC,
    .dc_funcs.pDeleteDC = broadway_DeleteDC,
    .dc_funcs.pGetDeviceCaps = broadway_GetDeviceCaps,
    .dc_funcs.pGetDeviceGammaRamp = broadway_GetDeviceGammaRamp,
    .dc_funcs.pSetDeviceGammaRamp = broadway_SetDeviceGammaRamp,
    .dc_funcs.priority = GDI_PRIORITY_GRAPHICS_DRV,

    .pActivateKeyboardLayout = broadway_ActivateKeyboardLayout,
    .pBeep = broadway_Beep,
    .pChangeDisplaySettings = broadway_ChangeDisplaySettings,
    .pClipCursor = broadway_ClipCursor,
    .pClipboardWindowProc = broadway_ClipboardWindowProc,
    .pDesktopWindowProc = broadway_DesktopWindowProc,
    .pDestroyCursorIcon = broadway_DestroyCursorIcon,
    .pDestroyWindow = broadway_DestroyWindow,
    .pGetCurrentDisplaySettings = broadway_GetCurrentDisplaySettings,
    .pGetDisplayDepth = broadway_GetDisplayDepth,
    .pUpdateDisplayDevices = broadway_UpdateDisplayDevices,
    .pGetCursorPos = broadway_GetCursorPos,
    .pGetKeyboardLayoutList = broadway_GetKeyboardLayoutList,
    .pGetKeyNameText = broadway_GetKeyNameText,
    .pMapVirtualKeyEx = broadway_MapVirtualKeyEx,
    .pProcessEvents = broadway_ProcessEvents,
    .pRegisterHotKey = broadway_RegisterHotKey,
    .pSetCapture = broadway_SetCapture,
    .pSetCursor = broadway_SetCursor,
    .pSetCursorPos = broadway_SetCursorPos,
    .pSetDesktopWindow = broadway_SetDesktopWindow,
    .pSetFocus = broadway_SetFocus,
    .pSetLayeredWindowAttributes = broadway_SetLayeredWindowAttributes,
    .pSetParent = broadway_SetParent,
    .pSetWindowRgn = broadway_SetWindowRgn,
    .pSetWindowStyle = broadway_SetWindowStyle,
    .pSetWindowText = broadway_SetWindowText,
    .pShowWindow = broadway_ShowWindow,
    .pSysCommand =broadway_SysCommand,
    .pSystemParametersInfo = broadway_SystemParametersInfo,
    .pThreadDetach = broadway_ThreadDetach,
    .pToUnicodeEx = broadway_ToUnicodeEx,
    .pUnregisterHotKey = broadway_UnregisterHotKey,
    .pUpdateClipboard = broadway_UpdateClipboard,
    .pUpdateLayeredWindow = broadway_UpdateLayeredWindow,
    .pVkKeyScanEx = broadway_VkKeyScanEx,
    .pImeProcessKey = broadway_ImeProcessKey,
    .pImeToAsciiEx = broadway_ImeToAsciiEx,
    .pNotifyIMEStatus = broadway_NotifyIMEStatus,
    .pWindowMessage = broadway_WindowMessage,
    .pWindowPosChanged = broadway_WindowPosChanged,
    .pWindowPosChanging = broadway_WindowPosChanging,
    .pwine_get_vulkan_driver = broadway_wine_get_vulkan_driver,
    .pwine_get_wgl_driver = broadway_wine_get_wgl_driver,
#endif
};


void init_user_driver(void)
{
    __wine_set_user_driver( &broadway_funcs, WINE_GDI_DRIVER_VERSION );
}

