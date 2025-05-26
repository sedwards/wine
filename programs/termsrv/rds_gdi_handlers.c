#include "rds_gdi_handlers.h"
#include <stdio.h> // For printf
#include "../../dlls/winerds.drv/rds.h" // For RDS_SURFACE, find_surface, gdi_move_to, gdi_line_to

// If WINE_DEFAULT_DEBUG_CHANNEL is used for termsrv, define it.
// WINE_DEFAULT_DEBUG_CHANNEL(termsrv_gdi); 

void Handle_RDS_MSG_MOVE_TO(const RDS_MESSAGE *msg)
{
    RDS_SURFACE *surface;
    if (!msg) return;

    printf("Handling RDS_MSG_MOVE_TO: surfaceId=%lu, x=%lu, y=%lu\n", 
           (unsigned long)msg->params.moveTo.surfaceId, 
           (unsigned long)msg->params.moveTo.x, 
           (unsigned long)msg->params.moveTo.y);
    
    surface = find_surface(msg->params.moveTo.surfaceId);
    if (surface)
    {
        gdi_move_to(surface, msg->params.moveTo.x, msg->params.moveTo.y);
    }
    else
    {
        printf("ERR: Handle_RDS_MSG_MOVE_TO: Surface %lu not found.\n", (unsigned long)msg->params.moveTo.surfaceId);
    }
}

void Handle_RDS_MSG_LINE_TO(const RDS_MESSAGE *msg)
{
    RDS_SURFACE *surface;
    if (!msg) return;

    printf("Handling RDS_MSG_LINE_TO: surfaceId=%lu, x=%lu, y=%lu, color=%lx\n",
           (unsigned long)msg->params.lineTo.surfaceId,
           (unsigned long)msg->params.lineTo.x,
           (unsigned long)msg->params.lineTo.y,
           (unsigned long)msg->params.lineTo.color);

    surface = find_surface(msg->params.lineTo.surfaceId);
    if (surface)
    {
        gdi_line_to(surface, msg->params.lineTo.x, msg->params.lineTo.y, msg->params.lineTo.color);
    }
    else
    {
        printf("ERR: Handle_RDS_MSG_LINE_TO: Surface %lu not found.\n", (unsigned long)msg->params.lineTo.surfaceId);
    }
}

void Handle_RDS_MSG_RECTANGLE(const RDS_MESSAGE *msg)
{
    RDS_SURFACE *surface;
    if (!msg) return;

    // WINE_TRACE or printf
    printf("Handling RDS_MSG_RECTANGLE: surfaceId=%lu, LTRB=(%ld,%ld,%ld,%ld), color=%lx, filled=%d\n",
           (unsigned long)msg->params.rectangle.surfaceId,
           (long)msg->params.rectangle.left, (long)msg->params.rectangle.top,
           (long)msg->params.rectangle.right, (long)msg->params.rectangle.bottom,
           (unsigned long)msg->params.rectangle.color, msg->params.rectangle.filled);
    
    surface = find_surface(msg->params.rectangle.surfaceId);
    if (surface)
    {
        gdi_rectangle(surface, msg->params.rectangle.left, msg->params.rectangle.top,
                      msg->params.rectangle.right, msg->params.rectangle.bottom,
                      msg->params.rectangle.color, msg->params.rectangle.filled);
    }
    else
    {
        printf("ERR: Handle_RDS_MSG_RECTANGLE: Surface %lu not found.\n", (unsigned long)msg->params.rectangle.surfaceId);
    }
}

void Handle_RDS_MSG_TEXT_OUT(const RDS_MESSAGE *msg, const WCHAR *text_data)
{
    RDS_SURFACE *surface;
    if (!msg || (msg->params.textOut.count > 0 && !text_data) ) return;

    // WINE_TRACE or printf
    // Avoid printing WCHAR directly with %S in printf if it causes issues with non-null-terminated buffers or unicode.
    printf("Handling RDS_MSG_TEXT_OUT: surfaceId=%lu, x=%lu, y=%lu, color=%lx, count=%lu, data_size=%lu, text='(data)'\n",
           (unsigned long)msg->params.textOut.surfaceId,
           (unsigned long)msg->params.textOut.x, (unsigned long)msg->params.textOut.y,
           (unsigned long)msg->params.textOut.color, (unsigned long)msg->params.textOut.count,
           (unsigned long)msg->params.textOut.data_size);
    
    surface = find_surface(msg->params.textOut.surfaceId);
    if (surface)
    {
        // Pass text_data which contains the actual string
        gdi_text_out(surface, msg->params.textOut.x, msg->params.textOut.y,
                     text_data, msg->params.textOut.count, msg->params.textOut.color);
    }
    else
    {
        printf("ERR: Handle_RDS_MSG_TEXT_OUT: Surface %lu not found.\n", (unsigned long)msg->params.textOut.surfaceId);
    }
}
