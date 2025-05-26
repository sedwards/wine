#include "rds_gdi_handlers.h"
#include <stdio.h> // For printf
#include <windows.h> // For Win32 GDI functions like MoveToEx, LineTo, Rectangle, TextOutW
#include "../../dlls/winerds.drv/rds.h" // For RDS_SURFACE, find_surface

// If WINE_DEFAULT_DEBUG_CHANNEL is used for termsrv, define it.
// WINE_DEFAULT_DEBUG_CHANNEL(termsrv_gdi); 

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
        HPEN hNewPen = CreatePen(PS_SOLID, 1, msg->params.lineTo.color);
        if (hNewPen)
        {
            HPEN hOldPen = (HPEN)SelectObject(surface->hdc, hNewPen);
            LineTo(surface->hdc, msg->params.lineTo.x, msg->params.lineTo.y);
            SelectObject(surface->hdc, hOldPen);
            DeleteObject(hNewPen);
        }
        else // Fallback if pen creation failed
        {
            printf("WARN: CreatePen failed for LineTo, using HDC current pen.\n");
            LineTo(surface->hdc, msg->params.lineTo.x, msg->params.lineTo.y);
        }
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
        rc.right = msg->params.rectangle.right;
        rc.bottom = msg->params.rectangle.bottom;

        HPEN hNewPen = CreatePen(PS_SOLID, 1, msg->params.rectangle.color);
        HBRUSH hNewBrush;

        if (msg->params.rectangle.filled) {
            hNewBrush = CreateSolidBrush(msg->params.rectangle.color);
        } else {
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
            if (msg->params.rectangle.filled && hNewBrush) // Only delete if we created it
            {
                DeleteObject(hNewBrush);
            }
        }
        else // Fallback or error
        {
            if(hNewPen) DeleteObject(hNewPen);
            if(msg->params.rectangle.filled && hNewBrush) DeleteObject(hNewBrush); // Check if created before deleting
            printf("WARN: CreatePen or CreateSolidBrush failed for Rectangle, using HDC current objects.\n");
            Rectangle(surface->hdc, rc.left, rc.top, rc.right, rc.bottom);
        }
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
        COLORREF oldColor = SetTextColor(surface->hdc, msg->params.textOut.color);
        
        TextOutW(surface->hdc, msg->params.textOut.x, msg->params.textOut.y,
                 text_data, msg->params.textOut.count);
        
        SetTextColor(surface->hdc, oldColor); // Restore old color
    }
    else
    {
        printf("ERR: Handle_RDS_MSG_TEXT_OUT: Surface %lu or its HDC not found.\n", (unsigned long)msg->params.textOut.surfaceId);
    }
}
