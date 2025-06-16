#include "rdsdrv_dll.h"

#include "ntuser.h"
#include "winuser.h"

#include "rdsgdi_driver.h"
#include "rds.h"

#include "pipe_client.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(winerds);

/*
 * Surface Routing Fix for CreateDC
 *
 * The issue is that CreateDC creates new surfaces instead of using
 * the existing RDS surface. We need to route all display DCs to
 * the same RDS surface.
 */

// In your graphics.c or winerdsdrv_main.c, add this global tracking
static DWORD g_primary_surface_id = 1; // The surface termsrv displays
static RDS_SURFACE *g_primary_surface = NULL; // Cache the primary surface

#if 0
// Update your RDS drawing functions to always use the primary surface
BOOL RDS_MoveTo(PHYSDEV dev, INT x, INT y)
{
    RDS_MESSAGE msg;

    TRACE("RDS_MoveTo called: dev=%p, hdc=%p, x=%d, y=%d\n", dev, dev ? dev->hdc : NULL, x, y);

    msg.msgType = RDS_MSG_MOVE_TO;
    // ALWAYS use the primary surface that termsrv displays
    msg.params.moveTo.surfaceId = g_primary_surface_id;
    msg.params.moveTo.x = x;
    msg.params.moveTo.y = y;

    if (!SendRDSMessage(&msg, NULL, 0))
    {
        ERR("Failed to send RDS_MSG_MOVE_TO\n");
        return FALSE;
    }

    TRACE("RDS_MoveTo: Sent to surface %lu\n", g_primary_surface_id);

    return GET_NEXT_PHYSDEV(dev, pMoveTo)->funcs->pMoveTo(GET_NEXT_PHYSDEV(dev, pMoveTo), x, y);
}

BOOL RDS_LineTo(PHYSDEV dev, INT x, INT y)
{
    RDS_MESSAGE msg;
    LOGPEN logPen;

    TRACE("RDS_LineTo called: dev=%p, hdc=%p, x=%d, y=%d\n", dev, dev ? dev->hdc : NULL, x, y);

    if (dev && dev->hdc) {
        HPEN hCurrentPen = GetCurrentObject(dev->hdc, OBJ_PEN);
        if (hCurrentPen) {
            GetObjectW(hCurrentPen, sizeof(LOGPEN), &logPen);
        } else {
            memset(&logPen, 0, sizeof(LOGPEN));
            logPen.lopnStyle = PS_SOLID;
            logPen.lopnWidth.x = 1;
            logPen.lopnColor = RGB(0,0,0);
            WARN("Could not get current pen, using default for LineTo.\n");
        }
    } else {
        memset(&logPen, 0, sizeof(LOGPEN));
        logPen.lopnStyle = PS_SOLID;
        logPen.lopnWidth.x = 1;
        logPen.lopnColor = RGB(0,0,0);
        WARN("dev or dev->hdc is NULL, using default pen for LineTo.\n");
    }

    msg.msgType = RDS_MSG_LINE_TO;
    // ALWAYS use the primary surface
    msg.params.lineTo.surfaceId = g_primary_surface_id;
    msg.params.lineTo.x = x;
    msg.params.lineTo.y = y;
    msg.params.lineTo.color = logPen.lopnColor;
    msg.params.lineTo.lopnStyle = logPen.lopnStyle;
    msg.params.lineTo.lopnWidth_x = logPen.lopnWidth.x;

    if (!SendRDSMessage(&msg, NULL, 0))
    {
        ERR("Failed to send RDS_MSG_LINE_TO\n");
        return FALSE;
    }

    TRACE("RDS_LineTo: Sent to surface %lu\n", g_primary_surface_id);

    return GET_NEXT_PHYSDEV(dev, pLineTo)->funcs->pLineTo(GET_NEXT_PHYSDEV(dev, pLineTo), x, y);
}
#endif

BOOL RDS_MoveTo(PHYSDEV dev, INT x, INT y)
{
    printf("WINERDS: *** DRIVER FUNCTION CALLED *** MoveTo(%d, %d)\n", x, y);

    RDS_MESSAGE msg;
    msg.msgType = RDS_MSG_MOVE_TO;
    msg.params.moveTo.surfaceId = 1;
    msg.params.moveTo.x = x;
    msg.params.moveTo.y = y;

    if (SendRDSMessage(&msg, NULL, 0)) {
        printf("WINERDS: MoveTo message sent successfully\n");
    } else {
        printf("WINERDS: MoveTo message FAILED\n");
    }

    return GET_NEXT_PHYSDEV(dev, pMoveTo)->funcs->pMoveTo(GET_NEXT_PHYSDEV(dev, pMoveTo), x, y);
}

BOOL RDS_LineTo(PHYSDEV dev, INT x, INT y)
{
    printf("WINERDS: *** DRIVER FUNCTION CALLED *** LineTo(%d, %d)\n", x, y);

    RDS_MESSAGE msg;
    msg.msgType = RDS_MSG_LINE_TO;
    msg.params.lineTo.surfaceId = 1;
    msg.params.lineTo.x = x;
    msg.params.lineTo.y = y;
    msg.params.lineTo.color = RGB(255, 0, 0);
    msg.params.lineTo.lopnStyle = PS_SOLID;
    msg.params.lineTo.lopnWidth_x = 1;

    if (SendRDSMessage(&msg, NULL, 0)) {
        printf("WINERDS: LineTo message sent successfully\n");
    } else {
        printf("WINERDS: LineTo message FAILED\n");
    }

    return GET_NEXT_PHYSDEV(dev, pLineTo)->funcs->pLineTo(GET_NEXT_PHYSDEV(dev, pLineTo), x, y);
}


BOOL RDS_Rectangle(PHYSDEV dev, INT left, INT top, INT right, INT bottom)
{
    RDS_MESSAGE msg;
    LOGPEN logPen;
    LOGBRUSH logBrush;
    BOOL is_filled = FALSE;

    TRACE("RDS_Rectangle called: dev=%p, hdc=%p, rect=(%d,%d,%d,%d)\n",
          dev, dev ? dev->hdc : NULL, left, top, right, bottom);

    if (dev && dev->hdc) {
        HPEN hCurrentPen = GetCurrentObject(dev->hdc, OBJ_PEN);
        HBRUSH hCurrentBrush = GetCurrentObject(dev->hdc, OBJ_BRUSH);

        if (hCurrentPen) {
            GetObjectW(hCurrentPen, sizeof(LOGPEN), &logPen);
        } else {
            memset(&logPen, 0, sizeof(LOGPEN));
            logPen.lopnStyle = PS_SOLID;
            logPen.lopnWidth.x = 1;
            logPen.lopnColor = RGB(0,0,0);
            WARN("Could not get current pen, using default for Rectangle.\n");
        }

        if (hCurrentBrush) {
            GetObjectW(hCurrentBrush, sizeof(LOGBRUSH), &logBrush);
            if (logBrush.lbStyle != BS_NULL && logBrush.lbStyle != BS_HOLLOW) {
                is_filled = TRUE;
            }
        } else {
            memset(&logBrush, 0, sizeof(LOGBRUSH));
            logBrush.lbStyle = BS_NULL;
            WARN("Could not get current brush, assuming no fill for Rectangle.\n");
        }
    } else {
        memset(&logPen, 0, sizeof(LOGPEN));
        logPen.lopnStyle = PS_SOLID;
        logPen.lopnWidth.x = 1;
        logPen.lopnColor = RGB(0,0,0);
        memset(&logBrush, 0, sizeof(LOGBRUSH));
        logBrush.lbStyle = BS_NULL;
        WARN("dev or dev->hdc is NULL, using default pen/brush for Rectangle.\n");
    }

    msg.msgType = RDS_MSG_RECTANGLE;
    // ALWAYS use the primary surface
    msg.params.rectangle.surfaceId = g_primary_surface_id;
    msg.params.rectangle.left = left;
    msg.params.rectangle.top = top;
    msg.params.rectangle.right = right;
    msg.params.rectangle.bottom = bottom;

    msg.params.rectangle.color = logPen.lopnColor;
    msg.params.rectangle.filled = is_filled;
    msg.params.rectangle.pen_lopnStyle = logPen.lopnStyle;
    msg.params.rectangle.pen_lopnWidth_x = logPen.lopnWidth.x;

    msg.params.rectangle.brush_lbStyle = logBrush.lbStyle;
    msg.params.rectangle.brush_lbColor = logBrush.lbColor;
    msg.params.rectangle.brush_lbHatch = logBrush.lbHatch;

    if (!SendRDSMessage(&msg, NULL, 0))
    {
        ERR("Failed to send RDS_MSG_RECTANGLE\n");
        return FALSE;
    }

    TRACE("RDS_Rectangle: Sent to surface %lu\n", g_primary_surface_id);

    return GET_NEXT_PHYSDEV(dev, pRectangle)->funcs->pRectangle(GET_NEXT_PHYSDEV(dev, pRectangle), left, top, right, bottom);
}

BOOL RDS_ExtTextOut(PHYSDEV dev, INT x, INT y, UINT flags, const RECT *rect,
                    LPCWSTR str, UINT count, const INT *dx)
{
    RDS_MESSAGE msg;
    LOGFONTW logFont;
    COLORREF text_fg_color = RGB(0,0,0);
    COLORREF text_bk_color = RGB(255,255,255);
    INT bk_mode = OPAQUE;
    DWORD data_size = 0;

    TRACE("RDS_ExtTextOut called: dev=%p, hdc=%p, pos=(%d,%d), count=%d\n",
          dev, dev ? dev->hdc : NULL, x, y, count);

    if (count <= 0 || !str) {
        return GET_NEXT_PHYSDEV(dev, pExtTextOut)->funcs->pExtTextOut(
            GET_NEXT_PHYSDEV(dev, pExtTextOut), x, y, flags, rect, str, count, dx);
    }

    if (dev && dev->hdc) {
        HFONT hCurrentFont = GetCurrentObject(dev->hdc, OBJ_FONT);
        if (hCurrentFont) {
            GetObjectW(hCurrentFont, sizeof(LOGFONTW), &logFont);
        } else {
            memset(&logFont, 0, sizeof(LOGFONTW));
            lstrcpynW(logFont.lfFaceName, L"System", LF_FACESIZE);
            logFont.lfHeight = -12;
            WARN("Could not get current font, using default for ExtTextOut.\n");
        }
        text_fg_color = GetTextColor(dev->hdc);
        text_bk_color = GetBkColor(dev->hdc);
        bk_mode = GetBkMode(dev->hdc);
    } else {
        memset(&logFont, 0, sizeof(LOGFONTW));
        lstrcpynW(logFont.lfFaceName, L"System", LF_FACESIZE);
        logFont.lfHeight = -12;
        WARN("dev or dev->hdc is NULL, using defaults for ExtTextOut.\n");
    }

    data_size = count * sizeof(WCHAR);

    msg.msgType = RDS_MSG_TEXT_OUT;
    // ALWAYS use the primary surface
    msg.params.textOut.surfaceId = g_primary_surface_id;
    msg.params.textOut.x = x;
    msg.params.textOut.y = y;

    msg.params.textOut.color = text_fg_color;
    msg.params.textOut.text_bk_color = text_bk_color;
    msg.params.textOut.bk_mode = bk_mode;

    msg.params.textOut.count = count;
    msg.params.textOut.data_size = data_size;

    msg.params.textOut.font_lfHeight = logFont.lfHeight;
    msg.params.textOut.font_lfWidth = logFont.lfWidth;
    msg.params.textOut.font_lfWeight = logFont.lfWeight;
    msg.params.textOut.font_lfItalic = logFont.lfItalic;
    msg.params.textOut.font_lfUnderline = logFont.lfUnderline;
    msg.params.textOut.font_lfStrikeOut = logFont.lfStrikeOut;
    msg.params.textOut.font_lfCharSet = logFont.lfCharSet;
    lstrcpynW(msg.params.textOut.font_lfFaceName, logFont.lfFaceName, LF_FACESIZE);
    msg.params.textOut.font_lfFaceName[LF_FACESIZE-1] = L'\0';

    if (!SendRDSMessage(&msg, str, data_size))
    {
        ERR("Failed to send RDS_MSG_TEXT_OUT\n");
        return FALSE;
    }

    TRACE("RDS_ExtTextOut: Sent to surface %lu\n", g_primary_surface_id);

    return GET_NEXT_PHYSDEV(dev, pExtTextOut)->funcs->pExtTextOut(
        GET_NEXT_PHYSDEV(dev, pExtTextOut), x, y, flags, rect, str, count, dx);
}

