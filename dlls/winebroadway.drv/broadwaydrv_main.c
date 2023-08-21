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

#include "unixlib.h"
#include "broadway-server.h"
#include "broadwaydrv.h"

BroadwayServer *server;
void init_user_driver(void);

WINE_DEFAULT_DEBUG_CHANNEL(broadwaydrv);

void init_broadway_connection(void)
{
    GError *error;
    error = NULL;

    const char *client_port;
    client_port = ":0";

    //dir = g_dir_open(target.c_str(), 0, &error);
    server = _wine_broadway_server_new(client_port, &error);
    if( error != NULL )
    {
        //std::cout << error->message << std::endl;
        printf("Some error: %s", error-> message);
        g_clear_error (&error);
    }

    if( server == NULL )
        FIXME("init_broadway_connection - Some other error\n");
}

/***********************************************************************
 *           BROADWAYDRV process initialisation routine
 */
static NTSTATUS broadwaydrv_init( void *arg )
{
    struct init_params *params = arg;

    FIXME("broadwaydrv_init - about to start user driver\n");
    init_broadway_connection();

    init_user_driver();

    FIXME("broadwaydrv_init - we made it\n");
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
        ULONG foreign_window_proc;
        ULONG show_systray;
    } *params32 = arg;
    struct init_params params;

    params.foreign_window_proc = UlongToPtr( params32->foreign_window_proc );
    params.show_systray = UlongToPtr( params32->show_systray );
    return broadwaydrv_init( &params );
}

const unixlib_entry_t __wine_unix_call_wow64_funcs[] =
{
    broadwaydrv_wow64_init,
};

C_ASSERT( ARRAYSIZE(__wine_unix_call_wow64_funcs) == unix_funcs_count );

#endif /* _WIN64 */
