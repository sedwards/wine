
//#include "config.h"

#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
//#include <dlfcn.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS

#include "windows.h"
#include "wine/server.h"
#include "wine/debug.h"
#include "wine/list.h"

#define WINE_UNIX_LIB

/* driver.c */
#include <stdarg.h>
#include <stdlib.h>
//#include "windef.h"
//#include "winbase.h"
//#include "wingdi.h"
#include "wine/gdi_driver.h"

#include "wine/debug.h"


/* Connection to our RDS service */
extern BOOL rds_connect_service(void);
extern void rds_disconnect_service(void);

WINE_DEFAULT_DEBUG_CHANNEL(rds);

static struct gdi_dc_funcs winerds_driver_funcs;

/***********************************************************************
 *           DrvEnableDriver  (WINERDS.@)
 */
BOOL CDECL DrvEnableDriver(ULONG version, ULONG size, DRVENABLEDATA *data)
{
    WINE_TRACE("version %u, size %u, data %p\n", version, size, data);

    if (!rds_connect_service())
    {
        WINE_ERR("Failed to connect to RDS service\n");
        return FALSE;
    }

    if (size < sizeof(*data))
    {
        WINE_ERR("Invalid data size\n");
        return FALSE;
    }

    data->iDriverVersion = DDI_DRIVER_VERSION;
    data->c = sizeof(winerds_driver_funcs) / sizeof(DRVFN);
    data->pdrvfn = (DRVFN *)&winerds_driver_funcs;

    return TRUE;
}

/***********************************************************************
 *           DrvQueryDriverInfo  (WINERDS.@)
 */
BOOL CDECL DrvQueryDriverInfo(DWORD dwMode, PVOID pBuffer, DWORD cbBuf, PDWORD pcbNeeded)
{
    WINE_TRACE("dwMode %u, pBuffer %p, cbBuf %u, pcbNeeded %p\n", 
               dwMode, pBuffer, cbBuf, pcbNeeded);
    return FALSE;  /* Not implemented yet */
}

/* Initialize the driver funcs */
static struct gdi_dc_funcs winerds_driver_funcs =
{
    /* Basic drawing functions */
    NULL,                        /* pAbortDoc */
    NULL,                        /* pAbortPath */
    NULL,                        /* pAlphaBlend */
    NULL,                        /* pAngleArc */
    NULL,                        /* pArc */
    NULL,                        /* pArcTo */

#if 0
    NULL,                        /* pBeginPath */
    NULL,                        /* pBitBlt */
    NULL,                        /* pCanRenderFormat */
    NULL,                        /* pCancelDC */
    NULL,                        /* pCheckColorsInGamut */
    NULL,                        /* pChoosePixelFormat */
    NULL,                        /* pChord */
    NULL,                        /* pCloseFigure */
    NULL,                        /* pCreateBitmap */
    NULL,                        /* pCreateDC */
    NULL,                        /* pCreateDIBSection */
    NULL,                        /* pDeleteBitmap */
    NULL,                        /* pDeleteDC */
    NULL,                        /* pDeleteObject */
    NULL,                        /* pDescribePixelFormat */
    NULL,                        /* pDeviceCapabilities */
    NULL,                        /* pEllipse */
    NULL,                        /* pEndDoc */
    NULL,                        /* pEndPage */
    NULL,                        /* pEndPath */
    NULL,                        /* pEnumFonts */
    NULL,                        /* pEnumICMProfiles */
    NULL,                        /* pExcludeClipRect */
    NULL,                        /* pExtDeviceMode */
    NULL,                        /* pExtEscape */
    NULL,                        /* pExtFloodFill */
    NULL,                        /* pExtSelectClipRgn */
    NULL,                        /* pExtTextOut */
    NULL,                        /* pFillPath */
    NULL,                        /* pFillRgn */
    NULL,                        /* pFlattenPath */
    NULL,                        /* pFontIsLinked */
    NULL,                        /* pFrameRgn */
    NULL,                        /* pGdiComment */
    NULL,                        /* pGetBitmapBits */
    NULL,                        /* pGetBitmapDimension */
    NULL,                        /* pGetCharABCWidths */
    NULL,                        /* pGetCharABCWidthsI */
    NULL,                        /* pGetCharWidth */
    NULL,                        /* pGetDeviceCaps */
    NULL,                        /* pGetDeviceGammaRamp */
    NULL,                        /* pGetFontData */
    NULL,                        /* pGetFontUnicodeRanges */
    NULL,                        /* pGetGlyphIndices */
    NULL,                        /* pGetGlyphOutline */
    NULL,                        /* pGetICMProfile */
    NULL,                        /* pGetImage */
    NULL,                        /* pGetKerningPairs */
    NULL,                        /* pGetNearestColor */
    NULL,                        /* pGetOutlineTextMetrics */
    NULL,                        /* pGetPixel */
    NULL,                        /* pGetPixelFormat */
    NULL,                        /* pGetSystemPaletteEntries */
    NULL,                        /* pGetTextCharsetInfo */
    NULL,                        /* pGetTextExtentExPoint */
    NULL,                        /* pGetTextExtentPoint */
    NULL,                        /* pGetTextFace */
    NULL,                        /* pGetTextMetrics */
    NULL,                        /* pGradientFill */
    NULL,                        /* pIntersectClipRect */
    NULL,                        /* pInvertRgn */
    winerds_LineTo,              /* pLineTo */
    NULL,                        /* pModifyWorldTransform */
    winerds_MoveTo,              /* pMoveTo */
    NULL,                        /* pOffsetClipRgn */
    NULL,                        /* pOffsetViewportOrg */
    NULL,                        /* pOffsetWindowOrg */
    NULL,                        /* pPaintRgn */
    NULL,                        /* pPatBlt */
    NULL,                        /* pPie */
    NULL,                        /* pPolyBezier */
    NULL,                        /* pPolyBezierTo */
    NULL,                        /* pPolyDraw */
    NULL,                        /* pPolyPolygon */
    NULL,                        /* pPolyPolyline */
    NULL,                        /* pPolygon */
    NULL,                        /* pPolyline */
    NULL,                        /* pPolylineTo */
    NULL,                        /* pPutImage */
    NULL,                        /* pRealizeDefaultPalette */
    NULL,                        /* pRealizePalette */
    winerds_Rectangle,           /* pRectangle */
    NULL,                        /* pResetDC */
    NULL,                        /* pRestoreDC */
    NULL,                        /* pRoundRect */
    NULL,                        /* pSaveDC */
    NULL,                        /* pScaleViewportExt */
    NULL,                        /* pScaleWindowExt */
    NULL,                        /* pSelectBitmap */
    NULL,                        /* pSelectBrush */
    NULL,                        /* pSelectClipPath */
    NULL,                        /* pSelectFont */
    NULL,                        /* pSelectPalette */
    NULL,                        /* pSelectPen */
    NULL,                        /* pSetArcDirection */
    NULL,                        /* pSetBitmapBits */
    NULL,                        /* pSetBitmapDimension */
    NULL,                        /* pSetBkColor */
    NULL,                        /* pSetBkMode */
    NULL,                        /* pSetBoundsRect */
    NULL,                        /* pSetDCBrushColor */
    NULL,                        /* pSetDCPenColor */
    NULL,                        /* pSetDIBColorTable */
    NULL,                        /* pSetDIBitsToDevice */
    NULL,                        /* pSetDeviceClipping */
    NULL,                        /* pSetDeviceGammaRamp */
    NULL,                        /* pSetLayout */
    NULL,                        /* pSetMapMode */
    NULL,                        /* pSetMapperFlags */
    NULL,                        /* pSetPixel */
    NULL,                        /* pSetPixelFormat */
    NULL,                        /* pSetPolyFillMode */
    NULL,                        /* pSetROP2 */
    NULL,                        /* pSetRelAbs */
    NULL,                        /* pSetStretchBltMode */
    NULL,                        /* pSetTextAlign */
    NULL,                        /* pSetTextCharacterExtra */
    NULL,                        /* pSetTextColor */
    NULL,                        /* pSetTextJustification */
    NULL,                        /* pSetViewportExt */
    NULL,                        /* pSetViewportOrg */
    NULL,                        /* pSetWindowExt */
    NULL,                        /* pSetWindowOrg */
    NULL,                        /* pSetWorldTransform */
    NULL,                        /* pStartDoc */
    NULL,                        /* pStartPage */
    NULL,                        /* pStretchBlt */
    NULL,                        /* pStretchDIBits */
    NULL,                        /* pStrokeAndFillPath */
    NULL,                        /* pStrokePath */
    winerds_TextOut,             /* pTextOut */
    NULL,                        /* pUpdateColors */
    NULL,                        /* pWidenPath */
    NULL,                        /* pwglCopyContext */
    NULL,                        /* pwglCreateContext */
    NULL,                        /* pwglCreateContextAttribsARB */
    NULL,                        /* pwglDeleteContext */
    NULL,                        /* pwglGetPbufferDCARB */
    NULL,                        /* pwglGetProcAddress */
    NULL,                        /* pwglMakeCurrent */
    NULL,                        /* pwglMakeContextCurrentARB */
    NULL,                        /* pwglSetPixelFormatWINE */
    NULL,                        /* pwglShareLists */
    NULL,                        /* pwglSwapBuffers */
#endif
    NULL                         /* pwglUseFontBitmapsA */
};

