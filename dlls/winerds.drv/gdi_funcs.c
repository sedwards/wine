/* Fixed version of gdi_funcs.c for Wine RDS driver */

#include <windows.h>
#include <winbase.h>
#include <winuser.h>
#include <wingdi.h>
#include <winerror.h>
#include <stdio.h>
#include <string.h>

#include "rdsgdi_driver.h"
#include "rdsdrv.h"
#include "pipe_client.h"
#include "rds_message.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(winerds);

// Driver version
#define DRIVER_VERSION 0x00030100

// Global driver state
static CRITICAL_SECTION rds_pdev_list_lock;
static CRITICAL_SECTION_DEBUG rds_pdev_list_lock_debug =
{
    0, 0, &rds_pdev_list_lock,
    { &rds_pdev_list_lock_debug.ProcessLocksList, &rds_pdev_list_lock_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": rds_pdev_list_lock") }
};
static CRITICAL_SECTION rds_pdev_list_lock = { &rds_pdev_list_lock_debug, -1, 0, 0, 0, 0 };

static RDSDRV_PDEVICE *rds_pdev_list_head = NULL;

// RDS driver device structure
typedef struct tagRDS_PHYSDEV
{
    struct gdi_physdev dev;
    RDSDRV_PDEVICE *pdev;
} RDS_PHYSDEV;

// Helper to get RDS physdev
static inline RDS_PHYSDEV *get_rds_dev(PHYSDEV dev)
{
    return (RDS_PHYSDEV *)dev;
}

// --- DC Functions ---
static BOOL WINAPI RDS_CreateDC(PHYSDEV *pdev, LPCWSTR device, LPCWSTR driver, const DEVMODEW *devmode)
{
    RDS_PHYSDEV *physdev;
    RDSDRV_PDEVICE *rds_pdev;
    static DWORD create_dc_count = 0;
    DWORD timestamp = GetTickCount();
    
    create_dc_count++;
    TRACE("[CreateDC #%lu] Called at %lu ms for device='%S', driver='%S'\n", 
           create_dc_count, timestamp,
           device ? device : L"(null)", 
           driver ? driver : L"(null)");

    // Allocate our physical device
    physdev = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(RDS_PHYSDEV));
    if (!physdev) {
        ERR("Failed to allocate RDS_PHYSDEV\n");
        return FALSE;
    }

    // Allocate RDS device info
    rds_pdev = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(RDSDRV_PDEVICE));
    if (!rds_pdev) {
        ERR("Failed to allocate RDSDRV_PDEVICE\n");
        HeapFree(GetProcessHeap(), 0, physdev);
        return FALSE;
    }

    // Initialize device properties based on the device type
    if (device && (wcscmp(device, L"DISPLAY") == 0 || wcsstr(device, L"\\\\.\\DISPLAY") == device)) {
        rds_pdev->is_screen = TRUE;
        rds_pdev->screen_width = (devmode && (devmode->dmFields & DM_PELSWIDTH)) ? devmode->dmPelsWidth : 800;
        rds_pdev->screen_height = (devmode && (devmode->dmFields & DM_PELSHEIGHT)) ? devmode->dmPelsHeight : 600;
        rds_pdev->screen_bpp = (devmode && (devmode->dmFields & DM_BITSPERPEL)) ? devmode->dmBitsPerPel : 32;
        TRACE("Created SCREEN pdev (%dx%dx%d)\n", 
               rds_pdev->screen_width, rds_pdev->screen_height, rds_pdev->screen_bpp);
    } else {
        rds_pdev->is_screen = FALSE;
        rds_pdev->screen_width = 800;
        rds_pdev->screen_height = 600;
        rds_pdev->screen_bpp = 32;
        TRACE("Created non-screen pdev\n");
    }

    physdev->pdev = rds_pdev;

    // Add to global list
    EnterCriticalSection(&rds_pdev_list_lock);
    rds_pdev->next = rds_pdev_list_head;
    rds_pdev_list_head = rds_pdev;
    LeaveCriticalSection(&rds_pdev_list_lock);

    // Push our device onto the stack
    push_dc_driver(pdev, &physdev->dev, &rds_dc_funcs);

    // Start RDS pipe if this is a display device
    if (rds_pdev->is_screen) {
        TRACE("[CreateDC #%lu] Starting RDS client pipe for screen device...\n", create_dc_count);
        if (StartRDSClientPipe()) {
            TRACE("[CreateDC #%lu] RDS client pipe started successfully\n", create_dc_count);
        } else {
            ERR("[CreateDC #%lu] Failed to start RDS client pipe!\n", create_dc_count);
        }
    }

    return TRUE;
}

static BOOL WINAPI RDS_DeleteDC(PHYSDEV dev)
{
    RDS_PHYSDEV *physdev = get_rds_dev(dev);
    RDSDRV_PDEVICE *pdev = physdev->pdev;
    RDSDRV_PDEVICE *prev_pdev = NULL;
    
    TRACE("winerds.drv: RDS_DeleteDC called\n");

    if (!pdev) {
        ERR("No PDEVICE found\n");
        return TRUE;
    }

    // Remove from list
    EnterCriticalSection(&rds_pdev_list_lock);
    RDSDRV_PDEVICE *cur = rds_pdev_list_head;
    while (cur) {
        if (cur == pdev) {
            if (prev_pdev) {
                prev_pdev->next = cur->next;
            } else {
                rds_pdev_list_head = cur->next;
            }
            break;
        }
        prev_pdev = cur;
        cur = cur->next;
    }
    LeaveCriticalSection(&rds_pdev_list_lock);

    // Free resources
    HeapFree(GetProcessHeap(), 0, pdev);
    HeapFree(GetProcessHeap(), 0, physdev);

    return TRUE;
}

// --- Drawing Functions ---
static BOOL WINAPI RDS_LineTo(PHYSDEV dev, INT x, INT y)
{
    RDS_PHYSDEV *physdev = get_rds_dev(dev);
    RDSDRV_PDEVICE *pdev = physdev->pdev;
    RDS_MESSAGE msg;
    
    TRACE("RDS_LineTo(%d, %d)\n", x, y);
    
    if (!pdev || !pdev->is_screen) {
        return TRUE; // Nothing to do for non-screen devices
    }

    // Send line drawing message to termsrv
    ZeroMemory(&msg, sizeof(RDS_MESSAGE));
    msg.msgType = RDS_MSG_LINE_TO;
    msg.params.lineTo.surfaceId = 1; // Primary surface
    msg.params.lineTo.x = x;
    msg.params.lineTo.y = y;
    msg.params.lineTo.color = RGB(255, 255, 255); // Default white
    msg.params.lineTo.lopnStyle = PS_SOLID;
    msg.params.lineTo.lopnWidth_x = 1;
    
    SendRDSMessage(&msg, NULL, 0);
    
    return TRUE;
}

static BOOL WINAPI RDS_Rectangle(PHYSDEV dev, INT left, INT top, INT right, INT bottom)
{
    RDS_PHYSDEV *physdev = get_rds_dev(dev);
    RDSDRV_PDEVICE *pdev = physdev->pdev;
    RDS_MESSAGE msg;
    
    TRACE("RDS_Rectangle(%d, %d, %d, %d)\n", left, top, right, bottom);
    
    if (!pdev || !pdev->is_screen) {
        return TRUE;
    }

    // Send rectangle message to termsrv
    ZeroMemory(&msg, sizeof(RDS_MESSAGE));
    msg.msgType = RDS_MSG_RECTANGLE;
    msg.params.rectangle.surfaceId = 1;
    msg.params.rectangle.left = left;
    msg.params.rectangle.top = top;
    msg.params.rectangle.right = right;
    msg.params.rectangle.bottom = bottom;
    msg.params.rectangle.color = RGB(255, 255, 255);
    msg.params.rectangle.filled = FALSE;
    msg.params.rectangle.pen_lopnStyle = PS_SOLID;
    msg.params.rectangle.pen_lopnWidth_x = 1;
    
    SendRDSMessage(&msg, NULL, 0);
    
    return TRUE;
}

static BOOL WINAPI RDS_ExtTextOut(PHYSDEV dev, INT x, INT y, UINT flags, const RECT *rect,
                                   LPCWSTR str, UINT count, const INT *dx)
{
    RDS_PHYSDEV *physdev = get_rds_dev(dev);
    RDSDRV_PDEVICE *pdev = physdev->pdev;
    RDS_MESSAGE msg;
    DWORD data_size;
    
    TRACE("RDS_ExtTextOut(%d, %d, '%S', count=%u)\n", x, y, str, count);
    
    if (!pdev || !pdev->is_screen || !str || count == 0) {
        return TRUE;
    }

    // Calculate size for text data
    data_size = count * sizeof(WCHAR);

    // Send text output message to termsrv
    ZeroMemory(&msg, sizeof(RDS_MESSAGE));
    msg.msgType = RDS_MSG_TEXT_OUT;
    msg.params.textOut.surfaceId = 1;
    msg.params.textOut.x = x;
    msg.params.textOut.y = y;
    msg.params.textOut.color = RGB(255, 255, 255);
    msg.params.textOut.count = count;
    msg.params.textOut.data_size = data_size;
    msg.params.textOut.text_bk_color = RGB(0, 0, 0);
    msg.params.textOut.bk_mode = TRANSPARENT;
    
    // Default font properties
    msg.params.textOut.font_lfHeight = -13;
    msg.params.textOut.font_lfWeight = FW_NORMAL;
    wcscpy(msg.params.textOut.font_lfFaceName, L"Arial");
    
    SendRDSMessage(&msg, str, data_size);
    
    return TRUE;
}

// Driver capabilities
static INT WINAPI RDS_GetDeviceCaps(PHYSDEV dev, INT cap)
{
    RDS_PHYSDEV *physdev = get_rds_dev(dev);
    RDSDRV_PDEVICE *pdev = physdev->pdev;
    
    TRACE("RDS_GetDeviceCaps(cap=%d)\n", cap);
    
    if (!pdev) {
        return 0;
    }

    switch (cap) {
    case HORZRES:
        return pdev->screen_width;
    case VERTRES:
        return pdev->screen_height;
    case BITSPIXEL:
        return pdev->screen_bpp;
    case RASTERCAPS:
        return RC_BITBLT | RC_BANDING | RC_SCALING | RC_BITMAP64 | RC_DI_BITMAP | RC_DIBTODEV;
    case TECHNOLOGY:
        return DT_RASDISPLAY;
    default:
        return 0;
    }
}

// Get modes function
ULONG WINAPI DrvGetModes(HANDLE hDriver, ULONG cjSize, DEVMODEW *pdm)
{
    const WCHAR deviceName[] = L"WineRDS Virtual Display";
    
    TRACE("winerds.drv: DrvGetModes called. hDriver=%p, cjSize=%lu, pdm=%p\n", hDriver, cjSize, pdm);
    
    if (pdm == NULL) {
        return sizeof(DEVMODEW);
    }
    
    if (cjSize < sizeof(DEVMODEW)) {
        return 0;
    }
    
    memset(pdm, 0, sizeof(DEVMODEW));
    lstrcpynW((LPWSTR)pdm->dmDeviceName, deviceName, CCHDEVICENAME);
    pdm->dmSize = sizeof(DEVMODEW);
    pdm->dmSpecVersion = DM_SPECVERSION;
    pdm->dmDriverVersion = (WORD)(DRIVER_VERSION & 0xFFFF);
    pdm->dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL | DM_DISPLAYFREQUENCY | DM_DISPLAYFLAGS;
    pdm->dmPelsWidth = 800;
    pdm->dmPelsHeight = 600;
    pdm->dmBitsPerPel = 32;
    pdm->dmDisplayFrequency = 60;
    pdm->dmDisplayFlags = 0;
    
    TRACE("winerds.drv: DrvGetModes filled DEVMODEW: %lux%lux%lu @ %luHz\n",
           pdm->dmPelsWidth, pdm->dmPelsHeight, pdm->dmBitsPerPel, pdm->dmDisplayFrequency);
           
    return sizeof(DEVMODEW);
}

// Forward declaration
static const struct gdi_dc_funcs rds_dc_funcs;

// Driver function table
static const struct gdi_dc_funcs rds_dc_funcs =
{
    NULL,                               /* pAbortDoc */
    NULL,                               /* pAbortPath */
    NULL,                               /* pAlphaBlend */
    NULL,                               /* pAngleArc */
    NULL,                               /* pArc */
    NULL,                               /* pArcTo */
    NULL,                               /* pBeginPath */
    NULL,                               /* pBlendImage */
    NULL,                               /* pChord */
    NULL,                               /* pCloseFigure */
    NULL,                               /* pCreateCompatibleDC */
    RDS_CreateDC,                       /* pCreateDC */
    RDS_DeleteDC,                       /* pDeleteDC */
    NULL,                               /* pDeleteObject */
    NULL,                               /* pEllipse */
    NULL,                               /* pEndDoc */
    NULL,                               /* pEndPage */
    NULL,                               /* pEndPath */
    NULL,                               /* pEnumFonts */
    NULL,                               /* pExtEscape */
    NULL,                               /* pExtFloodFill */
    RDS_ExtTextOut,                     /* pExtTextOut */
    NULL,                               /* pFillPath */
    NULL,                               /* pFillRgn */
    NULL,                               /* pFontIsLinked */
    NULL,                               /* pFrameRgn */
    NULL,                               /* pGetBoundsRect */
    NULL,                               /* pGetCharABCWidths */
    NULL,                               /* pGetCharABCWidthsI */
    NULL,                               /* pGetCharWidth */
    NULL,                               /* pGetCharWidthInfo */
    RDS_GetDeviceCaps,                  /* pGetDeviceCaps */
    NULL,                               /* pGetDeviceGammaRamp */
    NULL,                               /* pGetFontData */
    NULL,                               /* pGetFontRealizationInfo */
    NULL,                               /* pGetFontUnicodeRanges */
    NULL,                               /* pGetGlyphIndices */
    NULL,                               /* pGetGlyphOutline */
    NULL,                               /* pGetICMProfile */
    NULL,                               /* pGetImage */
    NULL,                               /* pGetKerningPairs */
    NULL,                               /* pGetNearestColor */
    NULL,                               /* pGetOutlineTextMetrics */
    NULL,                               /* pGetPixel */
    NULL,                               /* pGetSystemPaletteEntries */
    NULL,                               /* pGetTextCharsetInfo */
    NULL,                               /* pGetTextExtentExPoint */
    NULL,                               /* pGetTextExtentExPointI */
    NULL,                               /* pGetTextFace */
    NULL,                               /* pGetTextMetrics */
    NULL,                               /* pGradientFill */
    NULL,                               /* pInvertRgn */
    RDS_LineTo,                         /* pLineTo */
    NULL,                               /* pMoveTo */
    NULL,                               /* pPaintRgn */
    NULL,                               /* pPatBlt */
    NULL,                               /* pPie */
    NULL,                               /* pPolyBezier */
    NULL,                               /* pPolyBezierTo */
    NULL,                               /* pPolyDraw */
    NULL,                               /* pPolyPolygon */
    NULL,                               /* pPolyPolyline */
    NULL,                               /* pPolyline */
    NULL,                               /* pPolylineTo */
    NULL,                               /* pPutImage */
    NULL,                               /* pRealizeDefaultPalette */
    NULL,                               /* pRealizePalette */
    RDS_Rectangle,                      /* pRectangle */
    NULL,                               /* pResetDC */
    NULL,                               /* pRoundRect */
    NULL,                               /* pSelectBitmap */
    NULL,                               /* pSelectBrush */
    NULL,                               /* pSelectFont */
    NULL,                               /* pSelectPalette */
    NULL,                               /* pSelectPen */
    NULL,                               /* pSetBkColor */
    NULL,                               /* pSetBoundsRect */
    NULL,                               /* pSetDCBrushColor */
    NULL,                               /* pSetDCPenColor */
    NULL,                               /* pSetDIBitsToDevice */
    NULL,                               /* pSetDeviceClipping */
    NULL,                               /* pSetDeviceGammaRamp */
    NULL,                               /* pSetPixel */
    NULL,                               /* pSetTextColor */
    NULL,                               /* pStartDoc */
    NULL,                               /* pStartPage */
    NULL,                               /* pStretchBlt */
    NULL,                               /* pStretchDIBits */
    NULL,                               /* pStrokeAndFillPath */
    NULL,                               /* pStrokePath */
    NULL,                               /* pUnrealizePalette */
    GDI_PRIORITY_GRAPHICS_DRV           /* priority */
};

// Initialize the driver
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("[DllMain] Called at %lu ms, fdwReason=%d\n", GetTickCount(), fdwReason);
    
    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hinstDLL);
        break;
    case DLL_PROCESS_DETACH:
        StopRDSClientPipe();
        break;
    }
    
    return TRUE;
}

// Entry point for Wine to get our driver
const struct gdi_dc_funcs * CDECL __wine_get_gdi_driver(unsigned int version)
{
    TRACE("[__wine_get_gdi_driver] Called at %lu ms, version=%u\n", GetTickCount(), version);
    if (version != WINE_GDI_DRIVER_VERSION)
    {
        ERR("version mismatch, driver wants %u but Wine has %u\n", WINE_GDI_DRIVER_VERSION, version);
        return NULL;
    }
    TRACE("[__wine_get_gdi_driver] Returning driver function table\n");
    return &rds_dc_funcs;
}