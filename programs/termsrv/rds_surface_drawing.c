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

// Implementation for gdi_move_to (OBSOLETE - Handlers use MoveToEx directly)
BOOL gdi_move_to(RDS_SURFACE *surface, int x, int y)
{
    /* This function is now largely obsolete if Handle_RDS_MSG_MOVE_TO directly calls MoveToEx.
       The HDC itself maintains the current pen position.
       If kept, it should perhaps call MoveToEx(surface->hdc, x, y, NULL);
       For now, let's make it a no-op or remove its call from handler.
       The handler now calls MoveToEx directly.
    */
    // printf("OBSOLETE: gdi_move_to called for surfaceId=%lu. Should use HDC directly.\n", (unsigned long)surface->id);
    return TRUE; 
}

// Basic gdi_line_to (OBSOLETE - Handlers use LineTo directly)
BOOL gdi_line_to(RDS_SURFACE *surface, int x2, int y2, COLORREF color)
{
    /* This function is now obsolete. Handle_RDS_MSG_LINE_TO calls LineTo directly.
       The HDC's current pen color is used.
    */
    // printf("OBSOLETE: gdi_line_to called for surfaceId=%lu. Should use HDC directly.\n", (unsigned long)surface->id);
    return TRUE;
}

// Rectangle drawing (OBSOLETE - Handlers use Rectangle directly)
BOOL gdi_rectangle(RDS_SURFACE *surface, int left, int top, int right, int bottom, COLORREF color, BOOL filled)
{
    /* This function is now obsolete. Handle_RDS_MSG_RECTANGLE calls Rectangle directly.
       The HDC's current pen and brush are used.
    */
    // printf("OBSOLETE: gdi_rectangle called for surfaceId=%lu. Should use HDC directly.\n", (unsigned long)surface->id);
    return TRUE;
}

// Text drawing (OBSOLETE - Handlers use TextOutW directly)
BOOL gdi_text_out(RDS_SURFACE *surface, int x, int y, const WCHAR *text, int count, COLORREF color)
{
    /* This function is now obsolete. Handle_RDS_MSG_TEXT_OUT calls TextOutW directly.
       The HDC's current text color is used.
    */
    // printf("OBSOLETE: gdi_text_out called for surfaceId=%lu. Should use HDC directly.\n", (unsigned long)surface->id);
    return TRUE;
}

// Placeholder for gdi_draw_test_pattern - already called in rds.c, ensure it's implemented.
// This is a good place for it.
void gdi_draw_test_pattern(RDS_SURFACE *surface)
{
    if (!surface || !surface->hdc) {
        printf("ERR: gdi_draw_test_pattern: null surface or hdc\n");
        return;
    }
    printf("gdi_draw_test_pattern: Drawing test pattern on surface %lu using HDC %p\n", (unsigned long)surface->id, surface->hdc);
    
    RECT rcClient;
    rcClient.left = 0;
    rcClient.top = 0;
    rcClient.right = surface->width;
    rcClient.bottom = surface->height;

    // Fill background (light blue-ish)
    HBRUSH hBgBrush = CreateSolidBrush(RGB(200, 200, 240));
    if (!hBgBrush) {
        printf("ERR: CreateSolidBrush failed for background in test pattern.\n");
        return;
    }
    FillRect(surface->hdc, &rcClient, hBgBrush);
    DeleteObject(hBgBrush);

    HPEN hRedPen = CreatePen(PS_SOLID, 1, RGB(255, 0, 0));
    HPEN hGreenPen = CreatePen(PS_SOLID, 1, RGB(0, 255, 0));
    if (!hRedPen || !hGreenPen) {
        printf("ERR: CreatePen failed in test pattern.\n");
        if(hRedPen) DeleteObject(hRedPen);
        if(hGreenPen) DeleteObject(hGreenPen);
        return;
    }
    
    HPEN hOldPen = (HPEN)SelectObject(surface->hdc, hRedPen);

    MoveToEx(surface->hdc, 0, 0, NULL);
    LineTo(surface->hdc, surface->width - 1, surface->height - 1);
    
    SelectObject(surface->hdc, hGreenPen);
    MoveToEx(surface->hdc, surface->width - 1, 0, NULL);
    LineTo(surface->hdc, 0, surface->height - 1);

    SelectObject(surface->hdc, hOldPen); // Restore old pen
    DeleteObject(hRedPen);
    DeleteObject(hGreenPen);

    surface->dirty = TRUE;
    // The dirty_rect is already set to full surface during creation, or update as needed.
    // For this test pattern, it's assumed the whole surface is modified.
    SetRect(&surface->dirty_rect, 0, 0, surface->width, surface->height);
}
