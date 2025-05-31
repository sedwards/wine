
#include "rdsdrv_dll.h"

#include "ntuser.h"
#include "winuser.h"

#include "min_winddi.h"
//#include "ddk/wdm.h"

#include <string.h> // For memset, wcscmp, wcsncpy
#include "pipe_client.h"
#include "gdi_driver.h"
#include "rds.h" // Should include rds_message.h

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(winerds);

// Define RDSDRV_PDEVICE structure
typedef struct _RDSDRV_PDEVICE
{
    HDC         hdc;
    DWORD       session_id; // Example field, not used yet
    BOOL        is_screen;
    RECT        bounds;
    DWORD       bpp;
    // Screen specific dimensions
    int         screen_width;
    int         screen_height;
    int         screen_bpp;
    struct _RDSDRV_PDEVICE* next;
} RDSDRV_PDEVICE;

struct rds_physdev {
    struct gdi_physdev dev;  // must be first!
    RDSDevice *pdc;          // your custom pointer to device context
};

static RDSDRV_PDEVICE *rds_pdev_list_head = NULL;
static CRITICAL_SECTION rds_pdev_list_lock; // Used by DllMain, RdsCreateDCW, RdsDeleteDCW

// Forward declarations for DC functions
static HDC WINAPI RdsCreateDCW(PHYSDEV dev, LPCWSTR driver, LPCWSTR device, LPCWSTR output, const DEVMODEW *devmode);
static BOOL WINAPI RdsDeleteDCW(PHYSDEV dev, HDC hdc);
int WINAPI RdsGetDeviceCaps(PHYSDEV dev, int index);
// Add other DC function forward declarations if they are part of rds_dc_funcs

// Helper to find RDSDRV_PDEVICE by HDC (needs to be non-static for RdsGetDeviceCaps)
RDSDRV_PDEVICE* find_device_by_hdc(HDC hdc)
{
    RDSDRV_PDEVICE *pdev;
    EnterCriticalSection(&rds_pdev_list_lock);
    for (pdev = rds_pdev_list_head; pdev; pdev = pdev->next)
    {
        if (pdev->hdc == hdc) break;
    }
    LeaveCriticalSection(&rds_pdev_list_lock);
    return pdev;
}


BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, void *reserved)
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        InitializeCriticalSection(&rds_pdev_list_lock);
        FIXME("winerds.drv: Initialized rds_pdev_list_lock.\n");
        // register_rds_driver(); // Now handled by DrvEnableDriver
        FIXME("winerds.drv: register_rds_driver() call commented out in DllMain.\n");
        if (!StartRDSClientPipe())
        {
            FIXME("winerds.drv: CRITICAL ERROR: Failed to start RDS client pipe.\n");
        }
        else
        {
            FIXME("winerds.drv: RDS client pipe started successfully.\n");
        }
        break;
    case DLL_PROCESS_DETACH:
        FIXME("winerds.drv: DllMain called with DLL_PROCESS_DETACH.\n");
        StopRDSClientPipe();
        FIXME("winerds.drv: RDS client pipe stopped.\n");
        DeleteCriticalSection(&rds_pdev_list_lock);
        FIXME("winerds.drv: Deleted rds_pdev_list_lock.\n");
        // TODO: Clean up rds_pdev_list_head if any entries remain
        break;
    }
    return TRUE;
}

// --- Core Driver Functions ---
HSURF WINAPI DrvEnableSurface(DHPDEV dhpdev)
{
    RDS_MESSAGE msg;
    FIXME("winerds.drv: DrvEnableSurface called with dhpdev=%p\n", dhpdev);
    msg.msgType = RDS_MSG_ENABLE_PRIMARY_SURFACE;
    msg.params.enablePrimarySurface.surfaceId = 1;
    if (!SendRDSMessage(&msg, NULL, 0))
    {
        FIXME("winerds.drv: ERR - Failed to send RDS_MSG_ENABLE_PRIMARY_SURFACE\n");
        return NULL;
    }
    FIXME("winerds.drv: DrvEnableSurface returning dummy HSURF 1 for primary surface.\n");
    return (HSURF)1;
}

DHPDEV WINAPI DrvEnablePDEV(
    DEVMODEW *pdm, LPWSTR pwszLogAddress, ULONG cPat, HSURF *phsurfPatterns,
    ULONG cjCaps, ULONG *pdevcaps, ULONG cjDevInfo, DEVINFO *pdi,
    HDEV hdev, LPWSTR pwszDeviceName, HANDLE hDriver)
{
    FIXME("winerds.drv: DrvEnablePDEV called. Device: %S, LogAddress: %S\n", pwszDeviceName, pwszLogAddress);
    if (!pdi || cjDevInfo < sizeof(DEVINFO)) {
        FIXME("winerds.drv: DrvEnablePDEV ERROR - DEVINFO buffer too small or NULL.\n");
        return NULL;
    }
    if (!pdevcaps || cjCaps < sizeof(GDIINFO)) {
         FIXME("winerds.drv: DrvEnablePDEV ERROR - GDIINFO buffer (pdevcaps) too small or NULL.\n");
         return NULL;
    }
    memset(pdi, 0, sizeof(DEVINFO));
    memset(pdevcaps, 0, sizeof(GDIINFO));
    GDIINFO* pGdiInfo = (GDIINFO*)pdevcaps;

    // Fill DEVINFO (pdi)
    pdi->flGraphicsCaps = GCAPS_OPAQUERECT | GCAPS_HORIZSTRIKE | GCAPS_VERTSTRIKE | GCAPS_RASTERCAPS;
    pdi->iDitherFormat = BMF_32BPP;
    pdi->cxDither = 0;
    pdi->cyDither = 0;
    pdi->hpalDefault = 0;
    pdi->pfn = (PFN*) &rds_dc_funcs; // Associate DC functions

    // Fill GDIINFO
    pGdiInfo->ulVersion = DRIVERVERSION;
    pGdiInfo->ulTechnology = DT_RASDISPLAY;
    pGdiInfo->ulHorzRes = (pdm && (pdm->dmFields & DM_PELSWIDTH)) ? pdm->dmPelsWidth : 800;
    pGdiInfo->ulVertRes = (pdm && (pdm->dmFields & DM_PELSHEIGHT)) ? pdm->dmPelsHeight : 600;
    pGdiInfo->cBitsPixel = (pdm && (pdm->dmFields & DM_BITSPERPEL)) ? pdm->dmBitsPerPel : 32;
    pGdiInfo->cPlanes = 1;
    // Copy DEVINFO into GDIINFO's devinfo member
    memcpy(&pGdiInfo->devinfo, pdi, sizeof(DEVINFO));


    FIXME("winerds.drv: DrvEnablePDEV returning dummy DHPDEV 1 (HDEV %p).\n", hdev);
    return (DHPDEV)hdev; // Often HDEV is used as DHPDEV or part of it
}

VOID WINAPI DrvCompletePDEV(DHPDEV dhpdev, HDEV hdev) {
    FIXME("winerds.drv: STUB DrvCompletePDEV called for dhpdev=%p, hdev=%p\n", dhpdev, hdev);
}
VOID WINAPI DrvDisablePDEV(DHPDEV dhpdev) {
    FIXME("winerds.drv: STUB DrvDisablePDEV called for dhpdev=%p\n", dhpdev);
}
VOID WINAPI DrvDisableSurface(DHPDEV dhpdev, HSURF hsurf) {
    FIXME("winerds.drv: STUB DrvDisableSurface called for dhpdev=%p, hsurf=%p\n", dhpdev, hsurf);
}

ULONG WINAPI DrvGetModes(HANDLE hDriver, ULONG cjSize, DEVMODEW *pdm)
{
    FIXME("winerds.drv: DrvGetModes called. hDriver=%p, cjSize=%lu, pdm=%p\n", hDriver, cjSize, pdm);
    if (pdm == NULL) {
        return sizeof(DEVMODEW);
    }
    if (cjSize < sizeof(DEVMODEW)) {
        return 0;
    }
    memset(pdm, 0, sizeof(DEVMODEW));
    const WCHAR deviceName[] = L"WineRDS Virtual Display";
    lstrcpynW((LPWSTR)pdm->dmDeviceName, deviceName, CCHDEVICENAME);
    pdm->dmSize = sizeof(DEVMODEW);
    pdm->dmSpecVersion = DM_SPECVERSION;
    pdm->dmDriverVersion = DRIVERVERSION;
    pdm->dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL | DM_DISPLAYFREQUENCY | DM_DISPLAYFLAGS;
    pdm->dmPelsWidth = 800; // Default screen width
    pdm->dmPelsHeight = 600; // Default screen height
    pdm->dmBitsPerPel = 32;  // Default BPP
    pdm->dmDisplayFrequency = 60;
    pdm->dmDisplayFlags = 0;
    FIXME("winerds.drv: DrvGetModes filled DEVMODEW: %ldx%ldx%ld @ %ldHz\n",
           pdm->dmPelsWidth, pdm->dmPelsHeight, pdm->dmBitsPerPel, pdm->dmDisplayFrequency);
    return sizeof(DEVMODEW);
}

DHPDEV DrvEnablePDEV(DEVMODEW *pdevmode, LPWSTR pwszLogAddress,
    ULONG cPat, HSURF *phsurfPatterns, ULONG cjCaps, ULONG *pdevcaps,
    ULONG cjDevInfo, DEVINFO *pdi, HDEV hdev, PWSTR pwszDeviceName,
    HANDLE hDriver);

static DRVFN gdi_driver_core_funcs[] = {
    { INDEX_DrvEnablePDEV,          (PFN)DrvEnablePDEV         },
    { INDEX_DrvCompletePDEV,        (PFN)DrvCompletePDEV       },
    { INDEX_DrvDisablePDEV,         (PFN)DrvDisablePDEV        },
    { INDEX_DrvEnableSurface,       (PFN)DrvEnableSurface      },
    { INDEX_DrvDisableSurface,      (PFN)DrvDisableSurface     },
    { INDEX_DrvGetModes,            (PFN)DrvGetModes           },
};

BOOL WINAPI DrvEnableDriver( ULONG iEngineVersion, ULONG cj, DRVENABLEDATA *pded )
{
    FIXME("winerds.drv: DrvEnableDriver called, version 0x%lx, size %lu\n", iEngineVersion, cj);
    if (iEngineVersion < DRIVERVERSION) {
        return FALSE;
    }
    if (cj < sizeof(DRVENABLEDATA)) {
        return FALSE;
    }
    pded->iDriverVersion = DRIVERVERSION;
    pded->c = ARRAY_SIZE(gdi_driver_core_funcs);
    pded->pdrvfn = gdi_driver_core_funcs;
    FIXME("winerds.drv: DrvEnableDriver registered %lu core functions.\n", pded->c);
    return TRUE;
}

// --- DC Functions ---
static HDC WINAPI RdsCreateDCW(PHYSDEV dev, LPCWSTR driver, LPCWSTR device, LPCWSTR output, const DEVMODEW *devmode)
{
    RDSDRV_PDEVICE *pdev;
    HDC hdc = wine_CreateDCW(driver, device, output, devmode); // Call Wine's implementation
    FIXME("winerds.drv: RdsCreateDCW called for driver='%S', device='%S', output='%S', hdc_wine=%p\n", driver, device, output, hdc);

    if (!hdc) return NULL;

    pdev = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(RDSDRV_PDEVICE));
    if (!pdev) {
        FIXME("winerds.drv: RdsCreateDCW - HeapAlloc failed\n");
        wine_DeleteDC(hdc); // Clean up Wine's DC
        return NULL;
    }

    pdev->hdc = hdc;
    if (device && (wcscmp(device, L"DISPLAY") == 0 || wcsstr(device, L"\\\\.\\DISPLAY") == device)) {
        pdev->is_screen = TRUE;
        // Use devmode if provided, otherwise default
        pdev->screen_width = (devmode && (devmode->dmFields & DM_PELSWIDTH)) ? devmode->dmPelsWidth : 800;
        pdev->screen_height = (devmode && (devmode->dmFields & DM_PELSHEIGHT)) ? devmode->dmPelsHeight : 600;
        pdev->screen_bpp = (devmode && (devmode->dmFields & DM_BITSPERPEL)) ? devmode->dmBitsPerPel : 32;

        pdev->bounds.left = 0;
        pdev->bounds.top = 0;
        pdev->bounds.right = pdev->screen_width;
        pdev->bounds.bottom = pdev->screen_height;
        pdev->bpp = pdev->screen_bpp;
        FIXME("winerds.drv: RdsCreateDCW created SCREEN pdev for HDC %p (%dx%dx%d)\n", hdc, pdev->screen_width, pdev->screen_height, pdev->screen_bpp);
    } else {
        pdev->is_screen = FALSE;
        // For non-screen DCs, dimensions might come from a DIBSection or be less relevant here
        // Get them from the HDC itself if needed
        pdev->screen_width = GetDeviceCaps(hdc, HORZRES);
        pdev->screen_height = GetDeviceCaps(hdc, VERTRES);
        pdev->screen_bpp = GetDeviceCaps(hdc, BITSPIXEL);
        FIXME("winerds.drv: RdsCreateDCW created non-screen pdev for HDC %p (%dx%dx%d)\n", hdc, pdev->screen_width, pdev->screen_height, pdev->screen_bpp);
    }

    EnterCriticalSection(&rds_pdev_list_lock);
    pdev->next = rds_pdev_list_head;
    rds_pdev_list_head = pdev;
    LeaveCriticalSection(&rds_pdev_list_lock);

    // Associate our PDEVICE with the HDC for later retrieval (e.g. in RdsGetDeviceCaps if not using find_device_by_hdc)
    // This is a common pattern if PHYSDEV passed to RdsGetDeviceCaps is the HDC itself or directly related.
    // However, standard Wine DDI passes a PPHYSDEV.
    // For now, find_device_by_hdc will be used.

    return hdc; // Return Wine's HDC
}

static BOOL WINAPI RdsDeleteDCW(PHYSDEV dev, HDC hdc)
{
    RDSDRV_PDEVICE *pdev, *prev_pdev;
    FIXME("winerds.drv: RdsDeleteDCW called for hdc=%p\n", hdc);

    EnterCriticalSection(&rds_pdev_list_lock);
    for (pdev = rds_pdev_list_head, prev_pdev = NULL; pdev; prev_pdev = pdev, pdev = pdev->next) {
        if (pdev->hdc == hdc) {
            if (prev_pdev) prev_pdev->next = pdev->next;
            else rds_pdev_list_head = pdev->next;
            break;
        }
    }
    LeaveCriticalSection(&rds_pdev_list_lock);

    if (pdev) HeapFree(GetProcessHeap(), 0, pdev);

    return wine_DeleteDC(hdc); // Call Wine's implementation
}

int WINAPI RdsGetDeviceCaps(PHYSDEV dev, int index)
{
    RDSDRV_PDEVICE* rds_pdev = NULL;
    // PPHYSDEV is usually (DC* dc)->physDev. And DC has the HDC.
    // So if 'dev' is PPHYSDEV, then dev->pdc->hdc should be the HDC.
    if (dev && dev->pdc && dev->pdc->hdc)
    {
        rds_pdev = find_device_by_hdc(dev->pdc->hdc);
    }

    // FIXME("winerds.drv: RdsGetDeviceCaps: hdc=%p, index=%d. rds_pdev found: %p\n",
    //        (rds_pdev ? rds_pdev->hdc : NULL), index, rds_pdev);

    if (rds_pdev && rds_pdev->is_screen)
    {
        FIXME("winerds.drv: RdsGetDeviceCaps for SCREEN DC: index %d\n", index);
        switch (index)
        {
            case HORZRES:           return rds_pdev->screen_width;
            case VERTRES:           return rds_pdev->screen_height;
            case BITSPIXEL:         return rds_pdev->screen_bpp;
            case PLANES:            return 1;
            case NUMCOLORS:         return (rds_pdev->screen_bpp > 8) ? -1 : (1 << rds_pdev->screen_bpp);
            case ASPECTX:           return 36;
            case ASPECTY:           return 36;
            case ASPECTXY:          return 51;
            case LOGPIXELSX:        return 96;
            case LOGPIXELSY:        return 96;
            case TECHNOLOGY:        return DT_RASDISPLAY;
            case DRIVERVERSION:     return DRIVER_VERSION; // Use same version as DrvEnableDriver
            case RASTERCAPS:
                return RC_BITBLT   | RC_DI_BITMAP | RC_DIBTODEV | RC_STRETCHBLT |
                       RC_STRETCHDIB | RC_PALETTE | RC_SCALING | RC_GDI20_OUTPUT;
            default:
                FIXME("winerds.drv: RdsGetDeviceCaps for SCREEN DC: Unhandled index %d, passing to wine_GetDeviceCaps\n", index);
                return wine_GetDeviceCaps( dev->pdc->hdc, index );
        }
    }
    else if (dev && dev->pdc && dev->pdc->hdc)
    {
        FIXME("winerds.drv: RdsGetDeviceCaps for non-screen DC: index %d, passing to wine_GetDeviceCaps\n", index);
        return wine_GetDeviceCaps( dev->pdc->hdc, index );
    }

    FIXME("winerds.drv: WARN - RdsGetDeviceCaps: dev, pdc, or hdc is NULL, or rds_pdev not found. index=%d. Returning 0.\n", index);
    return 0;
}


// Actual rds_dc_funcs table using the functions from graphics.c
// Need to ensure those functions are declared before this table or defined as static here.
// For now, using placeholders for functions not yet fully defined or moved from graphics.c
extern BOOL rds_MoveTo(PHYSDEV,int,int); // Assuming these are in graphics.c
extern BOOL rds_LineTo(PHYSDEV,int,int);
extern BOOL rds_Rectangle(PHYSDEV,int,int,int,int);
extern BOOL rds_TextOut(PHYSDEV,int,int,LPCWSTR,int);
// extern BOOL rds_BitBlt(PHYSDEV,int,int,int,int,PHYSDEV,int,int,DWORD); // etc.

static const struct gdi_dc_funcs rds_dc_funcs =
{
    NULL, // DrvRealizeBrush
    NULL, // DrvDitherColor
    NULL, // DrvStrokePath
    NULL, // DrvFillPath
    NULL, // DrvStrokeAndFillPath
    NULL, // DrvPaint
    NULL, // (PFN_DrvBitBlt) rds_BitBlt,
    NULL, // DrvCopyBits
    NULL, // DrvStretchBlt
    NULL, // DrvSetPalette
    (PFN_DrvTextOut) rds_TextOut, // This is for DrvTextOut, not ExtTextOutW
    NULL, // DrvEscape
    NULL, // DrvDrawEscape
    NULL, // DrvSetPixel
    (PFN_DrvMoveTo) rds_MoveTo,
    (PFN_DrvLineTo) rds_LineTo,
    NULL, // DrvPolyline
    NULL, // DrvExcludeClipRect
    NULL, // DrvIntersectClipRect
    NULL, // DrvSaveScreenBits
    RdsCreateDCW,       // DrvCreateDC (using our RdsCreateDCW)
    RdsDeleteDCW,       // DrvDeleteDC (using our RdsDeleteDCW)
    RdsGetDeviceCaps,   // DrvGetDeviceCaps (using our RdsGetDeviceCaps)
    NULL, // DrvSetPointerShape
    NULL, // DrvMovePointer
    (PFN_DrvRectangle) rds_Rectangle,
    NULL, // DrvEllipse
    NULL, // DrvPolygon
    NULL, // DrvPolyPolygon
    // ... (rest of the fields, ensure they match the gdi_dc_funcs structure in wine/gdi_driver.h)
    // The number of NULLs here must match the structure definition precisely.
    // Based on a typical gdi_dc_funcs struct:
    NULL, /* DrvGetDirectDrawInfo */
    NULL, /* DrvEnableDirectDraw */
    NULL, /* DrvDisableDirectDraw */
    NULL, /* DrvGetModes - This is for the DC, DrvGetModes in core table is for driver init */
    0,    /* reserved */
    NULL, /* DrvCopyDeviceBitmap */
    NULL, /* DrvAlphaBlend */
    NULL, /* DrvGradientFill */
    NULL, /* DrvTransparentBlt */
    NULL, /* DrvNotify */
    NULL, /* DrvResetPDEV */
    NULL, /* DrvGetGlyphMode */
    NULL, /* DrvSynchronize */
    NULL, /* DrvReserved1 */
    NULL, /* DrvReserved2 */
    NULL, /* DrvReserved3 */
    NULL, /* DrvReserved4 */
    NULL, /* DrvReserved5 */
    NULL, /* DrvReserved6 */
    NULL, /* DrvReserved7 */
    NULL, /* DrvReserved8 */
    NULL, /* DrvSetContextHook */
    NULL, /* DrvSetClipList */
    NULL, /* DrvDestroyClipList */
    NULL, /* DrvGetPaletteEntries */
    NULL, /* DrvSetPaletteEntries */
    NULL, /* DrvPlgBlt */
    NULL, /* DrvStretchBltROP */
    NULL, /* DrvSynchronizeSurface */
};

