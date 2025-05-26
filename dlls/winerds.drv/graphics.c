#include <windows.h>

#include "wine/debug.h" // For WINE_TRACE, WINE_ERR, WINE_WARN

#include "rds.h"          // Should include rds_message.h, PHYSDEV definition
#include "pipe_client.h"  // For SendRDSMessage

WINE_DEFAULT_DEBUG_CHANNEL(rds);

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
    COLORREF color = 0; // Default to black

    WINE_TRACE("dev=%p, x=%d, y=%d\n", dev, x, y);

    // Attempt to get current DC color if dev and dev->hdc are valid
    // This relies on PHYSDEV having an hdc member that is valid.
    // Also assumes that PHYSDEV is a pointer type, which is typical.
    if (dev && dev->hdc) { // dev is typically DRIVER_PDEVICE dev
        color = GetTextColor(dev->hdc); // Or GetDCPenColor, depending on what's appropriate for lines
    } else {
        WINE_WARN("dev (%p) or dev->hdc is NULL, using default color for LineTo.\n", dev);
    }

    msg.msgType = RDS_MSG_LINE_TO;
    // TODO: Replace placeholder '1' with actual surface ID logic later
    // The actual surface ID should be derived from PHYSDEV dev
    msg.params.lineTo.surfaceId = 1; // Placeholder Surface ID
    msg.params.lineTo.x = x;
    msg.params.lineTo.y = y;
    msg.params.lineTo.color = color;

    if (!SendRDSMessage(&msg, NULL, 0))
    {
        WINE_ERR("Failed to send RDS_MSG_LINE_TO\n");
        return FALSE; // Indicate failure
    }
    return TRUE; // Indicate success
}

// Other GDI functions like rds_Rectangle, rds_TextOut would follow a similar pattern.

BOOL rds_Rectangle(PHYSDEV dev, INT left, INT top, INT right, INT bottom)
{
    RDS_MESSAGE msg;
    COLORREF pen_color = 0; // Default to black
    BOOL is_filled = FALSE; // Default to not filled

    WINE_TRACE("dev=%p, left=%d, top=%d, right=%d, bottom=%d\n", dev, left, top, right, bottom);

    if (dev && dev->hdc) {
        pen_color = GetDCPenColor(dev->hdc); // Or GetTextColor if pen color not directly available this way
        
        // Check if brush is not NULL to determine if filled
        HBRUSH hBrush = GetCurrentObject(dev->hdc, OBJ_BRUSH);
        if (hBrush != NULL && hBrush != GetStockObject(NULL_BRUSH) && hBrush != GetStockObject(HOLLOW_BRUSH)) {
             // A more complex check might be needed for pattern brushes etc.
             // For now, any non-null/hollow brush implies filled for simplicity.
             // The actual fill color would be from the brush, but RDS_MSG_RECTANGLE only has one 'color' field.
             // We'll send the pen_color for now and assume server handles fill with a default or this color.
             is_filled = TRUE; 
        }
    } else {
        WINE_WARN("dev or dev->hdc is NULL, using default color/fill for Rectangle.\n");
    }

    msg.msgType = RDS_MSG_RECTANGLE;
    msg.params.rectangle.surfaceId = 1; // Placeholder Surface ID
    msg.params.rectangle.left = left;
    msg.params.rectangle.top = top;
    msg.params.rectangle.right = right;
    msg.params.rectangle.bottom = bottom;
    msg.params.rectangle.color = pen_color; // Sending pen color
    msg.params.rectangle.filled = is_filled;

    if (!SendRDSMessage(&msg, NULL, 0))
    {
        WINE_ERR("Failed to send RDS_MSG_RECTANGLE\n");
        return FALSE;
    }
    return TRUE;
}

BOOL rds_TextOut(PHYSDEV dev, INT x, INT y, LPCWSTR str, INT count)
{
    RDS_MESSAGE msg;
    COLORREF color = 0; // Default to black
    DWORD data_size = 0;

    WINE_TRACE("dev=%p, x=%d, y=%d, str=%p, count=%d\n", dev, x, y, str, count);

    if (count <= 0 || !str) return TRUE; // Nothing to draw

    if (dev && dev->hdc) {
        color = GetTextColor(dev->hdc);
    } else {
        WINE_WARN("dev or dev->hdc is NULL, using default color for TextOut.\n");
    }

    data_size = count * sizeof(WCHAR);

    msg.msgType = RDS_MSG_TEXT_OUT;
    msg.params.textOut.surfaceId = 1; // Placeholder Surface ID
    msg.params.textOut.x = x;
    msg.params.textOut.y = y;
    msg.params.textOut.color = color;
    msg.params.textOut.count = count;
    msg.params.textOut.data_size = data_size;
    // The actual string 'str' will be passed as 'variable_data' to SendRDSMessage

    if (!SendRDSMessage(&msg, str, data_size))
    {
        WINE_ERR("Failed to send RDS_MSG_TEXT_OUT\n");
        return FALSE;
    }
    return TRUE;
}
