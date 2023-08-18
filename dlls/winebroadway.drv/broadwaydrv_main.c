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

#include "basetsd.h"
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "wine/server.h"
#include "wine/debug.h"
#include "wine/list.h"

#include <glib.h>
#include <gio/gunixsocketaddress.h>

#include <cairo.h>

#include <gtk/gtk.h>

#include "unixlib.h"

WINE_DEFAULT_DEBUG_CHANNEL(broadwaydrv);

struct _BroadwayServer {
  GObject parent_instance;

  guint32 next_serial;
  GSocketConnection *connection;

  guint32 recv_buffer_size;
  guint8 recv_buffer[1024];

  guint process_input_idle;
  GList *incomming;
};

typedef struct _BroadwayServer BroadwayServer;

BroadwayServer * wine_broadway_server_new (const char *display, GError **error);
guint32 wine_broadway_server_get_last_seen_time (BroadwayServer *server);
void wine_broadway_server_flush (BroadwayServer *server);
void wine_broadway_server_sync (BroadwayServer *server);
void wine_broadway_server_query_mouse (BroadwayServer *server,
                                  guint32            *toplevel,
                                  gint32             *root_x,
                                  gint32             *root_y,
                                  guint32            *mask);

guint32
wine_broadway_server_new_window (BroadwayServer *server,
                                 int x,
                                 int y,
                                 int width,
                                 int height,
                                 gboolean is_temp);


gboolean wine_broadway_server_window_show (BroadwayServer *server, gint id);

cairo_surface_t *
wine_broadway_server_create_surface (int width, int height);

void wine_broadway_server_window_update (BroadwayServer *server, gint id,
                                    cairo_surface_t *surface);

void test_connection(void)
{
    GdkDisplay *display;
    GtkWidget *window;
#if 0
    int argc;
    char **argv;

    gtk_init (&argc, &argv);

    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_widget_show (window);

    //dm = wine_broadway_device_manager_new (display);

    display = gdk_display_get_default ();

    gtk_main();
#endif
    GError *error;
    error = NULL;
    guint32 id;

    BroadwayServer *broadway_server;
    cairo_surface_t *surface;

    char *client_port;
    client_port = ":0";

    //dir = g_dir_open(target.c_str(), 0, &error);
    broadway_server = wine_broadway_server_new(client_port, &error);
    if( error != NULL )
    {
        //std::cout << error->message << std::endl;
        printf("Some error: %s", error-> message);
        g_clear_error (&error);
    }
    
    //printf("wine_broadway_server_new function did something: %s", error-> message);
    printf("wine_broadway_server_new function did something\n");

    id = wine_broadway_server_new_window(broadway_server,100,100,100,100,0);
    
    printf("wine_broadway_server_new_window function did something\n");

    wine_broadway_server_window_show (broadway_server, id);

    surface = wine_broadway_server_create_surface(100,100);

    wine_broadway_server_window_update (broadway_server, id, surface);

    wine_broadway_server_window_show (broadway_server, id);
    //return id;

}


static NTSTATUS WINAPI broadway_start_device(void *param, ULONG size);

/***********************************************************************
 *           BROADWAYDRV process initialisation routine
 */
static NTSTATUS broadwaydrv_init( void *arg )
{
    struct init_params *params = arg;

    //test_connection();
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
