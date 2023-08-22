/*
 * Mac graphics driver initialisation functions
 *
 * Copyright 1996 Alexandre Julliard
 * Copyright 2011, 2012, 2013 Ken Thomases for CodeWeavers, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#if 0
#pragma makedep unix
#endif

#include "config.h"

#include <stdlib.h>

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "ntgdi.h"
#include "wine/gdi_driver.h"

#include <pthread.h>

#include "broadwaydrv.h"

#include <glib.h>
#include <cairo.h>
#include <gdk/gdk.h>

#include "unixlib.h"
#include "broadway-server.h"
#include "broadwaydrv.h"

#include "wine/list.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(broadwaydrv);

unsigned int screen_width = 1024;
unsigned int screen_height = 768;
unsigned int screen_bpp = 32;
//static int palette_size;

cairo_surface_t *surface;
cairo_t *cr;
int initialization = 0;

typedef struct
{
    struct gdi_physdev  dev;
} BROADWAY_PDEVICE;

static inline BROADWAY_PDEVICE *get_broadwaydrv_dev(PHYSDEV dev)
{
    return (BROADWAY_PDEVICE*)dev;
}

//int retina_on = FALSE;
static pthread_mutex_t device_data_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_once_t init_once = PTHREAD_ONCE_INIT;

void init_recursive_mutex( pthread_mutex_t *mutex )
{
    pthread_mutexattr_t attr;

    pthread_mutexattr_init( &attr );
    pthread_mutexattr_settype( &attr, PTHREAD_MUTEX_RECURSIVE );
    pthread_mutex_init( mutex, &attr );
    pthread_mutexattr_destroy( &attr );
}

static const struct user_driver_funcs broadwaydrv_funcs;

#if 0
/***********************************************************************
 *              compute_desktop_rect
 */
static void compute_desktop_rect(void)
{
}

static BOOL intersect_rect( RECT *dst, const RECT *src1, const RECT *src2 )
{
    dst->left   = max(src1->left, src2->left);
    dst->top    = max(src1->top, src2->top);
    dst->right  = min(src1->right, src2->right);
    dst->bottom = min(src1->bottom, src2->bottom);
    return !IsRectEmpty( dst );
}
#endif

/**********************************************************************
 *              device_init
 *
 * Perform initializations needed upon creation of the first device.
 */
void device_init(void)
{
  FIXME("device_init - Created a 1024x768x32bpp window. This should work...\n");
  guint32 root_window_id;
  root_window_id = _wine_broadway_server_new_window (server,
                                                     0,0,
						     screen_width,
						     screen_height,
						     0);

  /* This code is for testing with a local cairo image as a virtual desktop */
#if 0
  FIXME("device_init - Created a 1024x768x32bpp window. This should work...\n");
  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, screen_width, screen_height);
#endif
  FIXME("device_init - We just created a surface here. This should work...\n");

  initialization = 1;

#if 0
  /* Fill the entire surface with red. */
  cairo_set_source_rgb(cr, 1, 0, 0);
  cairo_rectangle(cr, 0, 0, screen_width, screen_height);
  cairo_fill(cr);

  FIXME("device_init - And filled it with red. This is totally wrong\n");
#endif
}

#if 0
void macdrv_reset_device_metrics(void)
{
    pthread_mutex_lock(&device_data_mutex);
    device_data_valid = FALSE;
    pthread_mutex_unlock(&device_data_mutex);
}

#endif

static BROADWAY_PDEVICE *create_cairo_physdev(void)
{
    BROADWAY_PDEVICE *physDev;
    cairo_surface_t *surface;

    pthread_once( &init_once, device_init );

    if (!(physDev = calloc(1, sizeof(*physDev)))) return NULL;
#if 0
    /* This code is for testing with a local cairo surface */
    FIXME("create_cairo_physdev - We just create a surface here. This should work...\n");
    cr = cairo_create (surface);
#endif

    FIXME("create_cairo_physdev - Create a cairo surface on server. This should work...\n");
    surface = _wine_broadway_server_create_surface (screen_width, screen_height);

/*
    physDev->gc = XCreateGC( gdi_display, drawable, 0, NULL );
    XFlush( gdi_display );
*/

    FIXME("create_cairo_physdev - Returning physDev to the WinApi This should work...\n");
    return physDev;
}

/**********************************************************************
 *              CreateDC (MACDRV.@)
 */
BOOL BROADWAYDRV_CreateDC(PHYSDEV *pdev, LPCWSTR device, LPCWSTR output, const DEVMODEW* initData)
{
    BROADWAY_PDEVICE *physDev = create_cairo_physdev();

    FIXME("CreateDC - pdev %p hdc %p device %s output %s initData %p\n", pdev,
          (*pdev)->hdc, debugstr_w(device), debugstr_w(output), initData);

    if (!physDev)
    {
       FIXME("CreateDC - No physDev found, returning false\n");	   
       return FALSE;
    }

    push_dc_driver(pdev, &physDev->dev, &broadwaydrv_funcs.dc_funcs);
    FIXME("CreateDC - Pushed DC Driver, returning true\n");	   
    return TRUE;
}


/**********************************************************************
 *              CreateCompatibleDC (MACDRV.@)
 */
BOOL BROADWAYDRV_CreateCompatibleDC(PHYSDEV orig, PHYSDEV *pdev)
{
    BROADWAY_PDEVICE *physDev = create_cairo_physdev();

    FIXME("CreateCompatibleDC - orig %p orig->hdc %p pdev %p pdev->hdc %p\n", orig, (orig ? orig->hdc : NULL), pdev,
          ((pdev && *pdev) ? (*pdev)->hdc : NULL));

    if (!physDev)
    {
       FIXME("CreateCompatibleDC - No physDev found, returning false\n");	   
       return FALSE;
    }

    push_dc_driver(pdev, &physDev->dev, &broadwaydrv_funcs.dc_funcs);
    FIXME("CreateDC - Pushed DC Driver, returning true\n");	   
    return TRUE;
}


/**********************************************************************
 *              DeleteDC (MACDRV.@)
 */
BOOL BROADWAYDRV_DeleteDC(PHYSDEV dev)
{
    BROADWAY_PDEVICE *physDev = get_broadwaydrv_dev(dev);

    FIXME("DeleteDC hdc %p\n", dev->hdc);

    free(physDev);
    return TRUE;
}


/***********************************************************************
 *              GetDeviceCaps (MACDRV.@)
 */
INT BROADWAYDRV_GetDeviceCaps(PHYSDEV dev, INT cap)
{
    INT ret;

    //pthread_mutex_lock(&device_data_mutex);

    if (!initialization)
	 device_init();

    switch(cap)
    {
//    case SIZEPALETTE:
//        return palette_size;
    case HORZSIZE:
        ret = screen_width;
        break;
    case VERTSIZE:
        ret = screen_height;
        break;
    case BITSPIXEL:
        ret = screen_bpp;
        break;
    case HORZRES:
    case VERTRES:
    default:
        //pthread_mutex_unlock(&device_data_mutex);
        dev = GET_NEXT_PHYSDEV( dev, pGetDeviceCaps );
        ret = dev->funcs->pGetDeviceCaps( dev, cap );
        return ret;
    }

    FIXME("GetDeviceCaps cap %d -> %d\n", cap, ret);

    //pthread_mutex_unlock(&device_data_mutex);
    return ret;
}


