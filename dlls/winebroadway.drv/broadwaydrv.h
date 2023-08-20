#ifndef BROADWAYDRV_H
#define BROADWAYDRV_H

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

/* a few dynamic device caps */
//int bits_per_pixel;      /* pixel depth of screen */
//int device_data_valid;   /* do the above variables have up-to-date values? */

extern unsigned int screen_width;
extern unsigned int screen_height;

/* broadwaydrv private window data */
struct broadwaydrv_thread_data
{
    GObject                parent_instance;
    guint8                  *display;
    guint8                 id;
    //XEvent              *current_event;        /* event currently being processed */
    HWND                  grab_hwnd;            /* window that currently grabs the mouse */
    HWND                  last_focus;           /* last window that had focus */
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

struct broadwaydrv_win_data
{
    GObject             parent_instance;      
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
    //GdkGeometry         geometry_hints;
    //GdkWindowHints      geometry_hints_mask;
};

extern struct broadwaydrv_win_data *get_win_data(HWND hwnd) DECLSPEC_HIDDEN;
extern void release_win_data(struct broadwaydrv_win_data *data) DECLSPEC_HIDDEN;
extern void init_win_context(void) DECLSPEC_HIDDEN;
extern RGNDATA *get_region_data(HRGN hrgn, HDC hdc_lptodp) DECLSPEC_HIDDEN;

#endif /* BROADWAYDRV_H */
