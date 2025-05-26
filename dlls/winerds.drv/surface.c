#include <windows.h>
#include <wingdi.h>
#include <winuser.h>
#include "wine/debug.h"
#include "wine/heap.h"
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"

WINE_DEFAULT_DEBUG_CHANNEL(winerds);

// Structure to represent our in-memory surface
typedef struct _RDS_SURFACE
{
    DWORD   width;
    DWORD   height;
    DWORD   bpp;        // Bits per pixel
    DWORD   stride;     // Bytes per scanline
    BYTE*   buffer;     // Actual pixel data
    HBITMAP hBitmap;    // GDI bitmap handle if needed
    HDC     memDC;      // Memory DC for drawing operations
    BOOL    isDirty;    // Flag to indicate if surface needs update
    RECT    dirtyRect;  // Rectangle representing the dirty region
} RDS_SURFACE, *PRDS_SURFACE;

// Global state for the driver
typedef struct _RDS_DRIVER_STATE
{
    CRITICAL_SECTION surfaceLock;  // Lock for thread safety
    DWORD            activeSurfaces;
    // Could add more global state here
} RDS_DRIVER_STATE;

static RDS_DRIVER_STATE g_driverState;

// Initialize the RDS driver
BOOL RDS_Init(void)
{
    WINE_TRACE("Initializing WineRDS driver\n");
    
    // Initialize critical section for thread safety
    InitializeCriticalSection(&g_driverState.surfaceLock);
    g_driverState.activeSurfaces = 0;
    
    return TRUE;
}

// Clean up the RDS driver
void RDS_Cleanup(void)
{
    WINE_TRACE("Cleaning up WineRDS driver\n");
    
    DeleteCriticalSection(&g_driverState.surfaceLock);
}

// Calculate the stride (bytes per scanline) based on width and bpp
static DWORD CalculateStride(DWORD width, DWORD bpp)
{
    // Calculate bytes per scanline and align to DWORD boundary
    return ((width * bpp + 31) / 32) * 4;
}

// Create a new surface
PRDS_SURFACE RDS_CreateSurface(DWORD width, DWORD height, DWORD bpp)
{
    PRDS_SURFACE surface = NULL;
    HDC displayDC = NULL;
    BITMAPINFO bmi;
    LPVOID bits = NULL;
    
    WINE_TRACE("Creating surface %dx%d with %d bpp\n", width, height, bpp);
    
    // Validate input parameters
    if (width == 0 || height == 0 || (bpp != 24 && bpp != 32))
    {
        WINE_ERR("Invalid surface parameters: %dx%d, %d bpp\n", width, height, bpp);
        return NULL;
    }
    
    // Allocate the surface structure using Wine's heap allocator for better integration
    surface = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(RDS_SURFACE));
    if (!surface)
    {
        WINE_ERR("Failed to allocate surface structure\n");
        return NULL;
    }
    
    // Calculate stride (bytes per scanline)
    DWORD stride = CalculateStride(width, bpp);
    
    // Initialize the surface structure
    surface->width = width;
    surface->height = height;
    surface->bpp = bpp;
    surface->stride = stride;
    surface->isDirty = FALSE;
    SetRectEmpty(&surface->dirtyRect);
    
    // Create compatible DC for GDI operations
    displayDC = GetDC(NULL);
    if (!displayDC)
    {
        WINE_ERR("Failed to get display DC\n");
        HeapFree(GetProcessHeap(), 0, surface);
        return NULL;
    }
    
    surface->memDC = CreateCompatibleDC(displayDC);
    ReleaseDC(NULL, displayDC);
    
    if (!surface->memDC)
    {
        WINE_ERR("Failed to create memory DC\n");
        HeapFree(GetProcessHeap(), 0, surface);
        return NULL;
    }
    
    // Initialize BITMAPINFO structure for DIB creation
    ZeroMemory(&bmi, sizeof(BITMAPINFO));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -(LONG)height;  // Negative height for top-down DIB
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = bpp;
    bmi.bmiHeader.biCompression = BI_RGB;
    
    // Create DIB section
    surface->hBitmap = CreateDIBSection(surface->memDC, &bmi, DIB_RGB_COLORS, &bits, NULL, 0);
    if (!surface->hBitmap)
    {
        WINE_ERR("Failed to create DIB section\n");
        DeleteDC(surface->memDC);
        HeapFree(GetProcessHeap(), 0, surface);
        return NULL;
    }
    
    // Select the bitmap into the DC
    SelectObject(surface->memDC, surface->hBitmap);
    
    // Store the buffer pointer
    surface->buffer = bits;
    
    // Initialize the surface with white background
    RECT rect = { 0, 0, width, height };
    FillRect(surface->memDC, &rect, (HBRUSH)GetStockObject(WHITE_BRUSH));
    
    // Add to active surfaces count
    EnterCriticalSection(&g_driverState.surfaceLock);
    g_driverState.activeSurfaces++;
    LeaveCriticalSection(&g_driverState.surfaceLock);
    
    WINE_TRACE("Surface created successfully: %p\n", surface);
    return surface;
}

// Destroy a surface
BOOL RDS_DestroySurface(PRDS_SURFACE surface)
{
    if (!surface)
    {
        WINE_ERR("Invalid surface pointer\n");
        return FALSE;
    }
    
    WINE_TRACE("Destroying surface %p\n", surface);
    
    // Clean up GDI resources
    if (surface->hBitmap)
        DeleteObject(surface->hBitmap);
    
    if (surface->memDC)
        DeleteDC(surface->memDC);
    
    // The actual buffer memory is managed by the DIB section
    
    // Free the surface structure
    HeapFree(GetProcessHeap(), 0, surface);
    
    // Update active surfaces count
    EnterCriticalSection(&g_driverState.surfaceLock);
    g_driverState.activeSurfaces--;
    LeaveCriticalSection(&g_driverState.surfaceLock);
    
    return TRUE;
}

// Mark a rectangle as dirty (needing update)
void RDS_MarkDirty(PRDS_SURFACE surface, const RECT* rect)
{
    RECT updateRect;
    
    if (!surface || !rect)
        return;
    
    // Clip the rectangle to the surface bounds
    updateRect.left = max(0, rect->left);
    updateRect.top = max(0, rect->top);
    updateRect.right = min(surface->width, rect->right);
    updateRect.bottom = min(surface->height, rect->bottom);
    
    // If rectangle is valid
    if (updateRect.left < updateRect.right && updateRect.top < updateRect.bottom)
    {
        // If surface wasn't dirty, just use this rect
        if (!surface->isDirty)
        {
            surface->dirtyRect = updateRect;
            surface->isDirty = TRUE;
        }
        else
        {
            // Union with existing dirty rect
            UnionRect(&surface->dirtyRect, &surface->dirtyRect, &updateRect);
        }
    }
}

// Get the DC for drawing on the surface
HDC RDS_GetDC(PRDS_SURFACE surface)
{
    if (!surface)
        return NULL;
    
    return surface->memDC;
}

// Lock the surface for direct access to pixel data
BYTE* RDS_LockSurface(PRDS_SURFACE surface, DWORD* pStride)
{
    if (!surface || !pStride)
        return NULL;
    
    *pStride = surface->stride;
    return surface->buffer;
}

// Unlock the surface after direct access
void RDS_UnlockSurface(PRDS_SURFACE surface)
{
    // Nothing special needed for our implementation
    // This is a placeholder for future functionality
}

// Helper function to export surface data in a format suitable for RDP/Broadway
BOOL RDS_GetSurfaceData(PRDS_SURFACE surface, BYTE** ppData, DWORD* pSize, RECT* pUpdateRect)
{
    if (!surface || !ppData || !pSize)
        return FALSE;
    
    // If nothing is dirty, nothing to update
    if (!surface->isDirty)
    {
        *ppData = NULL;
        *pSize = 0;
        SetRectEmpty(pUpdateRect);
        return TRUE;
    }
    
    // For now, we just return the entire buffer
    // In a more optimized implementation, you would only copy the dirty region
    *ppData = surface->buffer;
    *pSize = surface->height * surface->stride;
    *pUpdateRect = surface->dirtyRect;
    
    // Reset dirty flag and rect
    surface->isDirty = FALSE;
    SetRectEmpty(&surface->dirtyRect);
    
    return TRUE;
}

