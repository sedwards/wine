// Basic drawing functions for winerds.drv

// MoveTo sets the current position
BOOL WINAPI RDS_MoveTo(HDC hdc, int x, int y, LPPOINT lppt)
{
    BOOL ret;
    
    // Use the underlying GDI function but mark the surface as dirty
    ret = MoveToEx(hdc, x, y, lppt);
    
    // Find which surface this HDC belongs to and mark it dirty
    // Note: You'll need a way to map from HDC to RDS_SURFACE
    // This is just pseudo-code for the concept
    PRDS_SURFACE surface = RDS_FindSurfaceByDC(hdc);
    if (surface)
    {
        RECT updateRect = { x - 1, y - 1, x + 1, y + 1 };
        RDS_MarkDirty(surface, &updateRect);
    }
    
    return ret;
}

// LineTo draws a line from current position to specified point
BOOL WINAPI RDS_LineTo(HDC hdc, int x, int y)
{
    BOOL ret;
    POINT currentPos;
    
    // Get current position to calculate dirty rect
    GetCurrentPositionEx(hdc, &currentPos);
    
    // Use the underlying GDI function
    ret = LineTo(hdc, x, y);
    
    // Mark the entire line area as dirty
    PRDS_SURFACE surface = RDS_FindSurfaceByDC(hdc);
    if (surface)
    {
        RECT updateRect = {
            min(currentPos.x, x) - 1,
            min(currentPos.y, y) - 1,
            max(currentPos.x, x) + 1,
            max(currentPos.y, y) + 1
        };
        RDS_MarkDirty(surface, &updateRect);
    }
    
    return ret;
}

// Rectangle draws a rectangle
BOOL WINAPI RDS_Rectangle(HDC hdc, int left, int top, int right, int bottom)
{
    BOOL ret;
    
    // Use the underlying GDI function
    ret = Rectangle(hdc, left, top, right, bottom);
    
    // Mark rectangle area as dirty
    PRDS_SURFACE surface = RDS_FindSurfaceByDC(hdc);
    if (surface)
    {
        RECT updateRect = { left, top, right, bottom };
        RDS_MarkDirty(surface, &updateRect);
    }
    
    return ret;
}

// BitBlt copies a bitmap from one DC to another
BOOL WINAPI RDS_BitBlt(HDC hdcDest, int xDest, int yDest, int width, int height,
                      HDC hdcSrc, int xSrc, int ySrc, DWORD rop)
{
    BOOL ret;
    
    // Use the underlying GDI function
    ret = BitBlt(hdcDest, xDest, yDest, width, height, hdcSrc, xSrc, ySrc, rop);
    
    // Mark destination area as dirty
    PRDS_SURFACE surface = RDS_FindSurfaceByDC(hdcDest);
    if (surface)
    {
        RECT updateRect = { xDest, yDest, xDest + width, yDest + height };
        RDS_MarkDirty(surface, &updateRect);
    }
    
    return ret;
}

