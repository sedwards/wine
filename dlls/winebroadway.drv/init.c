/*
 * X11 graphics driver initialisation functions
 *
 * Copyright 1996 Alexandre Julliard
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
#include "winreg.h"
#include "broadwaydrv.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(broadwaydrv);

Display *gdi_display;  /* display to use for all GDI functions */

static int palette_size;

static Pixmap stock_bitmap_pixmap;  /* phys bitmap for the default stock bitmap */

static pthread_once_t init_once = PTHREAD_ONCE_INIT;

static const struct user_driver_funcs broadwaydrv_funcs;
static const struct gdi_dc_funcs *xrender_funcs;


void init_recursive_mutex( pthread_mutex_t *mutex )
{
    pthread_mutexattr_t attr;

    pthread_mutexattr_init( &attr );
    pthread_mutexattr_settype( &attr, PTHREAD_MUTEX_RECURSIVE );
    pthread_mutex_init( mutex, &attr );
    pthread_mutexattr_destroy( &attr );
}


/**********************************************************************
 *	     device_init
 *
 * Perform initializations needed upon creation of the first device.
 */
static void device_init(void)
{
    /* Initialize XRender */
    xrender_funcs = BROADWAYDRV_XRender_Init();

    /* Init Xcursor */
    BROADWAYDRV_Xcursor_Init();

    palette_size = BROADWAYDRV_PALETTE_Init();

    stock_bitmap_pixmap = XCreatePixmap( gdi_display, root_window, 1, 1, 1 );
}


static BROADWAYDRV_PDEVICE *create_x11_physdev( Drawable drawable )
{
    BROADWAYDRV_PDEVICE *physDev;

    pthread_once( &init_once, device_init );

    if (!(physDev = calloc( 1, sizeof(*physDev) ))) return NULL;

    physDev->drawable = drawable;
    physDev->gc = XCreateGC( gdi_display, drawable, 0, NULL );
    XSetGraphicsExposures( gdi_display, physDev->gc, False );
    XSetSubwindowMode( gdi_display, physDev->gc, IncludeInferiors );
    XFlush( gdi_display );
    return physDev;
}

/**********************************************************************
 *	     BROADWAYDRV_CreateDC
 */
static BOOL BROADWAYDRV_CreateDC( PHYSDEV *pdev, LPCWSTR device, LPCWSTR output, const DEVMODEW* initData )
{
    BROADWAYDRV_PDEVICE *physDev = create_x11_physdev( root_window );

    if (!physDev) return FALSE;

    physDev->depth         = default_visual.depth;
    physDev->color_shifts  = &BROADWAYDRV_PALETTE_default_shifts;
    physDev->dc_rect       = NtUserGetVirtualScreenRect();
    OffsetRect( &physDev->dc_rect, -physDev->dc_rect.left, -physDev->dc_rect.top );
    push_dc_driver( pdev, &physDev->dev, &broadwaydrv_funcs.dc_funcs );
    if (xrender_funcs && !xrender_funcs->pCreateDC( pdev, device, output, initData )) return FALSE;
    return TRUE;
}


/**********************************************************************
 *	     BROADWAYDRV_CreateCompatibleDC
 */
static BOOL BROADWAYDRV_CreateCompatibleDC( PHYSDEV orig, PHYSDEV *pdev )
{
    BROADWAYDRV_PDEVICE *physDev = create_x11_physdev( stock_bitmap_pixmap );

    if (!physDev) return FALSE;

    physDev->depth  = 1;
    SetRect( &physDev->dc_rect, 0, 0, 1, 1 );
    push_dc_driver( pdev, &physDev->dev, &broadwaydrv_funcs.dc_funcs );
    if (orig) return TRUE;  /* we already went through Xrender if we have an orig device */
    if (xrender_funcs && !xrender_funcs->pCreateCompatibleDC( NULL, pdev )) return FALSE;
    return TRUE;
}


/**********************************************************************
 *	     BROADWAYDRV_DeleteDC
 */
static BOOL BROADWAYDRV_DeleteDC( PHYSDEV dev )
{
    BROADWAYDRV_PDEVICE *physDev = get_broadwaydrv_dev( dev );

    XFreeGC( gdi_display, physDev->gc );
    free( physDev );
    return TRUE;
}


void add_device_bounds( BROADWAYDRV_PDEVICE *dev, const RECT *rect )
{
    RECT rc;

    if (!dev->bounds) return;
    if (dev->region && NtGdiGetRgnBox( dev->region, &rc ))
    {
        if (intersect_rect( &rc, &rc, rect )) add_bounds_rect( dev->bounds, &rc );
    }
    else add_bounds_rect( dev->bounds, rect );
}

/***********************************************************************
 *           BROADWAYDRV_SetBoundsRect
 */
static UINT BROADWAYDRV_SetBoundsRect( PHYSDEV dev, RECT *rect, UINT flags )
{
    BROADWAYDRV_PDEVICE *pdev = get_broadwaydrv_dev( dev );

    if (flags & DCB_DISABLE) pdev->bounds = NULL;
    else if (flags & DCB_ENABLE) pdev->bounds = rect;
    return DCB_RESET;  /* we don't have device-specific bounds */
}


/***********************************************************************
 *           GetDeviceCaps    (BROADWAYDRV.@)
 */
static INT BROADWAYDRV_GetDeviceCaps( PHYSDEV dev, INT cap )
{
    switch(cap)
    {
    case SIZEPALETTE:
        return palette_size;
    default:
        dev = GET_NEXT_PHYSDEV( dev, pGetDeviceCaps );
        return dev->funcs->pGetDeviceCaps( dev, cap );
    }
}


/***********************************************************************
 *           SelectFont
 */
static HFONT BROADWAYDRV_SelectFont( PHYSDEV dev, HFONT hfont, UINT *aa_flags )
{
    if (default_visual.depth <= 8) *aa_flags = GGO_BITMAP;  /* no anti-aliasing on <= 8bpp */
    dev = GET_NEXT_PHYSDEV( dev, pSelectFont );
    return dev->funcs->pSelectFont( dev, hfont, aa_flags );
}

/**********************************************************************
 *           ExtEscape  (BROADWAYDRV.@)
 */
static INT BROADWAYDRV_ExtEscape( PHYSDEV dev, INT escape, INT in_count, LPCVOID in_data,
                             INT out_count, LPVOID out_data )
{
    BROADWAYDRV_PDEVICE *physDev = get_broadwaydrv_dev( dev );

    switch(escape)
    {
    case QUERYESCSUPPORT:
        if (in_data && in_count >= sizeof(DWORD))
        {
            switch (*(const INT *)in_data)
            {
            case BROADWAYDRV_ESCAPE:
                return TRUE;
            }
        }
        break;

    case BROADWAYDRV_ESCAPE:
        if (in_data && in_count >= sizeof(enum broadwaydrv_escape_codes))
        {
            switch(*(const enum broadwaydrv_escape_codes *)in_data)
            {
            case BROADWAYDRV_SET_DRAWABLE:
                if (in_count >= sizeof(struct broadwaydrv_escape_set_drawable))
                {
                    const struct broadwaydrv_escape_set_drawable *data = in_data;
                    physDev->dc_rect = data->dc_rect;
                    physDev->drawable = data->drawable;
                    XFreeGC( gdi_display, physDev->gc );
                    physDev->gc = XCreateGC( gdi_display, physDev->drawable, 0, NULL );
                    XSetGraphicsExposures( gdi_display, physDev->gc, False );
                    XSetSubwindowMode( gdi_display, physDev->gc, data->mode );
                    TRACE( "SET_DRAWABLE hdc %p drawable %lx dc_rect %s\n",
                           dev->hdc, physDev->drawable, wine_dbgstr_rect(&physDev->dc_rect) );
                    return TRUE;
                }
                break;
            case BROADWAYDRV_GET_DRAWABLE:
                if (out_count >= sizeof(struct broadwaydrv_escape_get_drawable))
                {
                    struct broadwaydrv_escape_get_drawable *data = out_data;
                    data->drawable = physDev->drawable;
                    return TRUE;
                }
                break;
            case BROADWAYDRV_FLUSH_GL_DRAWABLE:
                if (in_count >= sizeof(struct broadwaydrv_escape_flush_gl_drawable))
                {
                    const struct broadwaydrv_escape_flush_gl_drawable *data = in_data;
                    RECT rect = physDev->dc_rect;

                    OffsetRect( &rect, -physDev->dc_rect.left, -physDev->dc_rect.top );
                    if (data->flush) XFlush( gdi_display );
                    XSetFunction( gdi_display, physDev->gc, GXcopy );
                    XCopyArea( gdi_display, data->gl_drawable, physDev->drawable, physDev->gc,
                               0, 0, rect.right, rect.bottom,
                               physDev->dc_rect.left, physDev->dc_rect.top );
                    add_device_bounds( physDev, &rect );
                    return TRUE;
                }
                break;
            case BROADWAYDRV_START_EXPOSURES:
                XSetGraphicsExposures( gdi_display, physDev->gc, True );
                physDev->exposures = 0;
                return TRUE;
            case BROADWAYDRV_END_EXPOSURES:
                if (out_count >= sizeof(HRGN))
                {
                    HRGN hrgn = 0, tmp = 0;

                    XSetGraphicsExposures( gdi_display, physDev->gc, False );
                    if (physDev->exposures)
                    {
                        for (;;)
                        {
                            XEvent event;

                            XWindowEvent( gdi_display, physDev->drawable, ~0, &event );
                            if (event.type == NoExpose) break;
                            if (event.type == GraphicsExpose)
                            {
                                DWORD layout;
                                RECT rect;

                                rect.left   = event.xgraphicsexpose.x - physDev->dc_rect.left;
                                rect.top    = event.xgraphicsexpose.y - physDev->dc_rect.top;
                                rect.right  = rect.left + event.xgraphicsexpose.width;
                                rect.bottom = rect.top + event.xgraphicsexpose.height;
                                if (NtGdiGetDCDword( dev->hdc, NtGdiGetLayout, &layout ) &&
                                    (layout & LAYOUT_RTL))
                                    mirror_rect( &physDev->dc_rect, &rect );

                                TRACE( "got %s count %d\n", wine_dbgstr_rect(&rect),
                                       event.xgraphicsexpose.count );

                                if (!tmp) tmp = NtGdiCreateRectRgn( rect.left, rect.top,
                                                                    rect.right, rect.bottom );
                                else NtGdiSetRectRgn( tmp, rect.left, rect.top, rect.right, rect.bottom );
                                if (hrgn) NtGdiCombineRgn( hrgn, hrgn, tmp, RGN_OR );
                                else
                                {
                                    hrgn = tmp;
                                    tmp = 0;
                                }
                                if (!event.xgraphicsexpose.count) break;
                            }
                            else
                            {
                                ERR( "got unexpected event %d\n", event.type );
                                break;
                            }
                        }
                        if (tmp) NtGdiDeleteObjectApp( tmp );
                    }
                    *(HRGN *)out_data = hrgn;
                    return TRUE;
                }
                break;
            default:
                break;
            }
        }
        break;
    }
    return 0;
}

/**********************************************************************
 *           BROADWAYDRV_wine_get_wgl_driver
 */
static struct opengl_funcs *BROADWAYDRV_wine_get_wgl_driver( UINT version )
{
    return get_glx_driver( version );
}

/**********************************************************************
 *           BROADWAYDRV_wine_get_vulkan_driver
 */
static const struct vulkan_funcs *BROADWAYDRV_wine_get_vulkan_driver( UINT version )
{
    return get_vulkan_driver( version );
}


static const struct user_driver_funcs broadwaydrv_funcs =
{
    .dc_funcs.pArc = BROADWAYDRV_Arc,
    .dc_funcs.pChord = BROADWAYDRV_Chord,
    .dc_funcs.pCreateCompatibleDC = BROADWAYDRV_CreateCompatibleDC,
    .dc_funcs.pCreateDC = BROADWAYDRV_CreateDC,
    .dc_funcs.pDeleteDC = BROADWAYDRV_DeleteDC,
    .dc_funcs.pEllipse = BROADWAYDRV_Ellipse,
    .dc_funcs.pExtEscape = BROADWAYDRV_ExtEscape,
    .dc_funcs.pExtFloodFill = BROADWAYDRV_ExtFloodFill,
    .dc_funcs.pFillPath = BROADWAYDRV_FillPath,
    .dc_funcs.pGetDeviceCaps = BROADWAYDRV_GetDeviceCaps,
    .dc_funcs.pGetDeviceGammaRamp = BROADWAYDRV_GetDeviceGammaRamp,
    .dc_funcs.pGetICMProfile = BROADWAYDRV_GetICMProfile,
    .dc_funcs.pGetImage = BROADWAYDRV_GetImage,
    .dc_funcs.pGetNearestColor = BROADWAYDRV_GetNearestColor,
    .dc_funcs.pGetSystemPaletteEntries = BROADWAYDRV_GetSystemPaletteEntries,
    .dc_funcs.pGradientFill = BROADWAYDRV_GradientFill,
    .dc_funcs.pLineTo = BROADWAYDRV_LineTo,
    .dc_funcs.pPaintRgn = BROADWAYDRV_PaintRgn,
    .dc_funcs.pPatBlt = BROADWAYDRV_PatBlt,
    .dc_funcs.pPie = BROADWAYDRV_Pie,
    .dc_funcs.pPolyPolygon = BROADWAYDRV_PolyPolygon,
    .dc_funcs.pPolyPolyline = BROADWAYDRV_PolyPolyline,
    .dc_funcs.pPutImage = BROADWAYDRV_PutImage,
    .dc_funcs.pRealizeDefaultPalette = BROADWAYDRV_RealizeDefaultPalette,
    .dc_funcs.pRealizePalette = BROADWAYDRV_RealizePalette,
    .dc_funcs.pRectangle = BROADWAYDRV_Rectangle,
    .dc_funcs.pRoundRect = BROADWAYDRV_RoundRect,
    .dc_funcs.pSelectBrush = BROADWAYDRV_SelectBrush,
    .dc_funcs.pSelectFont = BROADWAYDRV_SelectFont,
    .dc_funcs.pSelectPen = BROADWAYDRV_SelectPen,
    .dc_funcs.pSetBoundsRect = BROADWAYDRV_SetBoundsRect,
    .dc_funcs.pSetDCBrushColor = BROADWAYDRV_SetDCBrushColor,
    .dc_funcs.pSetDCPenColor = BROADWAYDRV_SetDCPenColor,
    .dc_funcs.pSetDeviceClipping = BROADWAYDRV_SetDeviceClipping,
    .dc_funcs.pSetDeviceGammaRamp = BROADWAYDRV_SetDeviceGammaRamp,
    .dc_funcs.pSetPixel = BROADWAYDRV_SetPixel,
    .dc_funcs.pStretchBlt = BROADWAYDRV_StretchBlt,
    .dc_funcs.pStrokeAndFillPath = BROADWAYDRV_StrokeAndFillPath,
    .dc_funcs.pStrokePath = BROADWAYDRV_StrokePath,
    .dc_funcs.pUnrealizePalette = BROADWAYDRV_UnrealizePalette,
    .dc_funcs.pD3DKMTCheckVidPnExclusiveOwnership = BROADWAYDRV_D3DKMTCheckVidPnExclusiveOwnership,
    .dc_funcs.pD3DKMTCloseAdapter = BROADWAYDRV_D3DKMTCloseAdapter,
    .dc_funcs.pD3DKMTOpenAdapterFromLuid = BROADWAYDRV_D3DKMTOpenAdapterFromLuid,
    .dc_funcs.pD3DKMTQueryVideoMemoryInfo = BROADWAYDRV_D3DKMTQueryVideoMemoryInfo,
    .dc_funcs.pD3DKMTSetVidPnSourceOwner = BROADWAYDRV_D3DKMTSetVidPnSourceOwner,
    .dc_funcs.priority = GDI_PRIORITY_GRAPHICS_DRV,

    .pActivateKeyboardLayout = BROADWAYDRV_ActivateKeyboardLayout,
    .pBeep = BROADWAYDRV_Beep,
    .pGetKeyNameText = BROADWAYDRV_GetKeyNameText,
    .pMapVirtualKeyEx = BROADWAYDRV_MapVirtualKeyEx,
    .pToUnicodeEx = BROADWAYDRV_ToUnicodeEx,
    .pVkKeyScanEx = BROADWAYDRV_VkKeyScanEx,
    .pImeToAsciiEx = BROADWAYDRV_ImeToAsciiEx,
    .pNotifyIMEStatus = BROADWAYDRV_NotifyIMEStatus,
    .pDestroyCursorIcon = BROADWAYDRV_DestroyCursorIcon,
    .pSetCursor = BROADWAYDRV_SetCursor,
    .pGetCursorPos = BROADWAYDRV_GetCursorPos,
    .pSetCursorPos = BROADWAYDRV_SetCursorPos,
    .pClipCursor = BROADWAYDRV_ClipCursor,
    .pChangeDisplaySettings = BROADWAYDRV_ChangeDisplaySettings,
    .pGetCurrentDisplaySettings = BROADWAYDRV_GetCurrentDisplaySettings,
    .pGetDisplayDepth = BROADWAYDRV_GetDisplayDepth,
    .pUpdateDisplayDevices = BROADWAYDRV_UpdateDisplayDevices,
    .pCreateDesktop = BROADWAYDRV_CreateDesktop,
    .pCreateWindow = BROADWAYDRV_CreateWindow,
    .pDesktopWindowProc = BROADWAYDRV_DesktopWindowProc,
    .pDestroyWindow = BROADWAYDRV_DestroyWindow,
    .pFlashWindowEx = BROADWAYDRV_FlashWindowEx,
    .pGetDC = BROADWAYDRV_GetDC,
    .pProcessEvents = BROADWAYDRV_ProcessEvents,
    .pReleaseDC = BROADWAYDRV_ReleaseDC,
    .pScrollDC = BROADWAYDRV_ScrollDC,
    .pSetCapture = BROADWAYDRV_SetCapture,
    .pSetDesktopWindow = BROADWAYDRV_SetDesktopWindow,
    .pSetFocus = BROADWAYDRV_SetFocus,
    .pSetLayeredWindowAttributes = BROADWAYDRV_SetLayeredWindowAttributes,
    .pSetParent = BROADWAYDRV_SetParent,
    .pSetWindowIcon = BROADWAYDRV_SetWindowIcon,
    .pSetWindowRgn = BROADWAYDRV_SetWindowRgn,
    .pSetWindowStyle = BROADWAYDRV_SetWindowStyle,
    .pSetWindowText = BROADWAYDRV_SetWindowText,
    .pShowWindow = BROADWAYDRV_ShowWindow,
    .pSysCommand = BROADWAYDRV_SysCommand,
    .pClipboardWindowProc = BROADWAYDRV_ClipboardWindowProc,
    .pUpdateClipboard = BROADWAYDRV_UpdateClipboard,
    .pUpdateLayeredWindow = BROADWAYDRV_UpdateLayeredWindow,
    .pWindowMessage = BROADWAYDRV_WindowMessage,
    .pWindowPosChanging = BROADWAYDRV_WindowPosChanging,
    .pWindowPosChanged = BROADWAYDRV_WindowPosChanged,
    .pSystemParametersInfo = BROADWAYDRV_SystemParametersInfo,
    .pwine_get_vulkan_driver = BROADWAYDRV_wine_get_vulkan_driver,
    .pwine_get_wgl_driver = BROADWAYDRV_wine_get_wgl_driver,
    .pThreadDetach = BROADWAYDRV_ThreadDetach,
};


void init_user_driver(void)
{
    __wine_set_user_driver( &broadwaydrv_funcs, WINE_GDI_DRIVER_VERSION );
}
