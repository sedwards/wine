

#if 0
#pragma makedep unix
#endif

#include "config.h"

#include <stdarg.h>
#include <math.h>
#include <float.h>
#include <stdlib.h>
#ifndef PI
#define PI M_PI
#endif
#include <string.h>
#include <limits.h>

#include "windef.h"
#include "winbase.h"
#include "winnt.h"
#include "wingdi.h"
#include "winreg.h"

#include "rdsdrv.h"
#include "wine/debug.h"


//#include <windows.h>

#include "rds.h"          // Should include rds_message.h, PHYSDEV definition
#include "pipe_client.h"  // For SendRDSMessage

#include "wine/gdi_driver.h"

WINE_DEFAULT_DEBUG_CHANNEL(winerds);

/*
 * PHYSDEV is an opaque structure used by Wine's GDI internals.
 * We assume it has a member 'hdc' (HDC hdc;) based on the provided
 * rds_LineTo code trying to access dev->hdc.
 * If the actual PHYSDEV definition in Wine's headers (e.g., gdi_driver.h or similar)
 * is different, the color retrieval in rds_LineTo might need adjustment.
 * For now, we proceed with the assumption that dev->hdc is valid.
 */

// No old extern declarations like:
// extern BOOL rds_move_to(PHYSDEV dev, INT x, INT y);
// extern BOOL rds_line_to(PHYSDEV dev, INT x, INT y, COLORREF color);
// ... etc. These are being replaced by the functions below.

BOOL rds_MoveTo(PHYSDEV dev, INT x, INT y)
{
    RDS_MESSAGE msg;

    WINE_TRACE("dev=%p, x=%d, y=%d\n", dev, x, y);

    msg.msgType = RDS_MSG_MOVE_TO;
    // TODO: Replace placeholder '1' with actual surface ID logic later
    // The actual surface ID should be derived from PHYSDEV dev
    msg.params.moveTo.surfaceId = 1; // Placeholder Surface ID
    msg.params.moveTo.x = x;
    msg.params.moveTo.y = y;

    if (!SendRDSMessage(&msg, NULL, 0))
    {
        WINE_ERR("Failed to send RDS_MSG_MOVE_TO\n");
        return FALSE; // Indicate failure
    }
    return TRUE; // Indicate success
}

BOOL rds_LineTo(PHYSDEV dev, INT x, INT y)
{
    RDS_MESSAGE msg;
    LOGPEN logPen; // Use LOGPEN for Unicode compatibility if lfFaceName uses WCHAR

    // WINE_TRACE("dev=%p, x=%d, y=%d\n", dev, x, y); // Already present

    if (dev && dev->hdc) {
        HPEN hCurrentPen = GetCurrentObject(dev->hdc, OBJ_PEN);
        if (hCurrentPen) {
            GetObjectW(hCurrentPen, sizeof(LOGPEN), &logPen);
        } else {
            // Default LOGPEN if no pen or GetCurrentObject fails
            memset(&logPen, 0, sizeof(LOGPEN));
            logPen.lopnStyle = PS_SOLID;
            logPen.lopnWidth.x = 1;
            logPen.lopnColor = RGB(0,0,0); // Default black
            // WINE_WARN("Could not get current pen, using default for LineTo.\n");
            printf("WARN: winerds.drv: Could not get current pen, using default for LineTo.\n");
        }
    } else {
        memset(&logPen, 0, sizeof(LOGPEN));
        logPen.lopnStyle = PS_SOLID;
        logPen.lopnWidth.x = 1;
        logPen.lopnColor = RGB(0,0,0);
        // WINE_WARN("dev or dev->hdc is NULL, using default pen for LineTo.\n");
        printf("WARN: winerds.drv: dev or dev->hdc is NULL, using default pen for LineTo.\n");
    }

    msg.msgType = RDS_MSG_LINE_TO;
    msg.params.lineTo.surfaceId = 1; // Placeholder Surface ID
    msg.params.lineTo.x = x;
    msg.params.lineTo.y = y;
    msg.params.lineTo.color = logPen.lopnColor; // This is pen_lopnColor
    msg.params.lineTo.lopnStyle = logPen.lopnStyle;
    msg.params.lineTo.lopnWidth_x = logPen.lopnWidth.x;

    if (!SendRDSMessage(&msg, NULL, 0))
    {
        // WINE_ERR("Failed to send RDS_MSG_LINE_TO\n");
        printf("ERR: winerds.drv: Failed to send RDS_MSG_LINE_TO\n");
        return FALSE;
    }
    return TRUE;
}

// Other GDI functions like rds_Rectangle, rds_TextOut would follow a similar pattern.

BOOL rds_Rectangle(PHYSDEV dev, INT left, INT top, INT right, INT bottom)
{
    RDS_MESSAGE msg;
    LOGPEN logPen;
    LOGBRUSH logBrush;
    BOOL is_filled = FALSE;

    // WINE_TRACE("dev=%p, left=%d, top=%d, right=%d, bottom=%d\n", dev, left, top, right, bottom); // Already present

    if (dev && dev->hdc) {
        HPEN hCurrentPen = GetCurrentObject(dev->hdc, OBJ_PEN);
        HBRUSH hCurrentBrush = GetCurrentObject(dev->hdc, OBJ_BRUSH);

        if (hCurrentPen) {
            GetObjectW(hCurrentPen, sizeof(LOGPEN), &logPen);
        } else {
            memset(&logPen, 0, sizeof(LOGPEN));
            logPen.lopnStyle = PS_SOLID; logPen.lopnWidth.x = 1; logPen.lopnColor = RGB(0,0,0);
            printf("WARN: winerds.drv: Could not get current pen, using default for Rectangle.\n");
        }

        if (hCurrentBrush) {
            GetObjectW(hCurrentBrush, sizeof(LOGBRUSH), &logBrush);
            if (logBrush.lbStyle != BS_NULL && logBrush.lbStyle != BS_HOLLOW) { // BS_NULL is from GetStockObject(NULL_BRUSH)
                is_filled = TRUE;
            }
        } else {
            memset(&logBrush, 0, sizeof(LOGBRUSH));
            logBrush.lbStyle = BS_NULL; // No fill
            printf("WARN: winerds.drv: Could not get current brush, assuming no fill for Rectangle.\n");
        }
    } else {
        memset(&logPen, 0, sizeof(LOGPEN));
        logPen.lopnStyle = PS_SOLID; logPen.lopnWidth.x = 1; logPen.lopnColor = RGB(0,0,0);
        memset(&logBrush, 0, sizeof(LOGBRUSH));
        logBrush.lbStyle = BS_NULL;
        printf("WARN: winerds.drv: dev or dev->hdc is NULL, using default pen/brush for Rectangle.\n");
    }

    msg.msgType = RDS_MSG_RECTANGLE;
    msg.params.rectangle.surfaceId = 1; // Placeholder
    msg.params.rectangle.left = left;
    msg.params.rectangle.top = top;
    msg.params.rectangle.right = right;
    msg.params.rectangle.bottom = bottom;
    
    msg.params.rectangle.color = logPen.lopnColor; // Pen color
    msg.params.rectangle.filled = is_filled;
    msg.params.rectangle.pen_lopnStyle = logPen.lopnStyle;
    msg.params.rectangle.pen_lopnWidth_x = logPen.lopnWidth.x;
    
    msg.params.rectangle.brush_lbStyle = logBrush.lbStyle;
    msg.params.rectangle.brush_lbColor = logBrush.lbColor;
    msg.params.rectangle.brush_lbHatch = logBrush.lbHatch;

    if (!SendRDSMessage(&msg, NULL, 0))
    {
        printf("ERR: winerds.drv: Failed to send RDS_MSG_RECTANGLE\n");
        return FALSE;
    }
    return TRUE;
}

BOOL rds_TextOut(PHYSDEV dev, INT x, INT y, LPCWSTR str, INT count)
{
    RDS_MESSAGE msg;
    LOGFONTW logFont;
    COLORREF text_fg_color = RGB(0,0,0);
    COLORREF text_bk_color = RGB(255,255,255);
    INT bk_mode = OPAQUE;
    DWORD data_size = 0;

    // WINE_TRACE("dev=%p, x=%d, y=%d, str=%p, count=%d\n", dev, x, y, str, count); // Already present

    if (count <= 0 || !str) return TRUE;

    if (dev && dev->hdc) {
        HFONT hCurrentFont = GetCurrentObject(dev->hdc, OBJ_FONT);
        if (hCurrentFont) {
            GetObjectW(hCurrentFont, sizeof(LOGFONTW), &logFont);
        } else {
            memset(&logFont, 0, sizeof(LOGFONTW));
            // Initialize with some system default perhaps, or leave empty if server handles defaults
            lstrcpynW(logFont.lfFaceName, (LPCWSTR)L"System", LF_FACESIZE); 
            logFont.lfHeight = -12; // Default size
            printf("WARN: winerds.drv: Could not get current font, using default for TextOut.\n");
        }
        text_fg_color = GetTextColor(dev->hdc);
        text_bk_color = GetBkColor(dev->hdc);
        bk_mode = GetBkMode(dev->hdc);
    } else {
        memset(&logFont, 0, sizeof(LOGFONTW));
        lstrcpynW(logFont.lfFaceName, (LPCWSTR)L"System", LF_FACESIZE);
        logFont.lfHeight = -12;
        printf("WARN: winerds.drv: dev or dev->hdc is NULL, using defaults for TextOut.\n");
    }

    data_size = count * sizeof(WCHAR);

    msg.msgType = RDS_MSG_TEXT_OUT;
    msg.params.textOut.surfaceId = 1; // Placeholder
    msg.params.textOut.x = x;
    msg.params.textOut.y = y;
    
    msg.params.textOut.color = text_fg_color; // This is text_fg_color
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
    msg.params.textOut.font_lfFaceName[LF_FACESIZE-1] = L'\0'; // Ensure null termination


    if (!SendRDSMessage(&msg, str, data_size))
    {
        printf("ERR: winerds.drv: Failed to send RDS_MSG_TEXT_OUT\n");
        return FALSE;
    }
    return TRUE;
}

