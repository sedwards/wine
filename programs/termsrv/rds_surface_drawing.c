#include <stdio.h> // For printf or WINE_TRACE/ERR if configured
#include <string.h> // For memset

#include "rds.h"

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


// In programs/termsrv/rds_surface_drawing.c

// Assuming RDS_SERVICE rds_service; is a global in rds.c
// and you might declare it as 'extern RDS_SERVICE rds_service;' here if needed.
// Or, you need a proper way to manage a list of surfaces if not just a single default.

BOOL destroy_surface(DWORD surface_id) {
    extern RDS_SERVICE rds_service; // If accessing a global from rds.c

    printf("termsrv: Attempting to destroy surface ID: %lu\n", (unsigned long)surface_id);

    // This logic assumes you are primarily dealing with rds_service.default_surface
    // and its ID was set correctly during creation.
    if (rds_service.default_surface && rds_service.default_surface->id == surface_id) {
        if (rds_service.default_surface->data) {
            HeapFree(GetProcessHeap(), 0, rds_service.default_surface->data);
            rds_service.default_surface->data = NULL;
        }
        HeapFree(GetProcessHeap(), 0, rds_service.default_surface);
        rds_service.default_surface = NULL; // Very important!
        printf("termsrv: Surface ID %lu destroyed.\n", (unsigned long)surface_id);
        return TRUE;
    }

    // If you have a list of multiple surfaces, you'd search here.
    // For now, this handles the default_surface case.

    printf("termsrv: ERR - destroy_surface: Surface ID %lu not found or default_surface mismatch.\n", (unsigned long)surface_id);
    return FALSE;
}

// Example sketch in programs/termsrv/rds_surface_drawing.c
RDS_SURFACE *create_surface_ex(DWORD width, DWORD height, DWORD bpp) {
    RDS_SURFACE *surface = (RDS_SURFACE *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(RDS_SURFACE));
    if (!surface) return NULL;

    surface->width = width;
    surface->height = height;
    surface->bpp = bpp; // Assume 24 or 32
    surface->stride = (width * (bpp / 8) + 3) & ~3; // DWORD align
    surface->data = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, surface->stride * surface->height);

    if (!surface->data) {
        HeapFree(GetProcessHeap(), 0, surface);
        return NULL;
    }
    // The caller (initialize_service in rds.c) should set surface->id for the default_surface
    return surface;
}

// Enhanced GDI test pattern with comprehensive testing and error reporting
void gdi_draw_test_pattern(RDS_SURFACE *surface)
{
    if (!surface || !surface->hdc) {
        printf("ERR: gdi_draw_test_pattern: null surface or hdc\n");
        return;
    }

    printf("=== GDI Test Pattern Starting ===\n");
    printf("Surface ID: %lu, HDC: %p, Size: %dx%d\n",
           (unsigned long)surface->id, surface->hdc, surface->width, surface->height);

    // Test 1: Basic Rectangle Fill
    printf("Test 1: Background fill...\n");
    RECT rcClient = {0, 0, surface->width, surface->height};
    HBRUSH hBgBrush = CreateSolidBrush(RGB(220, 220, 250));
    if (!hBgBrush) {
        printf("ERR: CreateSolidBrush failed for background (GLE=%lu)\n", GetLastError());
        return;
    }

    int fillResult = FillRect(surface->hdc, &rcClient, hBgBrush);
    if (!fillResult) {
        printf("ERR: FillRect failed (GLE=%lu)\n", GetLastError());
    } else {
        printf("OK: Background filled successfully\n");
    }
    DeleteObject(hBgBrush);

    // Test 2: Pen Creation and Line Drawing
    printf("\nTest 2: Creating pens and drawing lines...\n");
    HPEN hRedPen = CreatePen(PS_SOLID, 3, RGB(255, 0, 0));
    HPEN hGreenPen = CreatePen(PS_SOLID, 2, RGB(0, 255, 0));
    HPEN hBluePen = CreatePen(PS_SOLID, 4, RGB(0, 0, 255));

    if (!hRedPen || !hGreenPen || !hBluePen) {
        printf("ERR: CreatePen failed - Red:%p Green:%p Blue:%p (GLE=%lu)\n",
               hRedPen, hGreenPen, hBluePen, GetLastError());
        if (hRedPen) DeleteObject(hRedPen);
        if (hGreenPen) DeleteObject(hGreenPen);
        if (hBluePen) DeleteObject(hBluePen);
        return;
    }
    printf("OK: All pens created successfully\n");

    // Save original pen
    HPEN hOldPen = (HPEN)SelectObject(surface->hdc, hRedPen);
    if (!hOldPen) {
        printf("ERR: SelectObject for pen failed (GLE=%lu)\n", GetLastError());
    }

    // Test 3: Basic Lines (Diagonal Cross)
    printf("\nTest 3: Drawing diagonal lines...\n");
    BOOL moveResult = MoveToEx(surface->hdc, 10, 10, NULL);
    BOOL lineResult1 = LineTo(surface->hdc, surface->width - 10, surface->height - 10);

    SelectObject(surface->hdc, hGreenPen);
    BOOL moveResult2 = MoveToEx(surface->hdc, surface->width - 10, 10, NULL);
    BOOL lineResult2 = LineTo(surface->hdc, 10, surface->height - 10);

    printf("MoveTo results: %d, %d | LineTo results: %d, %d\n",
           moveResult, moveResult2, lineResult1, lineResult2);

    // Test 4: Rectangles with different pens
    printf("\nTest 4: Drawing rectangles...\n");
    SelectObject(surface->hdc, hBluePen);

    int rectTests[] = {50, 50, 150, 100,    // Rectangle 1
                       200, 50, 300, 100,   // Rectangle 2
                       50, 150, 150, 200,   // Rectangle 3
                       200, 150, 300, 200}; // Rectangle 4

    for (int i = 0; i < 4; i++) {
        int* coords = &rectTests[i * 4];
        BOOL rectResult = Rectangle(surface->hdc, coords[0], coords[1], coords[2], coords[3]);
        printf("Rectangle %d (%d,%d,%d,%d): %s\n", i+1,
               coords[0], coords[1], coords[2], coords[3],
               rectResult ? "OK" : "FAILED");
    }

    // Test 5: Text Output
    printf("\nTest 5: Text output tests...\n");

    // Set text properties
    SetTextColor(surface->hdc, RGB(0, 0, 0));
    SetBkColor(surface->hdc, RGB(255, 255, 255));
    SetBkMode(surface->hdc, OPAQUE);

    const char* testTexts[] = {
        "WineRDS Terminal Server",
        "Line 2: Basic GDI Test",
        "Line 3: Text rendering test",
        "Line 4: Multiple font test"
    };

    for (int i = 0; i < 4; i++) {
        int yPos = 250 + (i * 20);
        BOOL textResult = TextOutA(surface->hdc, 20, yPos, testTexts[i], strlen(testTexts[i]));
        printf("TextOut line %d: %s (result: %d)\n", i+1, testTexts[i], textResult);
    }

    // Test 6: Brushes and Filled Shapes
    printf("\nTest 6: Brushes and filled rectangles...\n");

    HBRUSH brushes[] = {
        CreateSolidBrush(RGB(255, 200, 200)),  // Light red
        CreateSolidBrush(RGB(200, 255, 200)),  // Light green
        CreateSolidBrush(RGB(200, 200, 255)),  // Light blue
        CreateSolidBrush(RGB(255, 255, 200))   // Light yellow
    };

    for (int i = 0; i < 4; i++) {
        if (!brushes[i]) {
            printf("ERR: CreateSolidBrush %d failed\n", i);
            continue;
        }

        RECT fillRect = {350 + (i * 60), 50, 400 + (i * 60), 100};
        int fillResult = FillRect(surface->hdc, &fillRect, brushes[i]);
        printf("Colored rectangle %d: %s\n", i+1, fillResult ? "OK" : "FAILED");
        DeleteObject(brushes[i]);
    }

    // Test 7: Complex Shape Drawing
    printf("\nTest 7: Complex polygon shape...\n");
    SelectObject(surface->hdc, hRedPen);

    POINT polyPoints[] = {
        {400, 150}, {450, 130}, {500, 150}, {480, 200}, {420, 200}, {400, 150}
    };

    BOOL polyResult = Polygon(surface->hdc, polyPoints, 5);
    printf("Polygon drawing: %s\n", polyResult ? "OK" : "FAILED");

    // Test 8: CreateDC Test (the failing case you mentioned)
    printf("\nTest 8: CreateDC testing...\n");

    HDC testDC = CreateDCA("DISPLAY", NULL, NULL, NULL);
    if (testDC == NULL) {
        printf("ERR: CreateDCA(\"DISPLAY\") failed (GLE=%lu)\n", GetLastError());
        printf("This suggests our driver isn't properly handling CreateDC calls\n");
    } else {
        printf("OK: CreateDCA(\"DISPLAY\") succeeded, HDC: %p\n", testDC);

        // Test drawing on the created DC
        HPEN testPen = CreatePen(PS_SOLID, 5, RGB(255, 0, 255));
        if (testPen) {
            HPEN oldTestPen = SelectObject(testDC, testPen);
            BOOL testMove = MoveToEx(testDC, 10, 10, NULL);
            BOOL testLine = LineTo(testDC, 100, 100);

            printf("Test draw on CreateDC - Move: %d, Line: %d\n", testMove, testLine);

            SelectObject(testDC, oldTestPen);
            DeleteObject(testPen);
        }

        DeleteDC(testDC);
        printf("OK: DeleteDC completed\n");
    }

    // Test 9: Compatible DC Test
    printf("\nTest 9: Compatible DC and bitmap test...\n");

    HDC memDC = CreateCompatibleDC(surface->hdc);
    if (!memDC) {
        printf("ERR: CreateCompatibleDC failed (GLE=%lu)\n", GetLastError());
    } else {
        printf("OK: CreateCompatibleDC succeeded, HDC: %p\n", memDC);

        HBITMAP memBitmap = CreateCompatibleBitmap(surface->hdc, 100, 100);
        if (!memBitmap) {
            printf("ERR: CreateCompatibleBitmap failed (GLE=%lu)\n", GetLastError());
        } else {
            printf("OK: CreateCompatibleBitmap succeeded\n");

            HBITMAP oldBitmap = SelectObject(memDC, memBitmap);

            // Draw something on memory DC
            HBRUSH testBrush = CreateSolidBrush(RGB(255, 128, 0));
            if (testBrush) {
                RECT memRect = {0, 0, 100, 100};
                FillRect(memDC, &memRect, testBrush);
                DeleteObject(testBrush);
                printf("OK: Drew on memory DC\n");

                // Try to blit it back
                BOOL blitResult = BitBlt(surface->hdc, 350, 250, 100, 100, memDC, 0, 0, SRCCOPY);
                printf("BitBlt result: %s\n", blitResult ? "OK" : "FAILED");
            }

            SelectObject(memDC, oldBitmap);
            DeleteObject(memBitmap);
        }

        DeleteDC(memDC);
    }

    // Test 10: Device Capabilities
    printf("\nTest 10: Device capabilities...\n");

    int caps[] = {HORZRES, VERTRES, BITSPIXEL, PLANES, TECHNOLOGY, RASTERCAPS};
    const char* capNames[] = {"HORZRES", "VERTRES", "BITSPIXEL", "PLANES", "TECHNOLOGY", "RASTERCAPS"};

    for (int i = 0; i < 6; i++) {
        int value = GetDeviceCaps(surface->hdc, caps[i]);
        printf("%s: %d\n", capNames[i], value);
    }

    // Cleanup
    SelectObject(surface->hdc, hOldPen);
    DeleteObject(hRedPen);
    DeleteObject(hGreenPen);
    DeleteObject(hBluePen);

    // Mark surface as dirty
    surface->dirty = TRUE;
    SetRect(&surface->dirty_rect, 0, 0, surface->width, surface->height);

    printf("\n=== GDI Test Pattern Completed ===\n");
    printf("Surface marked as dirty, rect: (%d,%d,%d,%d)\n",
           surface->dirty_rect.left, surface->dirty_rect.top,
           surface->dirty_rect.right, surface->dirty_rect.bottom);
}

// Additional helper function to test CreateDC specifically
void test_createdc_functionality(void)
{
    printf("\n=== Dedicated CreateDC Test ===\n");

    // Test different CreateDC calls
    const char* drivers[] = {"DISPLAY", "winerds", NULL};
    const char* devices[] = {NULL, "\\\\.\\DISPLAY1", "WineRDS Virtual Display"};

    for (int d = 0; d < 3; d++) {
        for (int dev = 0; dev < 3; dev++) {
            printf("Testing CreateDCA(\"%s\", \"%s\", NULL, NULL)...\n",
                   drivers[d] ? drivers[d] : "NULL",
                   devices[dev] ? devices[dev] : "NULL");

            HDC testHDC = CreateDCA(drivers[d], devices[dev], NULL, NULL);
            if (testHDC) {
                printf("  SUCCESS: HDC = %p\n", testHDC);

                // Test basic operations
                int width = GetDeviceCaps(testHDC, HORZRES);
                int height = GetDeviceCaps(testHDC, VERTRES);
                int bpp = GetDeviceCaps(testHDC, BITSPIXEL);

                printf("  Caps: %dx%dx%d\n", width, height, bpp);

                // Try a simple drawing operation
                HPEN testPen = CreatePen(PS_SOLID, 1, RGB(255, 0, 0));
                if (testPen) {
                    HPEN oldPen = SelectObject(testHDC, testPen);
                    BOOL moveOK = MoveToEx(testHDC, 0, 0, NULL);
                    BOOL lineOK = LineTo(testHDC, 50, 50);
                    printf("  Draw test - Move: %d, Line: %d\n", moveOK, lineOK);

                    SelectObject(testHDC, oldPen);
                    DeleteObject(testPen);
                }

                DeleteDC(testHDC);
                printf("  DeleteDC completed\n");
            } else {
                printf("  FAILED: GetLastError() = %lu\n", GetLastError());
            }
            printf("\n");
        }
    }
}

// Debug function to trace where CreateDC calls are going
void debug_createdc_routing(void)
{
    printf("\n=== CreateDC Routing Debug ===\n");

    // Test the exact sequence that causes black screen
    printf("1. Testing CreateDCA(\"DISPLAY\") - the working case...\n");
    HDC displayDC = CreateDCA("DISPLAY", NULL, NULL, NULL);
    if (displayDC) {
        printf("   HDC: %p\n", displayDC);

        // Check if this HDC maps to our surface
        printf("   Testing drawing operations...\n");

        // Clear to a distinctive color first
        HBRUSH testBrush = CreateSolidBrush(RGB(255, 128, 0)); // Orange
        RECT fullRect = {0, 0, 1024, 768};
        FillRect(displayDC, &fullRect, testBrush);
        DeleteObject(testBrush);
        // Draw something distinctive
        HPEN testPen = CreatePen(PS_SOLID, 10, RGB(0, 255, 255)); // Cyan
        HPEN oldPen = SelectObject(displayDC, testPen);

        // Draw a big X across the screen
        MoveToEx(displayDC, 0, 0, NULL);
        LineTo(displayDC, 1024, 768);
        MoveToEx(displayDC, 1024, 0, NULL);
        LineTo(displayDC, 0, 768);

        // Draw text
        SetTextColor(displayDC, RGB(255, 0, 0));
        SetBkMode(displayDC, TRANSPARENT);
        TextOutA(displayDC, 100, 100, "CREATEDC TEST - Should be visible!", 35);

        SelectObject(displayDC, oldPen);
        DeleteObject(testPen);

        printf("   Drawing completed - check if visible on screen\n");

        // Force flush/update
        GdiFlush();

        DeleteDC(displayDC);
    }

    printf("\n2. Checking if our RDS surface is being updated...\n");
    printf("   Look for RDS messages in pipe communication\n");
    printf("   Check if termsrv is receiving drawing commands\n");
}

// Function to test the specific black screen scenario
void test_black_screen_scenario(void)
{
    printf("\n=== Black Screen Test Scenario ===\n");

    HDC testDC = CreateDCA("DISPLAY", NULL, NULL, NULL);
    if (!testDC) {
        printf("FAILED: Could not create DISPLAY DC\n");
        return;
    }

    printf("Created DC: %p\n", testDC);

    // Try the exact operations that result in black screen
    printf("Setting up drawing context...\n");

    // Set a distinctive background
    HBRUSH bgBrush = CreateSolidBrush(RGB(128, 128, 255)); // Light blue
    RECT screenRect;
    screenRect.left = 0;
    screenRect.top = 0;
    screenRect.right = GetDeviceCaps(testDC, HORZRES);
    screenRect.bottom = GetDeviceCaps(testDC, VERTRES);

    printf("Screen dimensions: %dx%d\n", screenRect.right, screenRect.bottom);

    int fillResult = FillRect(testDC, &screenRect, bgBrush);
    printf("FillRect result: %d\n", fillResult);
    DeleteObject(bgBrush);

    // Draw something simple and obvious
    HPEN bigPen = CreatePen(PS_SOLID, 20, RGB(255, 0, 0));
    HPEN oldPen = SelectObject(testDC, bigPen);

    // Draw a border around the screen
    MoveToEx(testDC, 10, 10, NULL);
    LineTo(testDC, screenRect.right - 10, 10);
    LineTo(testDC, screenRect.right - 10, screenRect.bottom - 10);
    LineTo(testDC, 10, screenRect.bottom - 10);
    LineTo(testDC, 10, 10);

    SelectObject(testDC, oldPen);
    DeleteObject(bigPen);

    // Large text
    HFONT bigFont = CreateFontA(48, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                                CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                                DEFAULT_PITCH | FF_DONTCARE, "Arial");

    if (bigFont) {
        HFONT oldFont = SelectObject(testDC, bigFont);
        SetTextColor(testDC, RGB(255, 255, 0)); // Yellow
        SetBkMode(testDC, TRANSPARENT);

        TextOutA(testDC, 100, 200, "WINE RDS TEST", 13);
        TextOutA(testDC, 100, 300, "Should be visible!", 18);

        SelectObject(testDC, oldFont);
        DeleteObject(bigFont);
    }

    printf("Drawing operations completed\n");

    // Force any pending operations
    GdiFlush();

    printf("GdiFlush() called\n");

    // Keep DC open for a moment to see if it appears
    printf("Keeping DC open for 2 seconds...\n");
    Sleep(2000);

    DeleteDC(testDC);
    printf("DC deleted\n");
}

// Function to check surface synchronization
void check_surface_sync(void)
{
    printf("\n=== Surface Synchronization Check ===\n");

    // This should help identify if the issue is:
    // 1. Drawing operations not reaching our driver
    // 2. Our driver not forwarding to RDS surface
    // 3. RDS surface not being displayed by termsrv

    printf("Check the following in your logs:\n");
    printf("1. Are RDS_MoveTo, RDS_LineTo, etc. being called?\n");
    printf("2. Are SendRDSMessage calls succeeding?\n");
    printf("3. Is termsrv receiving and processing the messages?\n");
    printf("4. Is the surface being marked dirty and updated?\n");

    printf("\nTo enable more debugging:\n");
    printf("1. Add TRACE statements to your RDS_* functions\n");
    printf("2. Check WINEDEBUG=+rdsdrv for more output\n");
    printf("3. Verify pipe communication is working\n");

    printf("\nPossible issues:\n");
    printf("- CreateDC might create a different surface than expected\n");
    printf("- Drawing might go to offscreen buffer\n");
    printf("- Surface updates might not trigger display refresh\n");
    printf("- Coordinate system mismatch\n");
}

/*
 * Surface Routing Fix for CreateDC
 *
 * The issue is that CreateDC creates new surfaces instead of using
 * the existing RDS surface. We need to route all display DCs to
 * the same RDS surface.
 */

// Simple test for CreateDC functionality in termsrv
void test_createdc_simple(void)
{
    HDC testDC;
    HBRUSH testBrush;
    RECT testRect;

    printf("\n=== Simple CreateDC Test ===\n");

    testDC = CreateDCA("DISPLAY", NULL, NULL, NULL);
    if (!testDC) {
        printf("CreateDCA failed: %lu\n", GetLastError());
        return;
    }

    printf("CreateDC succeeded: %p\n", testDC);
    printf("Drawing test pattern...\n");

    // Draw something obvious
    testBrush = CreateSolidBrush(RGB(0, 255, 255)); // Cyan
    testRect.left = 50;
    testRect.top = 50;
    testRect.right = 200;
    testRect.bottom = 150;

    FillRect(testDC, &testRect, testBrush);
    DeleteObject(testBrush);

    SetTextColor(testDC, RGB(255, 0, 0));
    TextOutA(testDC, 60, 80, "SIMPLE TEST", 11);

    printf("Drawing completed\n");
    DeleteDC(testDC);
}

