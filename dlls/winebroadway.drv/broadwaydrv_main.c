#if 0
#pragma makedep unix
#endif

#include "config.h"

#include <fcntl.h>
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

//#include "broadwaydrv.h"
//#include "winreg.h"
#include "wine/server.h"
#include "wine/debug.h"
#include "wine/list.h"

#include "unixlib.h"

WINE_DEFAULT_DEBUG_CHANNEL(broadwaydrv);

static NTSTATUS WINAPI broadway_start_device(void *param, ULONG size);

/***********************************************************************
 *           BROADWAYDRV process initialisation routine
 */
static NTSTATUS broadwaydrv_init( void *arg )
{
    struct init_params *params = arg;
#if 0
    Display *display;
    void *libx11 = dlopen( SONAME_LIBX11, RTLD_NOW|RTLD_GLOBAL );

    if (!libx11)
    {
        ERR( "failed to load %s: %s\n", SONAME_LIBX11, dlerror() );
        return STATUS_UNSUCCESSFUL;
    }
    pXGetEventData = dlsym( libx11, "XGetEventData" );
    pXFreeEventData = dlsym( libx11, "XFreeEventData" );
#ifdef SONAME_LIBXEXT
    dlopen( SONAME_LIBXEXT, RTLD_NOW|RTLD_GLOBAL );
#endif

    setup_options();

    /* Open display */

    client_foreign_window_proc = params->foreign_window_proc;

    fcntl( ConnectionNumber(display), F_SETFD, 1 ); /* set close on exec flag */
    root_window = DefaultRootWindow( display );
    gdi_display = display;
    old_error_handler = XSetErrorHandler( error_handler );

    init_pixmap_formats( display );
    init_visuals( display, DefaultScreen( display ));
    screen_bpp = pixmap_formats[default_visual.depth]->bits_per_pixel;

    init_win_context();

    if (TRACE_ON(synchronous)) XSynchronize( display, True );

    xinerama_init( DisplayWidth( display, default_visual.screen ),
                   DisplayHeight( display, default_visual.screen ));
    X11DRV_Settings_Init();

    /* initialize XVidMode */
    X11DRV_XF86VM_Init();
    /* initialize XRandR */
    X11DRV_XRandR_Init();
#ifdef SONAME_LIBXCOMPOSITE
    X11DRV_XComposite_Init();
#endif
    X11DRV_XInput2_Init();

    XkbUseExtension( gdi_display, NULL, NULL );
    X11DRV_InitKeyboard( gdi_display );

    init_user_driver();
    X11DRV_DisplayDevices_Init(FALSE);
    *params->show_systray = show_systray;
#endif

    init_user_driver();
    //X11DRV_DisplayDevices_Init(FALSE);
    ERR("broadwaydrv_init - we made it\n");
    return STATUS_SUCCESS;
}


NTSTATUS broadwaydrv_client_func( enum broadwaydrv_client_funcs id, const void *params, ULONG size )
{
    void *ret_ptr;
    ULONG ret_len;
    return KeUserModeCallback( id, params, size, &ret_ptr, &ret_len );
}


NTSTATUS broadwaydrv_client_call( enum client_callback func, UINT arg )
{
    struct client_callback_params params = { .id = func, .arg = arg };
    return broadwaydrv_client_func( client_func_callback, &params, sizeof(params) );
}

const unixlib_entry_t __wine_unix_call_funcs[] =
{
    broadwaydrv_init,
};

C_ASSERT( ARRAYSIZE(__wine_unix_call_funcs) == unix_funcs_count );

#ifdef _WIN64

static NTSTATUS broadwaydrv_wow64_init( void *arg )
{
    struct
    {
        //ULONG foreign_window_proc;
        //ULONG show_systray;
    } *params32 = arg;
    struct init_params params;

    //params.foreign_window_proc = UlongToPtr( params32->foreign_window_proc );
    //params.show_systray = UlongToPtr( params32->show_systray );
    return broadwaydrv_init( &params );
}

const unixlib_entry_t __wine_unix_call_wow64_funcs[] =
{
    broadwaydrv_wow64_init,
};

C_ASSERT( ARRAYSIZE(__wine_unix_call_wow64_funcs) == unix_funcs_count );

#endif /* _WIN64 */

