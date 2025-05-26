#include "../../dlls/winerds.drv/rds.h" // Or "rds.h" if copied locally. Adjust path as needed.
#include <stdio.h> // For printf or WINE_TRACE/ERR if configured
#include <string.h> // For memset

// Assuming rds_service and its default_surface are accessible, e.g., via an extern or getter.
// For this subtask, we might need a simplified way if rds_service is not directly global.
// Let's assume for now that rds_service.default_surface can be accessed if needed,
// or that find_surface will be smarter later.

// Simplified find_surface for now
// A proper implementation would look up in a list of active surfaces.
// This requires rds_service.default_surface to be linkable or passed in.
// For now, to make this self-contained for the worker, we'll imagine a global
// default_surface for placeholder ID 1.
// In reality, this function would be more complex.
extern RDS_SERVICE rds_service; // Assume this is defined in rds.c and linked.

RDS_SURFACE *find_surface(DWORD surface_id)
{
    // WINE_TRACE("Looking for surface ID: %lu\n", (unsigned long)surface_id);
    printf("Looking for surface ID: %lu\n", (unsigned long)surface_id);
    // Placeholder logic: if surface_id is 1, return the default_surface.
    // This relies on rds_service.default_surface being initialized and accessible.
    if (surface_id == 1 && rds_service.default_surface) {
        return rds_service.default_surface;
    }
    // WINE_ERR("Surface ID %lu not found.\n", (unsigned long)surface_id);
    printf("ERR: Surface ID %lu not found.\n", (unsigned long)surface_id);
    return NULL; 
}

// Implementation for gdi_move_to
BOOL gdi_move_to(RDS_SURFACE *surface, int x, int y)
{
    if (!surface) 
    {
        // WINE_ERR("gdi_move_to: surface is NULL\n");
        printf("ERR: gdi_move_to: surface is NULL\n");
        return FALSE;
    }
    // WINE_TRACE("gdi_move_to: surfaceId=%lu, x=%d, y=%d\n", (unsigned long)surface->id, x, y);
    printf("gdi_move_to: surfaceId=%lu, x=%d, y=%d\n", (unsigned long)surface->id, x, y);
    surface->current_x = x;
    surface->current_y = y;
    return TRUE;
}

// Basic gdi_line_to: only horizontal or vertical lines, 32bpp
BOOL gdi_line_to(RDS_SURFACE *surface, int x2, int y2, COLORREF color)
{
    int x1, y1;
    unsigned char *pixel_ptr;
    int i;

    if (!surface)
    {
        // WINE_ERR("gdi_line_to: surface is NULL\n");
        printf("ERR: gdi_line_to: surface is NULL\n");
        return FALSE;
    }
    if (!surface->data)
    {
        // WINE_ERR("gdi_line_to: surface->data is NULL\n");
        printf("ERR: gdi_line_to: surface->data is NULL\n");
        return FALSE;
    }
    
    x1 = surface->current_x;
    y1 = surface->current_y;

    // WINE_TRACE("gdi_line_to: surfaceId=%lu from (%d,%d) to (%d,%d), color=%lx\n", 
    //            (unsigned long)surface->id, x1, y1, x2, y2, (unsigned long)color);
    printf("gdi_line_to: surfaceId=%lu from (%d,%d) to (%d,%d), color=%lx\n", 
           (unsigned long)surface->id, x1, y1, x2, y2, (unsigned long)color);

    // Assuming 32bpp (4 bytes per pixel: B, G, R, A)
    // And origin (0,0) is top-left.
    // No clipping implemented for this basic version.

    if (y1 == y2) // Horizontal line
    {
        if (y1 < 0 || y1 >= surface->height) return TRUE; // Clipped
        int start_x = (x1 < x2) ? x1 : x2;
        int end_x = (x1 < x2) ? x2 : x1;
        if (start_x >= surface->width || end_x < 0) return TRUE; // Clipped
        
        start_x = (start_x < 0) ? 0 : start_x;
        end_x = (end_x >= surface->width) ? surface->width - 1 : end_x;

        pixel_ptr = (unsigned char *)surface->data + (y1 * surface->stride) + (start_x * 4);
        for (i = start_x; i <= end_x; i++)
        {
            pixel_ptr[0] = GetBValue(color); // Blue
            pixel_ptr[1] = GetGValue(color); // Green
            pixel_ptr[2] = GetRValue(color); // Red
            pixel_ptr[3] = 0xFF;             // Alpha (opaque)
            pixel_ptr += 4;
        }
    }
    else if (x1 == x2) // Vertical line
    {
        if (x1 < 0 || x1 >= surface->width) return TRUE; // Clipped
        int start_y = (y1 < y2) ? y1 : y2;
        int end_y = (y1 < y2) ? y2 : y1;
        if (start_y >= surface->height || end_y < 0) return TRUE; // Clipped

        start_y = (start_y < 0) ? 0 : start_y;
        end_y = (end_y >= surface->height) ? surface->height - 1 : end_y;

        pixel_ptr = (unsigned char *)surface->data + (start_y * surface->stride) + (x1 * 4);
        for (i = start_y; i <= end_y; i++)
        {
            pixel_ptr[0] = GetBValue(color);
            pixel_ptr[1] = GetGValue(color);
            pixel_ptr[2] = GetRValue(color);
            pixel_ptr[3] = 0xFF;
            pixel_ptr += surface->stride;
        }
    }
    else
    {
        // WINE_WARN("gdi_line_to: Diagonal lines not implemented in this basic version.\n");
        printf("WARN: gdi_line_to: Diagonal lines not implemented in this basic version.\n");
        // For now, just update current position for diagonal lines
    }

    surface->current_x = x2;
    surface->current_y = y2;
    surface->dirty = TRUE; 
    // TODO: Update surface->dirty_rect more precisely
    if (surface->dirty_rect.left > x1) surface->dirty_rect.left = x1;
    if (surface->dirty_rect.right < x2) surface->dirty_rect.right = x2; // Simplistic
    if (surface->dirty_rect.top > y1) surface->dirty_rect.top = y1;
    if (surface->dirty_rect.bottom < y2) surface->dirty_rect.bottom = y2;


    return TRUE;
}

BOOL gdi_rectangle(RDS_SURFACE *surface, int left, int top, int right, int bottom, COLORREF color, BOOL filled)
{
    int r, c; // row, col
    unsigned char *pixel_ptr_row_start;

    if (!surface || !surface->data) 
    {
        printf("ERR: gdi_rectangle: null surface or data\n");
        return FALSE;
    }

    printf("gdi_rectangle: surfaceId=%lu, LTRB=(%d,%d)-(%d,%d), color=%lx, filled=%d\n",
           (unsigned long)surface->id, left, top, right, bottom, (unsigned long)color, filled);

    // Basic clipping
    int r_left = (left < 0) ? 0 : left;
    int r_top = (top < 0) ? 0 : top;
    // Ensure right/bottom are not less than left/top after clipping
    int r_right = (right > surface->width) ? surface->width : right;
    r_right = (r_right < r_left) ? r_left : r_right;
    int r_bottom = (bottom > surface->height) ? surface->height : bottom;
    r_bottom = (r_bottom < r_top) ? r_top : r_bottom;


    if (filled)
    {
        for (r = r_top; r < r_bottom; ++r)
        {
            pixel_ptr_row_start = (unsigned char *)surface->data + (r * surface->stride) + (r_left * 4);
            for (c = r_left; c < r_right; ++c)
            {
                pixel_ptr_row_start[0] = GetBValue(color);
                pixel_ptr_row_start[1] = GetGValue(color);
                pixel_ptr_row_start[2] = GetRValue(color);
                pixel_ptr_row_start[3] = 0xFF; // Alpha
                pixel_ptr_row_start += 4;
            }
        }
    }
    else // Unfilled - draw 4 lines
    {
        // Store current pen position
        int orig_x = surface->current_x;
        int orig_y = surface->current_y;

        // Draw using existing gdi_move_to and gdi_line_to.
        // Note: The provided gdi_line_to only draws horizontal/vertical lines.
        // To draw a proper unfilled rectangle, gdi_line_to would need to support this.
        // The current implementation of gdi_line_to might not produce sharp corners
        // or may miss lines if they are not perfectly horizontal/vertical.
        // For this exercise, we assume it's sufficient or the basic gdi_line_to is used.
        
        // Top line: (left, top) to (right-1, top)
        gdi_move_to(surface, left, top);
        gdi_line_to(surface, right - 1, top, color);
        
        // Right line: (right-1, top) to (right-1, bottom-1)
        // gdi_move_to(surface, right - 1, top); // gdi_line_to updates current_x/y
        gdi_line_to(surface, right - 1, bottom - 1, color);
        
        // Bottom line: (right-1, bottom-1) to (left, bottom-1)
        // gdi_move_to(surface, right - 1, bottom - 1);
        gdi_line_to(surface, left, bottom - 1, color);
        
        // Left line: (left, bottom-1) to (left, top)
        // gdi_move_to(surface, left, bottom - 1);
        gdi_line_to(surface, left, top, color);

        // Restore pen position
        gdi_move_to(surface, orig_x, orig_y);
    }
    
    surface->dirty = TRUE;
    // Update dirty_rect accurately. For simplicity, using the provided rect.
    // More precise would be to union existing dirty_rect with this one.
    if (surface->dirty_rect.left > r_left) surface->dirty_rect.left = r_left;
    if (surface->dirty_rect.top > r_top) surface->dirty_rect.top = r_top;
    if (surface->dirty_rect.right < r_right) surface->dirty_rect.right = r_right;
    if (surface->dirty_rect.bottom < r_bottom) surface->dirty_rect.bottom = r_bottom;

    return TRUE;
}

BOOL gdi_text_out(RDS_SURFACE *surface, int x, int y, const WCHAR *text, int count, COLORREF color)
{
    if (!surface) return FALSE;
    // WINE_TRACE or printf
    printf("gdi_text_out: surfaceId=%lu, at (%d,%d), count=%d, color=%lx. Text data starts with: %lc (if count>0)\n",
           (unsigned long)surface->id, x, y, count, (unsigned long)color, (count > 0 && text) ? text[0] : L' ');

    // Placeholder: Draw a small 5x5 black rectangle at x,y
    if (surface->data) {
        COLORREF placeholder_color = RGB(10,10,10); // Dark gray placeholder
        int placeholder_size = 5;
        // Store current pen pos & color
        int orig_x = surface->current_x;
        int orig_y = surface->current_y;
        // COLORREF orig_pen_color = GetDCPenColor(dev->hdc); // Cannot get this here easily

        // Save current color, set to placeholder_color, then restore.
        // This is tricky because gdi_line_to takes color as a parameter.
        // The current gdi_line_to doesn't use a "current pen color" from the surface struct.
        
        // Top line
        gdi_move_to(surface, x, y);
        gdi_line_to(surface, x + placeholder_size - 1, y, placeholder_color);
        // Right line
        gdi_line_to(surface, x + placeholder_size - 1, y + placeholder_size - 1, placeholder_color);
        // Bottom line
        gdi_line_to(surface, x, y + placeholder_size - 1, placeholder_color);
        // Left line
        gdi_line_to(surface, x, y, placeholder_color);
        
        // Restore pen position (optional for placeholder)
        gdi_move_to(surface, orig_x, orig_y);
    }

    surface->dirty = TRUE;
    // TODO: update dirty_rect (more complex for text)
    // For placeholder, update dirty rect based on the small rectangle drawn
    if (surface->dirty_rect.left > x) surface->dirty_rect.left = x;
    if (surface->dirty_rect.top > y) surface->dirty_rect.top = y;
    if (surface->dirty_rect.right < x + 5) surface->dirty_rect.right = x + 5;
    if (surface->dirty_rect.bottom < y + 5) surface->dirty_rect.bottom = y + 5;
    return TRUE;
}

// Placeholder for gdi_draw_test_pattern - already called in rds.c, ensure it's implemented.
// This is a good place for it.
void gdi_draw_test_pattern(RDS_SURFACE *surface)
{
    if (!surface || !surface->data) return;
    // WINE_TRACE("gdi_draw_test_pattern: Drawing test pattern on surface %lu\n", (unsigned long)surface->id);
    printf("gdi_draw_test_pattern: Drawing test pattern on surface %lu\n", (unsigned long)surface->id);
    
    // Example: Fill with a color, then draw lines
    COLORREF bgColor = RGB(200, 200, 240); // Light blue-ish
    COLORREF lineColor1 = RGB(255, 0, 0);   // Red
    COLORREF lineColor2 = RGB(0, 255, 0);   // Green

    // Fill background (assuming 32bpp)
    for (int r = 0; r < surface->height; ++r) {
        unsigned char *row = (unsigned char*)surface->data + r * surface->stride;
        for (int c = 0; c < surface->width; ++c) {
            row[c*4 + 0] = GetBValue(bgColor);
            row[c*4 + 1] = GetGValue(bgColor);
            row[c*4 + 2] = GetRValue(bgColor);
            row[c*4 + 3] = 0xFF;
        }
    }
    
    // Draw some lines
    gdi_move_to(surface, 0, 0);
    gdi_line_to(surface, surface->width -1, surface->height -1, lineColor1);
    gdi_move_to(surface, surface->width -1, 0);
    gdi_line_to(surface, 0, surface->height -1, lineColor2);

    surface->dirty = TRUE;
    surface->dirty_rect.left = 0;
    surface->dirty_rect.top = 0;
    surface->dirty_rect.right = surface->width;
    surface->dirty_rect.bottom = surface->height;
}
