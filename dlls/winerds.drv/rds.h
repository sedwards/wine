#ifndef RDS_H
#define RDS_H

#include <windows.h> // For HDC, HBITMAP, RECT, DWORD, BOOL etc.

// Forward declaration if RDS_SERVICE uses RDS_SURFACE or vice-versa in a way that needs it.
// Not strictly needed if RDS_SERVICE definition comes after RDS_SURFACE.
typedef struct _RDS_SURFACE RDS_SURFACE;

/* 
 * RDS_SURFACE structure for managing a drawing surface.
 * This version is refactored to use DIB sections and memory HDCs.
 */
typedef struct _RDS_SURFACE {
    DWORD id;         /* Unique surface ID */
    int width;
    int height;
    int bpp;          /* Bits per pixel */
    HDC hdc;          /* Memory HDC for this surface */
    HBITMAP hBitmap;      /* DIB section HBITMAP selected into hdc */
    void *pBitmapBits;  /* Pointer to DIB bits (from CreateDIBSection) */
    int stride;       /* Bytes per row of pBitmapBits (optional, can be derived) */
    BOOL dirty;       /* Whether surface needs updating */
    RECT dirty_rect;  /* Region that needs updating */

    /* 
     * current_x and current_y are now less critical if all drawing goes through the HDC,
     * as the HDC maintains the current pen position. They can be kept for reference
     * or if some hybrid drawing occurs.
     */
    DWORD current_x;
    DWORD current_y;
} RDS_SURFACE;


// Assuming RDS_SERVICE struct definition might be here or included from here.
// For this task, only RDS_SURFACE is directly modified.
typedef struct _RDS_SERVICE {
    RDS_SURFACE *default_surface;
    // Other service-wide data
    // For example, a list of all active surfaces:
    // RDS_SURFACE *surfaces[MAX_SURFACES];
    // int num_surfaces;
} RDS_SERVICE;

// Function Prototypes that might be in rds.h (for awareness, not direct modification by worker here for Part 1)
// These would operate on or return RDS_SURFACE structures.
// Their internal implementations will change due to the RDS_SURFACE refactor.

// Surface management
// RDS_SURFACE *create_surface_ex(DWORD width, DWORD height, DWORD bpp); // Internally uses CreateDIBSection
// void destroy_surface(DWORD surface_id); // Handles DeleteDC, DeleteObject, HeapFree
// void *surface_get_data(RDS_SURFACE *surface); // Returns pBitmapBits

// Drawing functions (will be refactored later to use HDC from RDS_SURFACE)
// These are typically implemented in rds_surface_drawing.c
// BOOL gdi_move_to(RDS_SURFACE *surface, int x, int y);
// BOOL gdi_line_to(RDS_SURFACE *surface, int x, int y, COLORREF color);
// BOOL gdi_rectangle(RDS_SURFACE *surface, int left, int top, int right, int bottom, COLORREF color, BOOL filled);
// BOOL gdi_text_out(RDS_SURFACE *surface, int x, int y, const WCHAR *text, int count, COLORREF color);
// void gdi_draw_test_pattern(RDS_SURFACE *surface); // Will use HDC for drawing

// Global service object (if defined as such)
// extern RDS_SERVICE rds_service;

#endif // RDS_H
