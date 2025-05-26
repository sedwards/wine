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

/* driver.c */
#include <stdarg.h>
#include <stdlib.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "wine/gdi_driver.h"

#include "winerds.h"
#include "wine/debug.h"


WINE_DEFAULT_DEBUG_CHANNEL(rds);


typedef struct
{
    struct gdi_physdev  dev;
} RDSDRV_PDEVICE;

static inline RDSDRV_PDEVICE *get_rdsdrv_dev(PHYSDEV dev)
{
    return (RDSDRV_PDEVICE*)dev;
}


/* a few dynamic device caps */
//static RECT desktop_rect;     /* virtual desktop rectangle */
static int horz_size;           /* horz. size of screen in millimeters */
static int vert_size;           /* vert. size of screen in millimeters */
static int bits_per_pixel;      /* pixel depth of screen */
static int device_data_valid;   /* do the above variables have up-to-date values? */

int retina_on = FALSE;

static pthread_mutex_t device_data_mutex = PTHREAD_MUTEX_INITIALIZER;

static const struct user_driver_funcs rdsdrv_funcs;

#if 0
/***********************************************************************
 *              compute_desktop_rect
 */
static void compute_desktop_rect(void)
{
    CGDirectDisplayID displayIDs[32];
    uint32_t count, i;

    desktop_rect = CGRECTNull;
    if (CGGetOnlineDisplayList(ARRAY_SIZE(displayIDs), displayIDs, &count) != kCGErrorSuccess ||
        !count)
    {
        displayIDs[0] = CGMainDisplayID();
        count = 1;
    }

    for (i = 0; i < count; i++)
        desktop_rect = CGRECTUnion(desktop_rect, CGDisplayBounds(displayIDs[i]));
    desktop_rect = cgrect_win_from_rds(desktop_rect);
}


/***********************************************************************
 *              rdsdrv_get_desktop_rect
 *
 * Returns the rectangle encompassing all the screens.
 */
CGRECT rdsdrv_get_desktop_rect(void)
{
    CGRECT ret;

    pthread_mutex_lock(&device_data_mutex);

    if (!device_data_valid)
    {
        check_retina_status();
        compute_desktop_rect();
    }
    ret = desktop_rect;

    pthread_mutex_unlock(&device_data_mutex);

    TRACE("%s\n", wine_dbgstr_cgrect(ret));

    return ret;
}

#endif

static RECT desktop_rect = { {0, 0}, {1024, 768} }; // Fixed size virtual desktop

static int horz_size;           /* horz. size of screen in millimeters */
static int vert_size;           /* vert. size of screen in millimeters */
static int bits_per_pixel;      /* pixel depth of screen */
static int device_data_valid;   /* do the above variables have up-to-date values? */


/***********************************************************************
 *              compute_desktop_rect
 */
static void compute_desktop_rect(void)
{
    // For RDS, we use a fixed-size virtual desktop
    // No computation needed as we're not querying physical displays
    int x = 0;
    int y = 0;
    int width = 1024;  // Fixed width
    int height = 768;  // Fixed height
}

/***********************************************************************
 *              rdsdrv_get_desktop_rect
 *
 * Returns our virtual desktop rectangle.
 */
RECT rdsdrv_get_desktop_rect(void)
{
    RECT ret = desktop_rect;
    TRACE("Returning virtual desktop rect\n",
    return ret;
}

/**********************************************************************
 *              device_init
 *
 * Perform initializations needed upon creation of the first device.
 */
static void device_init(void)
{
    check_retina_status();

    /* Initialize device caps */
    horz_size = size_mm.width;
    vert_size = size_mm.height;
    bits_per_pixel = 32;
    compute_desktop_rect();

    device_data_valid = TRUE;
}


void rdsdrv_reset_device_metrics(void)
{
    pthread_mutex_lock(&device_data_mutex);
    device_data_valid = FALSE;
    pthread_mutex_unlock(&device_data_mutex);
}


static RDSDRV_PDEVICE *create_rds_physdev(void)
{
    RDSDRV_PDEVICE *physDev;

    pthread_mutex_lock(&device_data_mutex);
    if (!device_data_valid) device_init();
    pthread_mutex_unlock(&device_data_mutex);

    if (!(physDev = calloc(1, sizeof(*physDev)))) return NULL;

    return physDev;
}


/**********************************************************************
 *              CreateDC (RDSDRV.@)
 */
static BOOL rdsdrv_CreateDC(PHYSDEV *pdev, LPCWSTR device, LPCWSTR output, const DEVMODEW* initData)
{
    RDSDRV_PDEVICE *physDev = create_rds_physdev();

    TRACE("pdev %p hdc %p device %s output %s initData %p\n", pdev,
          (*pdev)->hdc, debugstr_w(device), debugstr_w(output), initData);

    if (!physDev) return FALSE;

    push_dc_driver(pdev, &physDev->dev, &rdsdrv_funcs.dc_funcs);
    return TRUE;
}


/**********************************************************************
 *              CreateCompatibleDC (RDSDRV.@)
 */
static BOOL rdsdrv_CreateCompatibleDC(PHYSDEV orig, PHYSDEV *pdev)
{
    RDSDRV_PDEVICE *physDev = create_rds_physdev();

    TRACE("orig %p orig->hdc %p pdev %p pdev->hdc %p\n", orig, (orig ? orig->hdc : NULL), pdev,
          ((pdev && *pdev) ? (*pdev)->hdc : NULL));

    if (!physDev) return FALSE;

    push_dc_driver(pdev, &physDev->dev, &rdsdrv_funcs.dc_funcs);
    return TRUE;
}


/**********************************************************************
 *              DeleteDC (RDSDRV.@)
 */
static BOOL rdsdrv_DeleteDC(PHYSDEV dev)
{
    RDSDRV_PDEVICE *physDev = get_rdsdrv_dev(dev);

    TRACE("hdc %p\n", dev->hdc);

    free(physDev);
    return TRUE;
}


/***********************************************************************
 *              GetDeviceCaps (RDSDRV.@)
 */
static INT rdsdrv_GetDeviceCaps(PHYSDEV dev, INT cap)
{
    INT ret;

    pthread_mutex_lock(&device_data_mutex);

    if (!device_data_valid) device_init();

    switch(cap)
    {
    case HORZSIZE:
        ret = horz_size;
        break;
    case VERTSIZE:
        ret = vert_size;
        break;
    case BITSPIXEL:
        ret = bits_per_pixel;
        break;
    case HORZRES:
    case VERTRES:
    default:
        pthread_mutex_unlock(&device_data_mutex);
        dev = GET_NEXT_PHYSDEV( dev, pGetDeviceCaps );
        ret = dev->funcs->pGetDeviceCaps( dev, cap );
        if ((cap == HORZRES || cap == VERTRES) && retina_on)
            ret *= 2;
        return ret;
    }

    TRACE("cap %d -> %d\n", cap, ret);

    pthread_mutex_unlock(&device_data_mutex);
    return ret;
}

#if 0
static const struct user_driver_funcs rdsdrv_funcs =
{
    .dc_funcs.pCreateCompatibleDC = rdsdrv_CreateCompatibleDC,
    .dc_funcs.pCreateDC = rdsdrv_CreateDC,
    .dc_funcs.pDeleteDC = rdsdrv_DeleteDC,
    .dc_funcs.pGetDeviceCaps = rdsdrv_GetDeviceCaps,
    .dc_funcs.pGetDeviceGammaRamp = rdsdrv_GetDeviceGammaRamp,
    .dc_funcs.pSetDeviceGammaRamp = rdsdrv_SetDeviceGammaRamp,
    .dc_funcs.priority = GDI_PRIORITY_GRAPHICS_DRV,

    .pActivateKeyboardLayout = rdsdrv_ActivateKeyboardLayout,
    .pBeep = rdsdrv_Beep,
    .pChangeDisplaySettings = rdsdrv_ChangeDisplaySettings,
    .pClipCursor = rdsdrv_ClipCursor,
    .pNotifyIcon = rdsdrv_NotifyIcon,
    .pCleanupIcons = rdsdrv_CleanupIcons,
    .pClipboardWindowProc = rdsdrv_ClipboardWindowProc,
    .pDesktopWindowProc = rdsdrv_DesktopWindowProc,
    .pDestroyCursorIcon = rdsdrv_DestroyCursorIcon,
    .pDestroyWindow = rdsdrv_DestroyWindow,
    .pUpdateDisplayDevices = rdsdrv_UpdateDisplayDevices,
    .pGetCursorPos = rdsdrv_GetCursorPos,
    .pGetKeyboardLayoutList = rdsdrv_GetKeyboardLayoutList,
    .pGetKeyNameText = rdsdrv_GetKeyNameText,
    .pMapVirtualKeyEx = rdsdrv_MapVirtualKeyEx,
    .pProcessEvents = rdsdrv_ProcessEvents,
    .pRegisterHotKey = rdsdrv_RegisterHotKey,
    .pSetCapture = rdsdrv_SetCapture,
    .pSetCursor = rdsdrv_SetCursor,
    .pSetCursorPos = rdsdrv_SetCursorPos,
    .pSetDesktopWindow = rdsdrv_SetDesktopWindow,
    .pActivateWindow = rdsdrv_ActivateWindow,
    .pSetLayeredWindowAttributes = rdsdrv_SetLayeredWindowAttributes,
    .pSetParent = rdsdrv_SetParent,
    .pSetWindowRgn = rdsdrv_SetWindowRgn,
    .pSetWindowStyle = rdsdrv_SetWindowStyle,
    .pSetWindowText = rdsdrv_SetWindowText,
    .pShowWindow = rdsdrv_ShowWindow,
    .pSysCommand =rdsdrv_SysCommand,
    .pSystemParametersInfo = rdsdrv_SystemParametersInfo,
    .pThreadDetach = rdsdrv_ThreadDetach,
    .pToUnicodeEx = rdsdrv_ToUnicodeEx,
    .pUnregisterHotKey = rdsdrv_UnregisterHotKey,
    .pUpdateClipboard = rdsdrv_UpdateClipboard,
    .pUpdateLayeredWindow = rdsdrv_UpdateLayeredWindow,
    .pVkKeyScanEx = rdsdrv_VkKeyScanEx,
    .pImeProcessKey = rdsdrv_ImeProcessKey,
    .pNotifyIMEStatus = rdsdrv_NotifyIMEStatus,
    .pSetIMECompositionRECT = rdsdrv_SetIMECompositionRECT,
    .pWindowMessage = rdsdrv_WindowMessage,
    .pWindowPosChanged = rdsdrv_WindowPosChanged,
    .pWindowPosChanging = rdsdrv_WindowPosChanging,
    .pGetWindowStyleMasks = rdsdrv_GetWindowStyleMasks,
    .pCreateWindowSurface = rdsdrv_CreateWindowSurface,
    .pVulkanInit = rdsdrv_VulkanInit,
    .pOpenGLInit = rdsdrv_OpenGLInit,
};


void init_user_driver(void)
{
    __wine_set_user_driver( &rdsdrv_funcs, WINE_GDI_DRIVER_VERSION );
}
#endif


