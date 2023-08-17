#include <glib.h>
#include <cairo.h>


#if 0
#include "gdk_types.h"

const struct gdi_dc_funcs* CDECL broadwaydrv_get_gdi_driver(unsigned int version)
{
    int result;

    if (version != WINE_GDI_DRIVER_VERSION)
    {
        ERR("version mismatch, gdi32 wants %u but winebroadway has %u\n", version, WINE_GDI_DRIVER_VERSION);
        return NULL;
    }

    return &broadwaydrv_funcs.dc_funcs;
}

void BROADWAYDRV_ProcessAttach() {
    TRACE("calling set user driver\n");
    __wine_set_user_driver(&broadwaydrv_funcs, WINE_GDI_DRIVER_VERSION);
}

static inline BROADWAYDRV_PDEVICE* get_broadwaydrv_dev(PHYSDEV dev)
{
    return (BROADWAYDRV_PDEVICE*)dev;
}

static BROADWAYDRV_PDEVICE* create_broadway_physdev(void)
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(BROADWAYDRV_PDEVICE));
}

BOOL WINE_CDECL broadwaydrv_DeleteDC(PHYSDEV dev)
{
    BROADWAYDRV_PDEVICE* physDev = get_broadwaydrv_dev(dev);

    TRACE("hdc %p\n", dev->hdc);

    HeapFree(GetProcessHeap(), 0, physDev);
    return TRUE;
}

INT WINE_CDECL broadwaydrv_GetDeviceCaps(PHYSDEV dev, INT cap) {
    INT result;
    switch (cap) {
    case PDEVICESIZE:
        return sizeof(BROADWAYDRV_PDEVICE);
    }
    TRACE("BROADWAY_GET_DEVICE_CAPS=%d\n", BROADWAY_GET_DEVICE_CAPS);
    CALL_2(BROADWAY_GET_DEVICE_CAPS, dev, cap);
    TRACE("dev=%p cap=%d result=%d\n", dev, cap, result);
    return result;
}

INT WINE_CDECL internal_GetDeviceCaps(INT cap) {
    return broadwaydrv_GetDeviceCaps(NULL, cap);
}
#endif
