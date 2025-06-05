#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <time.h>

#include "rds.h" 
#include "pipe_server.h"
#include "wine/debug.h"

/* Try to include Wine's internal PNG library */
#ifdef HAVE_PNG_H
#include <png.h>
#define USE_PNG_OUTPUT 1
#else
#define USE_PNG_OUTPUT 0
#endif

WINE_DEFAULT_DEBUG_CHANNEL(termsrv);

RDS_SERVICE rds_service; /* Global service struct */

/* Screenshot functionality */
static HANDLE screenshot_thread = NULL;
static BOOL screenshot_active = FALSE;

/* Screenshot thread function */
static DWORD WINAPI screenshot_thread_proc(LPVOID param)
{
    WCHAR filename[MAX_PATH];
    SYSTEMTIME st;
    int screenshot_count = 1;
    int i;
    
    TRACE("Screenshot thread started\n");
    
    while (screenshot_active && !rds_service.should_exit)
    {
        if (rds_service.default_surface && rds_service.default_surface->pBitmapBits)
        {
            GetLocalTime(&st);
            
#if USE_PNG_OUTPUT
            swprintf(filename, ARRAYSIZE(filename), 
                    L"rds_screenshot_%04d%02d%02d_%02d%02d%02d_%03d.png",
                    st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, screenshot_count);
            
            if (save_surface_to_png(rds_service.default_surface, filename)) {
                TRACE("Saved PNG screenshot: %s\n", debugstr_w(filename));
            } else {
                /* Fallback to BMP */
                swprintf(filename, ARRAYSIZE(filename), 
                        L"rds_screenshot_%04d%02d%02d_%02d%02d%02d_%03d.bmp",
                        st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, screenshot_count);
                save_surface_to_bmp(rds_service.default_surface, filename);
                TRACE("Saved BMP screenshot (PNG fallback): %s\n", debugstr_w(filename));
            }
#else
            swprintf(filename, ARRAYSIZE(filename), 
                    L"rds_screenshot_%04d%02d%02d_%02d%02d%02d_%03d.bmp",
                    st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, screenshot_count);
            
            save_surface_to_bmp(rds_service.default_surface, filename);
            TRACE("Saved BMP screenshot: %s\n", debugstr_w(filename));
#endif
            screenshot_count++;
        }
        
        /* Wait 60 seconds */
        for (i = 0; i < 600 && screenshot_active && !rds_service.should_exit; i++)
        {
            Sleep(100);
        }
    }
    
    TRACE("Screenshot thread exiting\n");
    return 0;
}

#if USE_PNG_OUTPUT
/* PNG save function using Wine's internal libpng */
BOOL save_surface_to_png(RDS_SURFACE *surface, const WCHAR *filename)
{
    FILE *fp;
    png_structp png_ptr;
    png_infop info_ptr;
    BOOL result = FALSE;
    char filename_utf8[MAX_PATH];
    BYTE **row_pointers = NULL;
    BYTE *src_bits;
    DWORD src_row_size;
    int y;
    
    if (!surface || !surface->pBitmapBits || !filename)
        return FALSE;
    
    /* Convert filename to UTF-8 */
    if (!WideCharToMultiByte(CP_UTF8, 0, filename, -1, filename_utf8, sizeof(filename_utf8), NULL, NULL)) {
        ERR("Failed to convert filename to UTF-8\n");
        return FALSE;
    }
    
    /* Open file for writing */
    fp = fopen(filename_utf8, "wb");
    if (!fp) {
        ERR("Failed to open file %s for writing\n", filename_utf8);
        return FALSE;
    }
    
    /* Create PNG structures */
    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) {
        ERR("png_create_write_struct failed\n");
        fclose(fp);
        return FALSE;
    }
    
    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        ERR("png_create_info_struct failed\n");
        png_destroy_write_struct(&png_ptr, NULL);
        fclose(fp);
        return FALSE;
    }
    
    /* Set up error handling */
    if (setjmp(png_jmpbuf(png_ptr))) {
        ERR("PNG write error occurred\n");
        png_destroy_write_struct(&png_ptr, &info_ptr);
        if (row_pointers) HeapFree(GetProcessHeap(), 0, row_pointers);
        fclose(fp);
        return FALSE;
    }
    
    /* Set up PNG */
    png_init_io(png_ptr, fp);
    
    /* Write PNG header */
    png_set_IHDR(png_ptr, info_ptr, surface->width, surface->height,
                 8, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
    
    png_write_info(png_ptr, info_ptr);
    
    /* Prepare row pointers */
    row_pointers = (BYTE **)HeapAlloc(GetProcessHeap(), 0, surface->height * sizeof(BYTE *));
    if (!row_pointers) {
        ERR("Failed to allocate row pointers\n");
        goto cleanup;
    }
    
    src_bits = (BYTE *)surface->pBitmapBits;
    src_row_size = ((surface->width * (surface->bpp / 8) + 3) & ~3);
    
    /* Set up row pointers (flip vertically since DIB is top-down, PNG expects bottom-up) */
    for (y = 0; y < surface->height; y++) {
        row_pointers[y] = src_bits + ((surface->height - 1 - y) * src_row_size);
    }
    
    /* Write image data */
    png_write_image(png_ptr, row_pointers);
    png_write_end(png_ptr, NULL);
    
    result = TRUE;
    TRACE("Successfully saved %ux%u PNG to %s\n", surface->width, surface->height, filename_utf8);
    
cleanup:
    if (row_pointers) HeapFree(GetProcessHeap(), 0, row_pointers);
    png_destroy_write_struct(&png_ptr, &info_ptr);
    fclose(fp);
    return result;
}
#else
/* Stub for when PNG is not available */
BOOL save_surface_to_png(RDS_SURFACE *surface, const WCHAR *filename)
{
    TRACE("PNG support not available, falling back to BMP\n");
    return FALSE;
}
#endif

/* Keep the existing BMP save function */
BOOL save_surface_to_bmp(RDS_SURFACE *surface, const WCHAR *filename)
{
    HANDLE file;
    BITMAPFILEHEADER file_header;
    BITMAPINFOHEADER info_header;
    DWORD bytes_written;
    BOOL result = FALSE;
    DWORD row_size;
    DWORD image_size;
    BYTE *src_bits;
    DWORD src_row_size;
    int y;
    
    if (!surface || !surface->pBitmapBits || !filename)
        return FALSE;
    
    file = CreateFileW(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 
                      FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE)
    {
        ERR("Failed to create file %s, error %lu\n", debugstr_w(filename), GetLastError());
        return FALSE;
    }
    
    /* Calculate image size */
    row_size = ((surface->width * (surface->bpp / 8) + 3) & ~3);
    image_size = row_size * surface->height;
    
    /* Fill BMP file header */
    file_header.bfType = 0x4D42; /* 'BM' */
    file_header.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + image_size;
    file_header.bfReserved1 = 0;
    file_header.bfReserved2 = 0;
    file_header.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    
    /* Fill BMP info header */
    info_header.biSize = sizeof(BITMAPINFOHEADER);
    info_header.biWidth = surface->width;
    info_header.biHeight = surface->height; /* Positive for bottom-up DIB */
    info_header.biPlanes = 1;
    info_header.biBitCount = surface->bpp;
    info_header.biCompression = 0; /* BI_RGB */
    info_header.biSizeImage = image_size;
    info_header.biXPelsPerMeter = 2835; /* ~72 DPI */
    info_header.biYPelsPerMeter = 2835;
    info_header.biClrUsed = 0;
    info_header.biClrImportant = 0;
    
    /* Write headers */
    if (!WriteFile(file, &file_header, sizeof(file_header), &bytes_written, NULL) ||
        bytes_written != sizeof(file_header))
    {
        ERR("Failed to write file header\n");
        goto cleanup;
    }
    
    if (!WriteFile(file, &info_header, sizeof(info_header), &bytes_written, NULL) ||
        bytes_written != sizeof(info_header))
    {
        ERR("Failed to write info header\n");
        goto cleanup;
    }
    
    /* Write bitmap data */
    src_bits = (BYTE *)surface->pBitmapBits;
    src_row_size = ((surface->width * (surface->bpp / 8) + 3) & ~3);
    
    for (y = surface->height - 1; y >= 0; y--)
    {
        BYTE *row_data = src_bits + (y * src_row_size);
        if (!WriteFile(file, row_data, row_size, &bytes_written, NULL) ||
            bytes_written != row_size)
        {
            ERR("Failed to write row %d\n", y);
            goto cleanup;
        }
    }
    
    result = TRUE;
    TRACE("Successfully saved %ux%u BMP to %s\n", surface->width, surface->height, debugstr_w(filename));
    
cleanup:
    CloseHandle(file);
    return result;
}

/* --- Stubs for functions assumed to exist --- */
BOOL surface_initialize(void) {
    TRACE("surface_initialize: STUB\n");
    return TRUE;
}

void surface_shutdown(void) {
    TRACE("surface_shutdown: STUB\n");
}

BOOL gdi_initialize(void) {
    TRACE("gdi_initialize: STUB\n");
    return TRUE;
}

void gdi_shutdown(void) {
    TRACE("gdi_shutdown: STUB\n");
}

/* --- Actual service functions --- */
BOOL initialize_service(void)
{
    BITMAPINFO bmi;
    
    memset(&rds_service, 0, sizeof(rds_service));
    rds_service.should_exit = FALSE;

    TRACE("Initializing RDS service\n");

    if (!surface_initialize()) { 
        ERR("surface_initialize failed\n");
        return FALSE; 
    }
    if (!gdi_initialize()) { 
        ERR("gdi_initialize failed\n");
        surface_shutdown(); 
        return FALSE; 
    }

    /* Create a default surface using DIB section */
    rds_service.default_surface = (RDS_SURFACE*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(RDS_SURFACE));
    if (!rds_service.default_surface) {
        ERR("Failed to allocate memory for default_surface\n");
        gdi_shutdown();
        surface_shutdown();
        return FALSE;
    }

    rds_service.default_surface->id = 1; /* Default display surface ID */
    rds_service.default_surface->width = 800;
    rds_service.default_surface->height = 600;
    rds_service.default_surface->bpp = 32;

    memset(&bmi, 0, sizeof(BITMAPINFO));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = rds_service.default_surface->width;
    bmi.bmiHeader.biHeight = -rds_service.default_surface->height; /* Top-down DIB */
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = rds_service.default_surface->bpp;
    bmi.bmiHeader.biCompression = BI_RGB;

    rds_service.default_surface->hBitmap = CreateDIBSection(
        NULL, /* hdc (optional, NULL for memory DIB) */
        &bmi,
        DIB_RGB_COLORS,
        &rds_service.default_surface->pBitmapBits,
        NULL, /* hSection (for file mapping objects, not needed here) */
        0     /* dwOffset */
    );

    if (!rds_service.default_surface->hBitmap) {
        ERR("CreateDIBSection failed, GLE=%lu\n", GetLastError());
        HeapFree(GetProcessHeap(), 0, rds_service.default_surface);
        rds_service.default_surface = NULL;
        gdi_shutdown();
        surface_shutdown();
        return FALSE;
    }

    rds_service.default_surface->hdc = CreateCompatibleDC(NULL);
    if (!rds_service.default_surface->hdc) {
        ERR("CreateCompatibleDC failed, GLE=%lu\n", GetLastError());
        DeleteObject(rds_service.default_surface->hBitmap);
        HeapFree(GetProcessHeap(), 0, rds_service.default_surface);
        rds_service.default_surface = NULL;
        gdi_shutdown();
        surface_shutdown();
        return FALSE;
    }

    /* Select the DIB section into the memory DC */
    SelectObject(rds_service.default_surface->hdc, rds_service.default_surface->hBitmap);
    
    /* Initialize dirty rect */
    SetRect(&rds_service.default_surface->dirty_rect, 0, 0, 
            rds_service.default_surface->width, rds_service.default_surface->height);
    rds_service.default_surface->dirty = TRUE;

    TRACE("Default surface created with DIB section and memory HDC\n");

    /* Draw a test pattern on the default surface */
    gdi_draw_test_pattern(rds_service.default_surface);
    TRACE("Test pattern drawn on default surface\n");

    /* Start screenshot thread */
    screenshot_active = TRUE;
    screenshot_thread = CreateThread(NULL, 0, screenshot_thread_proc, NULL, 0, NULL);
    if (!screenshot_thread) {
        ERR("Failed to create screenshot thread\n");
        screenshot_active = FALSE;
    } else {
        TRACE("Screenshot thread started\n");
    }

    /* Start Pipe Server */
    if (!StartRDSPipeServer())
    {
        ERR("Failed to start RDS pipe server\n");
        if (rds_service.default_surface) {
            destroy_surface(rds_service.default_surface->id);
            rds_service.default_surface = NULL;
        }
        gdi_shutdown();
        surface_shutdown();
        return FALSE;
    }
    TRACE("RDS Pipe Server started successfully\n");

    TRACE("RDS service initialized successfully\n");
    return TRUE;
}

static void shutdown_service(void)
{
    TRACE("Shutting down RDS service\n");

    /* Stop screenshot thread */
    if (screenshot_thread) {
        screenshot_active = FALSE;
        WaitForSingleObject(screenshot_thread, 5000);
        CloseHandle(screenshot_thread);
        screenshot_thread = NULL;
        TRACE("Screenshot thread stopped\n");
    }

    StopRDSPipeServer();
    TRACE("RDS Pipe Server stopped\n");

    if (rds_service.default_surface)
    {
        TRACE("Cleaning up default surface: HDC=%p, HBITMAP=%p\n", 
               rds_service.default_surface->hdc, rds_service.default_surface->hBitmap);
        
        if (rds_service.default_surface->hdc)
        {
            DeleteDC(rds_service.default_surface->hdc);
        }
        if (rds_service.default_surface->hBitmap)
        {
            DeleteObject(rds_service.default_surface->hBitmap);
        }
        HeapFree(GetProcessHeap(), 0, rds_service.default_surface);
        rds_service.default_surface = NULL;
    }

    gdi_shutdown();
    surface_shutdown();
    TRACE("RDS service shut down completed\n");
}

void process_events(void) {
    TRACE("process_events: Service loop running. Pipe server is active.\n");
    
    while (!rds_service.should_exit) {
        /* This loop keeps termsrv alive. */
        /* You can add periodic tasks here (like manual screenshot triggers) */
        /* or handling for other events if termsrv becomes more complex. */
        Sleep(100); /* Sleep briefly to prevent high CPU usage */
    }
    TRACE("process_events: Service loop exiting.\n");
}

/* Add a console control handler to catch Ctrl+C for graceful shutdown */
static BOOL WINAPI console_ctrl_handler(DWORD ctrl_type) {
    switch (ctrl_type) {
        case CTRL_C_EVENT:
        case CTRL_BREAK_EVENT:
        case CTRL_CLOSE_EVENT:
            TRACE("Shutdown signal (Ctrl+C or Close) received.\n");
            rds_service.should_exit = TRUE; /* Signal the main loop to terminate */
            return TRUE; /* Indicate we've handled the signal */
    }
    return FALSE;
}

/* Main function */
int wmain(void) {
    TRACE("Starting RDS Terminal Service\n");
    
    if (!SetConsoleCtrlHandler(console_ctrl_handler, TRUE)) {
        ERR("Could not set console control handler.\n");
        /* Continue anyway, but Ctrl+C might not be clean. */
    }

    if (initialize_service()) {
        process_events(); /* This will block until should_exit is true */
        shutdown_service();
    } else {
        ERR("RDS Terminal Service initialization failed.\n");
        return 1;
    }
    TRACE("RDS Terminal Service finished.\n");
    return 0;
}

