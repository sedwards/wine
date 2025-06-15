// winerdsdrv.c - Multi-session Wine RDS Display Driver
// This version reads WINERDS_SESSION to distinguish shared buffers

//wine explorer /desktop=User1,800x600 ...
//wine explorer /desktop=User2,800x600 ...
// WINERDS_SESSION=user1 wine explorer /desktop=User1,800x600
 

#include <config.h>
#include <windef.h>
#include <wingdi.h>
#include <winbase.h>
#include "wine/debug.h"
#include "wine/driver.h"
#include "gdi_driver.h"
#include <stdio.h>

WINE_DEFAULT_DEBUG_CHANNEL(rds);

#define WIDTH 800
#define HEIGHT 600

static void *framebuffer_data = NULL;
static HBITMAP framebuffer_bmp = NULL;
static HDC framebuffer_hdc = NULL;
static HANDLE hMap = NULL;

static BOOL RDSDrv_CreateFramebuffer(void) {
    char shm_name[64] = "Global\\winerds_framebuffer_default";
    char session[32] = {0};
    DWORD len = GetEnvironmentVariableA("WINERDS_SESSION", session, sizeof(session));
    if (len > 0 && len < sizeof(session)) {
        snprintf(shm_name, sizeof(shm_name), "Global\\winerds_framebuffer_%s", session);
    }

    BITMAPINFO bmi = {0};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = WIDTH;
    bmi.bmiHeader.biHeight = -HEIGHT;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    framebuffer_hdc = CreateCompatibleDC(NULL);
    framebuffer_bmp = CreateDIBSection(framebuffer_hdc, &bmi, DIB_RGB_COLORS,
                                       &framebuffer_data, NULL, 0);
    if (!framebuffer_bmp || !framebuffer_data) {
        ERR("Failed to create framebuffer DIBSection\n");
        return FALSE;
    }
    SelectObject(framebuffer_hdc, framebuffer_bmp);
    TRACE("Framebuffer created at %p (%dx%d)\n", framebuffer_data, WIDTH, HEIGHT);

    hMap = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0,
                              WIDTH * HEIGHT * 4, shm_name);
    if (!hMap) {
        ERR("Failed to create shared memory\n");
        return FALSE;
    }
    void *shared_view = MapViewOfFile(hMap, FILE_MAP_WRITE, 0, 0, WIDTH * HEIGHT * 4);
    if (!shared_view) {
        ERR("Failed to map shared framebuffer\n");
        return FALSE;
    }
    memcpy(shared_view, framebuffer_data, WIDTH * HEIGHT * 4);
    return TRUE;
}

static BOOL RDSDrv_BitBlt(HDC hdcDst, INT xDst, INT yDst, INT width, INT height,
                          HDC hdcSrc, INT xSrc, INT ySrc, DWORD rop) {
    BitBlt(framebuffer_hdc, xDst, yDst, width, height, hdcSrc, xSrc, ySrc, rop);
    return TRUE;
}

static BOOL RDSDrv_PatBlt(HDC hdc, INT x, INT y, INT width, INT height, DWORD rop) {
    PatBlt(framebuffer_hdc, x, y, width, height, rop);
    return TRUE;
}

static HDC RDSDrv_CreateDC(LPCWSTR driver, LPCWSTR device, LPCWSTR output,
                           const DEVMODEW *init_data) {
    TRACE("Creating Wine RDS DC\n");
    return framebuffer_hdc;
}

static void RDSDrv_DeleteDC(HDC hdc) {
    // NOP
}

static const struct gdi_dc_funcs rds_dc_funcs = {
    RDSDrv_BitBlt,
    NULL,
    RDSDrv_PatBlt,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL, NULL, NULL, NULL
};

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    if (fdwReason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hinstDLL);
        RDSDrv_CreateFramebuffer();
    }
    return TRUE;
}

const struct gdi_driver RDSGdiDriver = {
    RDSDrv_CreateDC,
    RDSDrv_DeleteDC,
    &rds_dc_funcs
};


