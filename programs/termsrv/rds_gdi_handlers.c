#include <stdio.h> // For printf
#include <windows.h> // For Win32 GDI functions like MoveToEx, LineTo, Rectangle, TextOutW
#include "rds_gdi_handlers.h"
#include "rds.h" // For RDS_SURFACE, find_surface
#include "broadway_server.h" // For Broadway framebuffer updates
#include "wine/debug.h"

/* External Broadway state */
extern BOOL broadway_enabled;
extern BROADWAY_SERVER g_broadway_server;

WINE_DEFAULT_DEBUG_CHANNEL(termsrv_gdi);

/* Helper function to update Broadway framebuffer after drawing operations */
static void update_broadway_framebuffer_if_enabled(RDS_SURFACE *surface)
{
    if (broadway_enabled && g_broadway_server.enabled && surface) {
        broadway_update_framebuffer(&g_broadway_server, surface);
    }
} 

void Handle_RDS_MSG_MOVE_TO(const RDS_MESSAGE *msg)
{
    RDS_SURFACE *surface;
    if (!msg) return;

    printf("Handling RDS_MSG_MOVE_TO (Win32): surfaceId=%lu, x=%lu, y=%lu\n", 
           (unsigned long)msg->params.moveTo.surfaceId, 
           (unsigned long)msg->params.moveTo.x, 
           (unsigned long)msg->params.moveTo.y);
    
    surface = find_surface(msg->params.moveTo.surfaceId);
    if (surface && surface->hdc)
    {
        MoveToEx(surface->hdc, msg->params.moveTo.x, msg->params.moveTo.y, NULL);
    }
    else
    {
        printf("ERR: Handle_RDS_MSG_MOVE_TO: Surface %lu or its HDC not found.\n", (unsigned long)msg->params.moveTo.surfaceId);
    }
}

void Handle_RDS_MSG_LINE_TO(const RDS_MESSAGE *msg)
{
    RDS_SURFACE *surface;
    if (!msg) return;

    printf("Handling RDS_MSG_LINE_TO (Win32 with on-the-fly Pen): surfaceId=%lu, x=%lu, y=%lu, color=%lx\n",
           (unsigned long)msg->params.lineTo.surfaceId,
           (unsigned long)msg->params.lineTo.x,
           (unsigned long)msg->params.lineTo.y,
           (unsigned long)msg->params.lineTo.color);

    surface = find_surface(msg->params.lineTo.surfaceId);
    if (surface && surface->hdc)
    {
        LOGPEN logPen;
        HPEN hNewPen;

        logPen.lopnStyle = msg->params.lineTo.lopnStyle;
        logPen.lopnWidth.x = msg->params.lineTo.lopnWidth_x;
        logPen.lopnWidth.y = 0; // According to MSDN, y is not used for pens.
        logPen.lopnColor = msg->params.lineTo.color;

        hNewPen = CreatePenIndirect(&logPen);
        if (hNewPen)
        {
            HPEN hOldPen = (HPEN)SelectObject(surface->hdc, hNewPen);
            LineTo(surface->hdc, msg->params.lineTo.x, msg->params.lineTo.y);
            SelectObject(surface->hdc, hOldPen);
            DeleteObject(hNewPen);
        }
        else
        {
            printf("WARN: CreatePenIndirect failed for LineTo (Style: %u, Width: %ld, Color: %lx). Using HDC current pen.\n",
                   logPen.lopnStyle, (long)logPen.lopnWidth.x, logPen.lopnColor);
            LineTo(surface->hdc, msg->params.lineTo.x, msg->params.lineTo.y);
        }
        
        /* Update Broadway framebuffer after drawing */
        update_broadway_framebuffer_if_enabled(surface);
    }
    else
    {
        printf("ERR: Handle_RDS_MSG_LINE_TO: Surface %lu or its HDC not found.\n", (unsigned long)msg->params.lineTo.surfaceId);
    }
}

void Handle_RDS_MSG_RECTANGLE(const RDS_MESSAGE *msg)
{
    RDS_SURFACE *surface;
    if (!msg) return;

    printf("Handling RDS_MSG_RECTANGLE (Win32 with on-the-fly Pen/Brush): surfaceId=%lu, LTRB=(%ld,%ld,%ld,%ld), color=%lx, filled=%d\n",
           (unsigned long)msg->params.rectangle.surfaceId,
           (long)msg->params.rectangle.left, (long)msg->params.rectangle.top,
           (long)msg->params.rectangle.right, (long)msg->params.rectangle.bottom,
           (unsigned long)msg->params.rectangle.color, msg->params.rectangle.filled);
         
    surface = find_surface(msg->params.rectangle.surfaceId);
    if (surface && surface->hdc)
    {
        RECT rc;
        rc.left = msg->params.rectangle.left;
        rc.top = msg->params.rectangle.top;
        LOGPEN logPen;
        HPEN hNewPen;
        LOGBRUSH logBrush;
        HBRUSH hNewBrush;
        BOOL brush_created = FALSE;
        
        rc.right = msg->params.rectangle.right;
        rc.bottom = msg->params.rectangle.bottom;

        // Setup Pen
        logPen.lopnStyle = msg->params.rectangle.pen_lopnStyle;
        logPen.lopnWidth.x = msg->params.rectangle.pen_lopnWidth_x;
        logPen.lopnWidth.y = 0;
        logPen.lopnColor = msg->params.rectangle.color; // This is pen_color
        hNewPen = CreatePenIndirect(&logPen);

        // Setup Brush
        if (msg->params.rectangle.filled)
        {
            logBrush.lbStyle = msg->params.rectangle.brush_lbStyle;
            logBrush.lbColor = msg->params.rectangle.brush_lbColor;
            logBrush.lbHatch = msg->params.rectangle.brush_lbHatch;
            hNewBrush = CreateBrushIndirect(&logBrush);
            brush_created = TRUE;
        }
        else
        {
            hNewBrush = (HBRUSH)GetStockObject(NULL_BRUSH);
        }

        if (hNewPen && hNewBrush)
        {
            HPEN hOldPen = (HPEN)SelectObject(surface->hdc, hNewPen);
            HBRUSH hOldBrush = (HBRUSH)SelectObject(surface->hdc, hNewBrush);

            Rectangle(surface->hdc, rc.left, rc.top, rc.right, rc.bottom);

            SelectObject(surface->hdc, hOldPen);
            SelectObject(surface->hdc, hOldBrush);
            
            DeleteObject(hNewPen);
            if (brush_created && hNewBrush)
            {
                DeleteObject(hNewBrush);
            }
        }
        else
        {
            if(hNewPen) DeleteObject(hNewPen);
            if(brush_created && hNewBrush) DeleteObject(hNewBrush);
            printf("WARN: CreatePenIndirect or CreateBrushIndirect failed for Rectangle. Pen(S:%u,W:%ld,C:%lx), Brush(S:%u,C:%lx,H:%llu). Using HDC current objects.\n",
                   logPen.lopnStyle, (long)logPen.lopnWidth.x, logPen.lopnColor,
                   logBrush.lbStyle, logBrush.lbColor, (unsigned long long)logBrush.lbHatch);
            Rectangle(surface->hdc, rc.left, rc.top, rc.right, rc.bottom);
        }
        
        /* Update Broadway framebuffer after drawing */
        update_broadway_framebuffer_if_enabled(surface);
    }
    else
    {
        printf("ERR: Handle_RDS_MSG_RECTANGLE: Surface %lu or its HDC not found.\n", (unsigned long)msg->params.rectangle.surfaceId);
    }
}

void Handle_RDS_MSG_TEXT_OUT(const RDS_MESSAGE *msg, const WCHAR *text_data)
{
    RDS_SURFACE *surface;
    if (!msg || (msg->params.textOut.count > 0 && !text_data)) return;

    printf("Handling RDS_MSG_TEXT_OUT (Win32 with on-the-fly TextColor): surfaceId=%lu, x=%lu, y=%lu, color=%lx, count=%lu, text='(data)'\n",
           (unsigned long)msg->params.textOut.surfaceId,
           (unsigned long)msg->params.textOut.x, (unsigned long)msg->params.textOut.y,
           (unsigned long)msg->params.textOut.color, (unsigned long)msg->params.textOut.count);
         
    surface = find_surface(msg->params.textOut.surfaceId);
    if (surface && surface->hdc)
    {
        LOGFONTW logFont;
        HFONT hNewFont, hOldFont;
        COLORREF oldTextColor, oldBkColor;
        int oldBkMode;

        // Populate LOGFONTW from message parameters
        logFont.lfHeight = msg->params.textOut.font_lfHeight;
        logFont.lfWidth = msg->params.textOut.font_lfWidth;
        logFont.lfEscapement = 0; // Default
        logFont.lfOrientation = 0; // Default
        logFont.lfWeight = msg->params.textOut.font_lfWeight;
        logFont.lfItalic = msg->params.textOut.font_lfItalic;
        logFont.lfUnderline = msg->params.textOut.font_lfUnderline;
        logFont.lfStrikeOut = msg->params.textOut.font_lfStrikeOut;
        logFont.lfCharSet = msg->params.textOut.font_lfCharSet;
        logFont.lfOutPrecision = OUT_DEFAULT_PRECIS; // Default
        logFont.lfClipPrecision = CLIP_DEFAULT_PRECIS; // Default
        logFont.lfQuality = DEFAULT_QUALITY; // Default
        logFont.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE; // Default
        lstrcpynW(logFont.lfFaceName, msg->params.textOut.font_lfFaceName, LF_FACESIZE);
        logFont.lfFaceName[LF_FACESIZE -1] = L'\0'; // Ensure null termination

        hNewFont = CreateFontIndirectW(&logFont);

        if (hNewFont)
        {
            hOldFont = (HFONT)SelectObject(surface->hdc, hNewFont);
        }
        else
        {
            printf("WARN: CreateFontIndirectW failed for TextOut. Using HDC current font.\n");
            hOldFont = NULL; // No font to restore if creation failed
        }

        // Set text properties
        oldTextColor = SetTextColor(surface->hdc, msg->params.textOut.color);
        oldBkColor = SetBkColor(surface->hdc, msg->params.textOut.text_bk_color);
        oldBkMode = SetBkMode(surface->hdc, msg->params.textOut.bk_mode);
        
        TextOutW(surface->hdc, msg->params.textOut.x, msg->params.textOut.y,
                 text_data, msg->params.textOut.count);
        
        // Restore text properties
        SetTextColor(surface->hdc, oldTextColor);
        SetBkColor(surface->hdc, oldBkColor);
        SetBkMode(surface->hdc, oldBkMode);

        if (hNewFont)
        {
            SelectObject(surface->hdc, hOldFont);
            DeleteObject(hNewFont);
        }
        
        /* Update Broadway framebuffer after drawing */
        update_broadway_framebuffer_if_enabled(surface);
    }
    else
    {
        printf("ERR: Handle_RDS_MSG_TEXT_OUT: Surface %lu or its HDC not found.\n", (unsigned long)msg->params.textOut.surfaceId);
    }
}

void Handle_RDS_MSG_PING(const RDS_MESSAGE *msg, HANDLE hPipe)
{
    RDS_MESSAGE pong_msg;
    DWORD cbWritten = 0;
    static DWORD ping_count = 0;
    DWORD timestamp = GetTickCount();
    
    ping_count++;
    printf("[PING #%lu] Received RDS_MSG_PING at %lu ms\n", (unsigned long)ping_count, (unsigned long)timestamp);
    
    ZeroMemory(&pong_msg, sizeof(RDS_MESSAGE));
    pong_msg.msgType = RDS_MSG_PONG;

    if (WriteFile(hPipe, &pong_msg, sizeof(RDS_MESSAGE), &cbWritten, NULL) && cbWritten == sizeof(RDS_MESSAGE)) 
    { 
        printf("[PING #%lu] Sent RDS_MSG_PONG successfully at %lu ms (response time: %lu ms)\n", 
               (unsigned long)ping_count, (unsigned long)GetTickCount(), 
               (unsigned long)(GetTickCount() - timestamp)); 
    } 
    else 
    { 
        printf("[PING #%lu] ERR - WriteFile failed for RDS_MSG_PONG. GLE=%lu\n", 
               (unsigned long)ping_count, (unsigned long)GetLastError()); 
    }
}


