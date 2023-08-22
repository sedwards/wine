#ifndef BROADWAYDRV_H
#define BROADWAYDRV_H

#include <limits.h>
#include <stdarg.h>
#include <stdlib.h>
#include <pthread.h>

#include "windef.h"
#include "winbase.h"
#include "ntgdi.h"
#include "wine/gdi_driver.h"
#include "unixlib.h"
#include "wine/list.h"
#include "wine/debug.h"

#include <glib.h>
#include <gio/gunixsocketaddress.h>
#include <cairo.h>

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

/* Global variable for our connection to the server */
extern BroadwayServer *server;

/* Broadway Display we are running on */
/* Set something like BROADWAY_DISPLAY=:0 */
extern const char *display;

typedef struct _Display {
    int fd;                          /* network socket */
    char *display_name;              /* "host:display" string used on this connect*/
} Display;

/* a few dynamic device caps */
//int bits_per_pixel;      /* pixel depth of screen */
//int device_data_valid;   /* do the above variables have up-to-date values? */

extern unsigned int screen_width;
extern unsigned int screen_height;

extern RECT virtual_screen_rect;// = { 0, 0, 0, 0 };

/* broadwaydrv private window data */
struct broadwaydrv_thread_data
{
    GObject                parent_instance;
    BroadwayServer         *server;
    char                   *display;
    guint8                 id;
    //XEvent               *current_event;        /* event currently being processed */
    HWND                   grab_hwnd;            /* window that currently grabs the mouse */
    HWND                   last_focus;           /* last window that had focus */
    //XFontSet font_set;             /* international text drawing font set */
    guint8                 selection_wnd;        /* window used for selection interactions */
    guint8                 clip_window;          /* window used for cursor clipping */
    BOOL                   clipping_cursor;      /* whether thread is currently clipping the cursor */
};

extern struct broadwaydrv_thread_data *broadwaydrv_init_thread_data(void) DECLSPEC_HIDDEN;

static inline struct broadwaydrv_thread_data *broadwaydrv_thread_data(void)
{
    return (struct broadwaydrv_thread_data *)(UINT_PTR)NtUserGetThreadInfo()->driver_data;
}

/* retrieve the thread display, or NULL if not created yet */
static inline Display *thread_display(void)
{
    struct broadwaydrv_thread_data *data = broadwaydrv_thread_data();
    if (!data) return NULL;
    return data->display;
}

/* retrieve the thread display, creating it if needed */
static inline Display *thread_init_display(void)
{
    return broadwaydrv_init_thread_data()->display;
}

extern struct broadway_win_data *get_win_data(HWND hwnd) DECLSPEC_HIDDEN;
extern void release_win_data(struct broadway_win_data *data) DECLSPEC_HIDDEN;
extern void init_win_context(void) DECLSPEC_HIDDEN;
extern RGNDATA *get_region_data(HRGN hrgn, HDC hdc_lptodp) DECLSPEC_HIDDEN;

struct broadwaydrv_window_surface
{
    struct window_surface   header;
    HWND                    window;
    RECT                    bounds;
    HRGN                    region;
    HRGN                    drawn;
    BOOL                    use_alpha;
    RGNDATA                *blit_data;
    BYTE                   *bits;
    pthread_mutex_t         mutex;
    BITMAPINFO              info;   /* variable size, must be last */
};


/* Proposed Window Structure Object */
struct _BroadwayWindow
{
    GObject             parent_instance;
    char                *display;
    HWND                *wrapper;                   /* hwnd that this private data belongs to */
    HWND                *screen;                   /* hwnd that this private data belongs to */
    RECT                window_rect;            /* USER window rectangle relative to parent */
    RECT                whole_rect;             /* Mac window rectangle for the whole window relative to parent */
    RECT                client_rect;            /* client area relative to parent */
    cairo_surface_t     *surface;
    cairo_surface_t     *last_surface;
    cairo_surface_t     *ref_surface;
    int                 id;
    BOOL                visible;
    BOOL                maximized;
    int                 transient_for;
    int                 pre_maximize_x;
    int                 pre_maximize_y;
    int                 pre_maximize_width;
    int                 pre_maximize_height;
    gint8               toplevel_window_type;
    BOOL                dirty;
    BOOL                last_synced;
    //XEvent              *current_event;        /* event currently being processed */
    HWND                  grab_hwnd;            /* window that currently grabs the mouse */
    HWND                  last_focus;           /* last window that had focus */
    //XFontSet font_set;             /* international text drawing font set */
    HWND                 selection_wnd;        /* window used for selection interactions */
    HWND                 clip_window;          /* window used for cursor clipping */
};
typedef struct _BroadwayWindow BroadwayWindow;


/* Event format for port */

enum event_type
{
    DESKTOP_CHANGED,
    CONFIG_CHANGED,
    SURFACE_CHANGED,
    MOTION_EVENT,
    KEYBOARD_EVENT,
};

union event_data
{
    enum event_type type;
    struct
    {
        enum event_type type;
        unsigned int    width;
        unsigned int    height;
    } desktop;
    struct
    {
        enum event_type type;
        unsigned int    dpi;
    } cfg;
    struct
    {
        enum event_type type;
        HWND            hwnd;
        BroadwayWindow  *window;
        BOOL            client;
        unsigned int    width;
        unsigned int    height;
    } surface;
    struct
    {
        enum event_type type;
        HWND            hwnd;
        INPUT           input;
    } motion;
    struct
    {
        enum event_type type;
        HWND            hwnd;
        UINT            lock_state;
        INPUT           input;
    } kbd;
};

int send_event( const union event_data *data ) DECLSPEC_HIDDEN;
static HWND capture_window;
extern HWND get_capture_window(void) DECLSPEC_HIDDEN;
extern void init_recursive_mutex( pthread_mutex_t *mutex ) DECLSPEC_HIDDEN;

#define DEPTH_COUNT 3
extern const unsigned int *depths DECLSPEC_HIDDEN;

static inline void mirror_rect( const RECT *window_rect, RECT *rect )
{
    int width = window_rect->right - window_rect->left;
    int tmp = rect->left;
    rect->left = width - rect->right;
    rect->right = width - tmp;
}

static inline BOOL is_window_rect_mapped( const RECT *rect )
{
    RECT virtual_rect = NtUserGetVirtualScreenRect();
    return (rect->left < virtual_rect.right &&
            rect->top < virtual_rect.bottom &&
            max( rect->right, rect->left + 1 ) > virtual_rect.left &&
            max( rect->bottom, rect->top + 1 ) > virtual_rect.top);
}

#endif /* BROADWAYDRV_H */
