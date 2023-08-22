#if 0
#pragma makedep unix
#endif

#include "config.h"

#include <fcntl.h>
#include <poll.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <assert.h>
#include <dlfcn.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS

#include "basetsd.h"
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "wingdi.h"
#include "ntgdi.h"

#include "wine/server.h"
#include "wine/debug.h"
#include "wine/list.h"

#include "unixlib.h"
#include "broadway-server.h"
#include "broadwaydrv.h"

WINE_DEFAULT_DEBUG_CHANNEL(broadway);
#if 0
static struct x11drv_display_device_handler host_handler;
static struct x11drv_settings_handler settings_handler;

const unsigned int *depths;

static pthread_mutex_t settings_mutex = PTHREAD_MUTEX_INITIALIZER;

void X11DRV_Settings_SetHandler(const struct x11drv_settings_handler *new_handler)
{
    if (new_handler->priority > settings_handler.priority)
    {
        settings_handler = *new_handler;
        TRACE("Display settings are now handled by: %s.\n", settings_handler.name);
    }
}

/***********************************************************************
 * Default handlers if resolution switching is not enabled
 *
 */
static BOOL nores_get_id(const WCHAR *device_name, BOOL is_primary, x11drv_settings_id *id)
{
    id->id = is_primary ? 1 : 0;
    return TRUE;
}

static BOOL nores_get_modes(x11drv_settings_id id, DWORD flags, DEVMODEW **new_modes, UINT *mode_count)
{
    RECT primary = get_host_primary_monitor_rect();
    DEVMODEW *modes;

    modes = calloc(1, sizeof(*modes));
    if (!modes)
    {
        RtlSetLastWin32Error( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
    }

    modes[0].dmSize = sizeof(*modes);
    modes[0].dmDriverExtra = 0;
    modes[0].dmFields = DM_DISPLAYORIENTATION | DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT |
                        DM_DISPLAYFLAGS | DM_DISPLAYFREQUENCY;
    modes[0].dmDisplayOrientation = DMDO_DEFAULT;
    modes[0].dmBitsPerPel = screen_bpp;
    modes[0].dmPelsWidth = primary.right;
    modes[0].dmPelsHeight = primary.bottom;
    modes[0].dmDisplayFlags = 0;
    modes[0].dmDisplayFrequency = 60;

    *new_modes = modes;
    *mode_count = 1;
    return TRUE;
}
#endif
POINT virtual_screen_to_root(INT x, INT y)
{
    RECT virtual = NtUserGetVirtualScreenRect();
    POINT pt;

    pt.x = x - virtual.left;
    pt.y = y - virtual.top;
    return pt;
}

POINT root_to_virtual_screen(INT x, INT y)
{
    RECT virtual = NtUserGetVirtualScreenRect();
    POINT pt;

    pt.x = x + virtual.left;
    pt.y = y + virtual.top;
    return pt;
}

/* Get the primary monitor rect from the host system */
RECT get_host_primary_monitor_rect(void)
{
    RECT rect = {0};

    return rect;
}


#if 0


void BOXEDDRV_DisplayDevices_Init(BOOL force)
{
    HANDLE mutex;
    struct boxed_gpu gpus[1];
    struct boxed_adapter adapters[1];
    struct boxed_monitor monitors[1];
    INT gpu_count, adapter_count, monitor_count;
    INT gpu, adapter, monitor;
    HDEVINFO gpu_devinfo = NULL, monitor_devinfo = NULL;
    HKEY video_hkey = NULL;
    INT video_index = 0;
    DWORD disposition = 0;
    WCHAR guidW[40];
    WCHAR driverW[1024];
    LUID gpu_luid;
    UINT output_id = 0;
    static const WCHAR wine_adapterW[] = { 'W','i','n','e',' ','A','d','a','p','t','e','r',0 };
    static const WCHAR generic_nonpnp_monitorW[] = {
        'G','e','n','e','r','i','c',' ',
        'N','o','n','-','P','n','P',' ','M','o','n','i','t','o','r',0 };

    mutex = get_display_device_init_mutex();

    if (RegCreateKeyExW(HKEY_LOCAL_MACHINE, video_keyW, 0, NULL, REG_OPTION_VOLATILE, KEY_ALL_ACCESS, NULL, &video_hkey,
        &disposition))
    {
        ERR("Failed to create video device key\n");
        goto done;
    }

    /* Avoid unnecessary reinit */
    if (!force && disposition != REG_CREATED_NEW_KEY)
        goto done;

    prepare_devices(video_hkey);

    gpu_devinfo = SetupDiCreateDeviceInfoList(&GUID_DEVCLASS_DISPLAY, NULL);
    monitor_devinfo = SetupDiCreateDeviceInfoList(&GUID_DEVCLASS_MONITOR, NULL);

    /* Initialize GPUs */
    // if (!handler->get_gpus(&gpus, &gpu_count))
    //     goto done;    
    lstrcpyW(gpus[0].name, wine_adapterW);
    gpus[0].id = 1;
    gpus[0].vendor_id = 0;
    gpus[0].device_id = 0;
    gpus[0].subsys_id = 0;
    gpus[0].revision_id = 0;
    memset(&gpus[0].vulkan_uuid, 0, sizeof(gpus[0].vulkan_uuid));

    gpu_count = 1;
    TRACE("GPU count: %d\n", gpu_count);

    for (gpu = 0; gpu < gpu_count; gpu++)
    {
        if (!BOXEDDRV_InitGpu(gpu_devinfo, &gpus[gpu], gpu, guidW, driverW, &gpu_luid))
            goto done;

        /* Initialize adapters */
        /*
        if (!handler->get_adapters(gpus[gpu].id, &adapters, &adapter_count))
            goto done;
            */
        adapter_count = 1;
        adapters[0].id = 1;
        adapters[0].state_flags = DISPLAY_DEVICE_PRIMARY_DEVICE | DISPLAY_DEVICE_ATTACHED_TO_DESKTOP;
        TRACE("GPU: %#lx %s, adapter count: %d\n", gpus[gpu].id, wine_dbgstr_w(gpus[gpu].name), adapter_count);

        for (adapter = 0; adapter < adapter_count; adapter++)
        {
            /*
            if (!handler->get_monitors(adapters[adapter].id, &monitors, &monitor_count))
                goto done;
                */
            INT width = boxeddrv_GetDeviceCaps(NULL, DESKTOPHORZRES);
            INT height = boxeddrv_GetDeviceCaps(NULL, DESKTOPVERTRES);
            monitor_count = 1;
            monitors[0].state_flags = DISPLAY_DEVICE_ATTACHED | DISPLAY_DEVICE_ACTIVE;
            SetRect(&monitors[0].rc_monitor, 0, 0, width, height);
            SetRect(&monitors[0].rc_work, 0, 0, width, height);
            lstrcpyW(monitors[0].name, generic_nonpnp_monitorW);
            TRACE("adapter: %#lx, monitor count: %d\n", adapters[adapter].id, monitor_count);

            if (!BOXEDDRV_InitAdapter(video_hkey, video_index, gpu, adapter, monitor_count,
                &gpus[gpu], guidW, driverW, &adapters[adapter]))
                goto done;

            /* Initialize monitors */
            for (monitor = 0; monitor < monitor_count; monitor++)
            {
                TRACE("monitor: %#x %s\n", monitor, wine_dbgstr_w(monitors[monitor].name));
                if (!BOXEDDRV_InitMonitor(monitor_devinfo, &monitors[monitor], monitor, video_index, &gpu_luid, output_id++))
                    goto done;
            }

            // handler->free_monitors(monitors);
            // monitors = NULL;
            video_index++;
        }

        // handler->free_adapters(adapters);
        // adapters = NULL;
    }

done:
    cleanup_devices();
    SetupDiDestroyDeviceInfoList(monitor_devinfo);
    SetupDiDestroyDeviceInfoList(gpu_devinfo);
    RegCloseKey(video_hkey);
    release_display_device_init_mutex(mutex);
    /*
    if (gpus)
        handler->free_gpus(gpus);
    if (adapters)
        handler->free_adapters(adapters);
    if (monitors)
        handler->free_monitors(monitors);
        */
}

BOOL WINE_CDECL boxeddrv_EnumDisplayMonitors(HDC hdc, LPRECT rect, MONITORENUMPROC proc, LPARAM lparam) {
    RECT r;
    r.left = 0;
    r.right = internal_GetDeviceCaps(HORZRES);
    r.top = 0;
    r.bottom = internal_GetDeviceCaps(VERTRES);

    TRACE("hdc=%p rect=%s proc=%p lparam=0x%08x\n", hdc, wine_dbgstr_rect(rect), proc, (int)lparam);
    if (hdc) {
        POINT origin;
        RECT limit;
        RECT monrect = r;

        if (!GetDCOrgEx(hdc, &origin)) return FALSE;
        if (GetClipBox(hdc, &limit) == ERROR) return FALSE;

        if (rect && !IntersectRect(&limit, &limit, rect)) return TRUE;

        if (IntersectRect(&monrect, &monrect, &limit)) {
            if (!proc((HMONITOR)1, hdc, &monrect, lparam))
                return FALSE;
        }
    }
    else {
        RECT monrect = r;
        RECT unused;

        if (!rect || IntersectRect(&unused, &monrect, rect)) {
            TRACE("calling proc hdc=%p monrect=%s proc=%p lparam=0x%08x\n", hdc, wine_dbgstr_rect(&monrect), proc, (int)lparam);
            if (!proc((HMONITOR)1, hdc, &monrect, lparam))
                return FALSE;
        }
    }

    return TRUE;
}

#endif

