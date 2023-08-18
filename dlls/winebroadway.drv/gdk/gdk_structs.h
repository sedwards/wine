#ifndef _GDK_STRUCTS_H
#define _GDK_STRUCTS_H

/* Actually, start with some types first */

/* Public 
typedef enum
{
  GDK_GRAB_SUCCESS         = 0,
  GDK_GRAB_ALREADY_GRABBED = 1,
  GDK_GRAB_INVALID_TIME    = 2,
  GDK_GRAB_NOT_VIEWABLE    = 3,
  GDK_GRAB_FROZEN          = 4,
  GDK_GRAB_FAILED          = 5
} GdkGrabStatus;

typedef enum
{
  GDK_SHIFT_MASK    = 1 << 0,
  GDK_LOCK_MASK     = 1 << 1,
  GDK_CONTROL_MASK  = 1 << 2,
  GDK_MOD1_MASK     = 1 << 3,
  GDK_MOD2_MASK     = 1 << 4,
  GDK_MOD3_MASK     = 1 << 5,
  GDK_MOD4_MASK     = 1 << 6,
  GDK_MOD5_MASK     = 1 << 7,
  GDK_BUTTON1_MASK  = 1 << 8,
  GDK_BUTTON2_MASK  = 1 << 9,
  GDK_BUTTON3_MASK  = 1 << 10,
  GDK_BUTTON4_MASK  = 1 << 11,
  GDK_BUTTON5_MASK  = 1 << 12,

  GDK_MODIFIER_RESERVED_13_MASK  = 1 << 13,
  GDK_MODIFIER_RESERVED_14_MASK  = 1 << 14,
  GDK_MODIFIER_RESERVED_15_MASK  = 1 << 15,
  GDK_MODIFIER_RESERVED_16_MASK  = 1 << 16,
  GDK_MODIFIER_RESERVED_17_MASK  = 1 << 17,
  GDK_MODIFIER_RESERVED_18_MASK  = 1 << 18,
  GDK_MODIFIER_RESERVED_19_MASK  = 1 << 19,
  GDK_MODIFIER_RESERVED_20_MASK  = 1 << 20,
  GDK_MODIFIER_RESERVED_21_MASK  = 1 << 21,
  GDK_MODIFIER_RESERVED_22_MASK  = 1 << 22,
  GDK_MODIFIER_RESERVED_23_MASK  = 1 << 23,
  GDK_MODIFIER_RESERVED_24_MASK  = 1 << 24,
  GDK_MODIFIER_RESERVED_25_MASK  = 1 << 25,
*/
  /* The next few modifiers are used by XKB, so we skip to the end.
   * Bits 15 - 25 are currently unused. Bit 29 is used internally.
   */
/*
  GDK_SUPER_MASK    = 1 << 26,
  GDK_HYPER_MASK    = 1 << 27,
  GDK_META_MASK     = 1 << 28,

  GDK_MODIFIER_RESERVED_29_MASK  = 1 << 29,

  GDK_RELEASE_MASK  = 1 << 30,
*/
  /* Combination of GDK_SHIFT_MASK..GDK_BUTTON5_MASK + GDK_SUPER_MASK
     + GDK_HYPER_MASK + GDK_META_MASK + GDK_RELEASE_MASK */
/*  GDK_MODIFIER_MASK = 0x5c001fff
} GdkModifierType;
*/

#define GDK_TYPE_DEVICE         (gdk_device_get_type ())
#define GDK_DEVICE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GDK_TYPE_DEVICE, GdkDevice))
#define GDK_IS_DEVICE(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GDK_TYPE_DEVICE))

typedef struct _GdkDeviceClass GdkDeviceClass;
typedef struct _GdkDeviceKey GdkDeviceKey;
/*
struct _GdkDeviceKey
{
  guint keyval;
  GdkModifierType modifiers;
};

 Public
typedef enum {
  GDK_DEVICE_TOOL_TYPE_UNKNOWN,
  GDK_DEVICE_TOOL_TYPE_PEN,
  GDK_DEVICE_TOOL_TYPE_ERASER,
  GDK_DEVICE_TOOL_TYPE_BRUSH,
  GDK_DEVICE_TOOL_TYPE_PENCIL,
  GDK_DEVICE_TOOL_TYPE_AIRBRUSH,
  GDK_DEVICE_TOOL_TYPE_MOUSE,
  GDK_DEVICE_TOOL_TYPE_LENS,
} GdkDeviceToolType;

typedef enum
{
  GDK_AXIS_FLAG_X        = 1 << GDK_AXIS_X,
  GDK_AXIS_FLAG_Y        = 1 << GDK_AXIS_Y,
  GDK_AXIS_FLAG_PRESSURE = 1 << GDK_AXIS_PRESSURE,
  GDK_AXIS_FLAG_XTILT    = 1 << GDK_AXIS_XTILT,
  GDK_AXIS_FLAG_YTILT    = 1 << GDK_AXIS_YTILT,
  GDK_AXIS_FLAG_WHEEL    = 1 << GDK_AXIS_WHEEL,
  GDK_AXIS_FLAG_DISTANCE = 1 << GDK_AXIS_DISTANCE,
  GDK_AXIS_FLAG_ROTATION = 1 << GDK_AXIS_ROTATION,
  GDK_AXIS_FLAG_SLIDER   = 1 << GDK_AXIS_SLIDER,
} GdkAxisFlags;
*/

struct _GdkDeviceTool
{
  GObject parent_instance;
  guint64 serial;
  guint64 hw_id;
  GdkDeviceToolType type;
  GdkAxisFlags tool_axes;
};

/* Public
struct _GdkSeat
{
  GObject parent_instance;
};

typedef enum {
  GDK_RENDERING_MODE_SIMILAR = 0,
  GDK_RENDERING_MODE_IMAGE,
  GDK_RENDERING_MODE_RECORDING
} GdkRenderingMode;
*/

#if 0
/* Tracks information about the device grab on this display */
typedef struct
{
  GdkWindow *window;
  GdkWindow *native_window;
  gulong serial_start;
  gulong serial_end; /* exclusive, i.e. not active on serial_end */
  guint event_mask;
  guint32 time;
  GdkGrabOwnership ownership;

  guint activated : 1;
  guint implicit_ungrab : 1;
  guint owner_events : 1;
  guint implicit : 1;
} GdkDeviceGrabInfo;
#endif

typedef enum {
  GDK_RENDERING_MODE_SIMILAR = 0,
  GDK_RENDERING_MODE_IMAGE,
  GDK_RENDERING_MODE_RECORDING
} GdkRenderingMode;

typedef struct _GdkDisplay GdkDisplay;
struct _GdkDisplay
{
  GObject parent_instance;

  GList *queued_events;
  GList *queued_tail;

  /* Information for determining if the latest button click
   * is part of a double-click or triple-click
   */
  GHashTable *multiple_click_info;

  guint event_pause_count;       /* How many times events are blocked */

  guint closed             : 1;  /* Whether this display has been closed */

  GArray *touch_implicit_grabs;
  GHashTable *device_grabs;
  GHashTable *motion_hint_info;
  GdkDeviceManager *device_manager;
  GList *input_devices; /* Deprecated, only used to keep gdk_display_list_devices working */

  GHashTable *pointers_info;  /* GdkPointerWindowInfo for each device */
  guint32 last_event_time;    /* Last reported event time from server */

  guint double_click_time;  /* Maximum time between clicks in msecs */
  guint double_click_distance;   /* Maximum distance between clicks in pixels */

  guint has_gl_extension_texture_non_power_of_two : 1;
  guint has_gl_extension_texture_rectangle : 1;

  guint debug_updates     : 1;
  guint debug_updates_set : 1;

  GdkRenderingMode rendering_mode;

  GList *seats;
};

typedef struct _GdkDisplayClass GdkDisplayClass;

struct _GdkDisplayClass
{
  GObjectClass parent_class;

  GType window_type;          /* type for native windows for this display, set in class_init */
  /* Stub a lot missing */
};

#define GDK_DEVICE_CLASS(c)     (G_TYPE_CHECK_CLASS_CAST ((c), GDK_TYPE_DEVICE, GdkDeviceClass))
#define GDK_IS_DEVICE_CLASS(c)  (G_TYPE_CHECK_CLASS_TYPE ((c), GDK_TYPE_DEVICE))
#define GDK_DEVICE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GDK_TYPE_DEVICE, GdkDeviceClass))

typedef struct _GdkDeviceClass GdkDeviceClass;
typedef struct _GdkDeviceKey GdkDeviceKey;

#if 0
struct _GdkDevice
{
  GObject parent_instance;

  gchar *name;
  GdkInputSource source;
  GdkInputMode mode;
  gboolean has_cursor;
  gint num_keys;
  GdkAxisFlags axis_flags;
  GdkDeviceKey *keys;
  GdkDeviceManager *manager;
  GdkDisplay *display;
  /* Paired master for master,
   * associated master for slaves
   */
  GdkDevice *associated;
  GList *slaves;
  GdkDeviceType type;
  GArray *axes;
  guint num_touches;

  gchar *vendor_id;
  gchar *product_id;

  GdkSeat *seat;
  GdkDeviceTool *last_tool;
};

struct _GdkDeviceClass
{
  GObjectClass parent_class;

  gboolean (* get_history)   (GdkDevice      *device,
                              GdkWindow      *window,
                              guint32         start,
                              guint32         stop,
                              GdkTimeCoord ***events,
                              gint           *n_events);

  void (* get_state)         (GdkDevice       *device,
                              GdkWindow       *window,
                              gdouble         *axes,
                              GdkModifierType *mask);

  void (* set_window_cursor) (GdkDevice *device,
                              GdkWindow *window,
                              GdkCursor *cursor);

  void (* warp)              (GdkDevice  *device,
                              GdkScreen  *screen,
                              gdouble     x,
                              gdouble     y);
  void (* query_state)       (GdkDevice       *device,
                              GdkWindow       *window,
                              GdkWindow      **root_window,
                              GdkWindow      **child_window,
                              gdouble          *root_x,
                              gdouble          *root_y,
                              gdouble          *win_x,
                              gdouble          *win_y,
                              GdkModifierType  *mask);
  GdkGrabStatus (* grab)     (GdkDevice        *device,
                              GdkWindow        *window,
                              gboolean          owner_events,
                              GdkEventMask      event_mask,
                              GdkWindow        *confine_to,
                              GdkCursor        *cursor,
                              guint32           time_);
  void          (*ungrab)    (GdkDevice        *device,
                              guint32           time_);

  GdkWindow * (* window_at_position) (GdkDevice       *device,
                                      double          *win_x,
                                      double          *win_y,
                                      GdkModifierType *mask,
                                      gboolean         get_toplevel);
  void (* select_window_events)      (GdkDevice       *device,
                                      GdkWindow       *window,
                                      GdkEventMask     event_mask);
};
#endif

#define GDK_DEVICE_MANAGER_CLASS(c)     (G_TYPE_CHECK_CLASS_CAST ((c), GDK_TYPE_DEVICE_MANAGER, GdkDeviceManagerClass))
#define GDK_IS_DEVICE_MANAGER_CLASS(c)  (G_TYPE_CHECK_CLASS_TYPE ((c), GDK_TYPE_DEVICE_MANAGER))
#define GDK_DEVICE_MANAGER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GDK_TYPE_DEVICE_MANAGER, GdkDeviceManagerClass))


typedef struct _GdkDeviceManagerClass GdkDeviceManagerClass;

struct _GdkDeviceManager
{
  GObject parent_instance;

  /*< private >*/
  GdkDisplay *display;
};

struct _GdkDeviceManagerClass
{
  GObjectClass parent_class;

  /* Signals */
  void (* device_added)   (GdkDeviceManager *device_manager,
                           GdkDevice        *device);
  void (* device_removed) (GdkDeviceManager *device_manager,
                           GdkDevice        *device);
  void (* device_changed) (GdkDeviceManager *device_manager,
                           GdkDevice        *device);

  /* VMethods */
  GList *     (* list_devices)       (GdkDeviceManager *device_manager,
                                      GdkDeviceType     type);
  GdkDevice * (* get_client_pointer) (GdkDeviceManager *device_manager);
};

/* Public
typedef enum
{
  GDK_X_CURSOR            = 0,
  GDK_ARROW               = 2,
  GDK_BASED_ARROW_DOWN    = 4,
  GDK_BASED_ARROW_UP      = 6,
  GDK_BOAT                = 8,
  GDK_BOGOSITY            = 10,
  GDK_BOTTOM_LEFT_CORNER  = 12,
  GDK_BOTTOM_RIGHT_CORNER = 14,
  GDK_BOTTOM_SIDE         = 16,
  GDK_BOTTOM_TEE          = 18,
  GDK_BOX_SPIRAL          = 20,
  GDK_CENTER_PTR          = 22,
  GDK_CIRCLE              = 24,
  GDK_CLOCK               = 26,
  GDK_COFFEE_MUG          = 28,
  GDK_CROSS               = 30,
  GDK_CROSS_REVERSE       = 32,
  GDK_CROSSHAIR           = 34,
  GDK_DIAMOND_CROSS       = 36,
  GDK_DOT                 = 38,
  GDK_DOTBOX              = 40,
  GDK_DOUBLE_ARROW        = 42,
  GDK_DRAFT_LARGE         = 44,
  GDK_DRAFT_SMALL         = 46,
  GDK_DRAPED_BOX          = 48,
  GDK_EXCHANGE            = 50,
  GDK_FLEUR               = 52,
  GDK_GOBBLER             = 54,
  GDK_GUMBY               = 56,
  GDK_GUMBY               = 56,
  GDK_HAND1               = 58,
  GDK_HAND2               = 60,
  GDK_HEART               = 62,
  GDK_ICON                = 64,
  GDK_IRON_CROSS          = 66,
  GDK_LEFT_PTR            = 68,
  GDK_LEFT_SIDE           = 70,
  GDK_LEFT_TEE            = 72,
  GDK_LEFTBUTTON          = 74,
  GDK_LL_ANGLE            = 76,
  GDK_LR_ANGLE            = 78,
  GDK_MAN                 = 80,
  GDK_MIDDLEBUTTON        = 82,
  GDK_MOUSE               = 84,
  GDK_PENCIL              = 86,
  GDK_PIRATE              = 88,
  GDK_PLUS                = 90,
  GDK_QUESTION_ARROW      = 92,
  GDK_RIGHT_PTR           = 94,
  GDK_RIGHT_SIDE          = 96,
  GDK_RIGHT_TEE           = 98,
  GDK_RIGHTBUTTON         = 100,
  GDK_RTL_LOGO            = 102,
  GDK_SAILBOAT            = 104,
  GDK_SB_DOWN_ARROW       = 106,
  GDK_SB_H_DOUBLE_ARROW   = 108,
  GDK_SB_LEFT_ARROW       = 110,
  GDK_SB_RIGHT_ARROW      = 112,
  GDK_SB_UP_ARROW         = 114,
  GDK_SB_V_DOUBLE_ARROW   = 116,
  GDK_SHUTTLE             = 118,
  GDK_SIZING              = 120,
  GDK_SPIDER              = 122,
  GDK_SPRAYCAN            = 124,
  GDK_STAR                = 126,
  GDK_TARGET              = 128,
  GDK_TCROSS              = 130,
  GDK_TOP_LEFT_ARROW      = 132,
  GDK_TOP_LEFT_CORNER     = 134,
  GDK_TOP_RIGHT_CORNER    = 136,
  GDK_TOP_SIDE            = 138,
  GDK_TOP_TEE             = 140,
  GDK_TREK                = 142,
  GDK_UL_ANGLE            = 144,
  GDK_UMBRELLA            = 146,
  GDK_UR_ANGLE            = 148,
  GDK_WATCH               = 150,
  GDK_XTERM               = 152,
  GDK_LAST_CURSOR,
  GDK_BLANK_CURSOR        = -2,
  GDK_CURSOR_IS_PIXMAP    = -1
} GdkCursorType;
*/

#define GDK_CURSOR_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GDK_TYPE_CURSOR, GdkCursorClass))
#define GDK_IS_CURSOR_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GDK_TYPE_CURSOR))
#define GDK_CURSOR_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), GDK_TYPE_CURSOR, GdkCursorClass))

typedef struct _GdkCursorClass GdkCursorClass;

struct _GdkCursor
{
  GObject parent_instance;

  GdkDisplay *display;
  GdkCursorType type;
};

struct _GdkCursorClass
{
  GObjectClass parent_class;

  cairo_surface_t * (* get_surface) (GdkCursor *cursor,
                                     gdouble   *x_hot,
                                     gdouble   *y_hot);
};

/* dkwindowimpl.h */
typedef enum
{
  GDK_TITLEBAR_GESTURE_DOUBLE_CLICK   = 1,
  GDK_TITLEBAR_GESTURE_RIGHT_CLICK    = 2,
  GDK_TITLEBAR_GESTURE_MIDDLE_CLICK   = 3
} GdkTitlebarGesture;

typedef struct _GdkWindowClass GdkWindowClass;

#define GDK_TYPE_WINDOW_IMPL           (gdk_window_impl_get_type ())
#define GDK_WINDOW_IMPL(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), GDK_TYPE_WINDOW_IMPL, GdkWindowImpl))
#define GDK_WINDOW_IMPL_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GDK_TYPE_WINDOW_IMPL, GdkWindowImplClass))
#define GDK_IS_WINDOW_IMPL(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), GDK_TYPE_WINDOW_IMPL))
#define GDK_IS_WINDOW_IMPL_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GDK_TYPE_WINDOW_IMPL))
#define GDK_WINDOW_IMPL_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), GDK_TYPE_WINDOW_IMPL, GdkWindowImplClass))

typedef struct _GdkWindowImpl       GdkWindowImpl;
typedef struct _GdkWindowImplClass  GdkWindowImplClass;

struct _GdkWindowImpl
{
  GObject parent;
};

struct _GdkWindowImplClass
{
  GObjectClass parent_class;

  cairo_surface_t *
               (* ref_cairo_surface)    (GdkWindow       *window);
  cairo_surface_t *
               (* create_similar_image_surface) (GdkWindow *     window,
                                                 cairo_format_t  format,
                                                 int             width,
                                                 int             height);

  void         (* show)                 (GdkWindow       *window,
                                         gboolean         already_mapped);
  void         (* hide)                 (GdkWindow       *window);
  void         (* withdraw)             (GdkWindow       *window);
  void         (* raise)                (GdkWindow       *window);
  void         (* lower)                (GdkWindow       *window);
  void         (* restack_under)        (GdkWindow       *window,
                                         GList           *native_siblings);
  void         (* restack_toplevel)     (GdkWindow       *window,
                                         GdkWindow       *sibling,
                                         gboolean        above);

  void         (* move_resize)          (GdkWindow       *window,
                                         gboolean         with_move,
                                         gint             x,
                                         gint             y,
                                         gint             width,
                                         gint             height);

  void         (* move_to_rect)         (GdkWindow       *window,
                                         const GdkRectangle *rect,
                                         GdkGravity       rect_anchor,
                                         GdkGravity       window_anchor,
                                         GdkAnchorHints   anchor_hints,
                                         gint             rect_anchor_dx,
                                         gint             rect_anchor_dy);
  void         (* set_background)       (GdkWindow       *window,
                                         cairo_pattern_t *pattern);

  GdkEventMask (* get_events)           (GdkWindow       *window);
  void         (* set_events)           (GdkWindow       *window,
                                         GdkEventMask     event_mask);

  gboolean     (* reparent)             (GdkWindow       *window,
                                         GdkWindow       *new_parent,
                                         gint             x,
                                         gint             y);

  void         (* set_device_cursor)    (GdkWindow       *window,
                                         GdkDevice       *device,
                                         GdkCursor       *cursor);

  void         (* get_geometry)         (GdkWindow       *window,
                                         gint            *x,
                                         gint            *y,
                                         gint            *width,
                                         gint            *height);
  void         (* get_root_coords)      (GdkWindow       *window,
                                         gint             x,
                                         gint             y,
                                         gint            *root_x,
                                         gint            *root_y);
  gboolean     (* get_device_state)     (GdkWindow       *window,
                                         GdkDevice       *device,
                                         gdouble         *x,
                                         gdouble         *y,
                                         GdkModifierType *mask);
  gboolean    (* begin_paint)           (GdkWindow       *window);
  void        (* end_paint)             (GdkWindow       *window);

  cairo_region_t * (* get_shape)        (GdkWindow       *window);
  cairo_region_t * (* get_input_shape)  (GdkWindow       *window);
  void         (* shape_combine_region) (GdkWindow       *window,
                                         const cairo_region_t *shape_region,
                                         gint             offset_x,
                                         gint             offset_y);
  void         (* input_shape_combine_region) (GdkWindow       *window,
                                               const cairo_region_t *shape_region,
                                               gint             offset_x,
                                               gint             offset_y);
  /* Called before processing updates for a window. This gives the windowing
   * layer a chance to save the region for later use in avoiding duplicate
   * exposes.
   */
  void     (* queue_antiexpose)     (GdkWindow       *window,
                                     cairo_region_t  *update_area);

/* Called to do the windowing system specific part of gdk_window_destroy(),
 *
 * window: The window being destroyed
 * recursing: If TRUE, then this is being called because a parent
 *     was destroyed. This generally means that the call to the windowing
 *     system to destroy the window can be omitted, since it will be
 *     destroyed as a result of the parent being destroyed.
 *     Unless @foreign_destroy
 * foreign_destroy: If TRUE, the window or a parent was destroyed by some
 *     external agency. The window has already been destroyed and no
 *     windowing system calls should be made. (This may never happen
 *     for some windowing systems.)
 */
  void         (* destroy)              (GdkWindow       *window,
                                         gboolean         recursing,
                                         gboolean         foreign_destroy);


 /* Called when gdk_window_destroy() is called on a foreign window
  * or an ancestor of the foreign window. It should generally reparent
  * the window out of it's current heirarchy, hide it, and then
  * send a message to the owner requesting that the window be destroyed.
  */
  void         (*destroy_foreign)       (GdkWindow       *window);

  /* optional */
  gboolean     (* beep)                 (GdkWindow       *window);

  void         (* focus)                (GdkWindow       *window,
                                         guint32          timestamp);
  void         (* set_type_hint)        (GdkWindow       *window,
                                         GdkWindowTypeHint hint);
  GdkWindowTypeHint (* get_type_hint)   (GdkWindow       *window);
  void         (* set_modal_hint)       (GdkWindow *window,
                                         gboolean   modal);
  void         (* set_skip_taskbar_hint) (GdkWindow *window,
                                          gboolean   skips_taskbar);
  void         (* set_skip_pager_hint)  (GdkWindow *window,
                                         gboolean   skips_pager);
  void         (* set_urgency_hint)     (GdkWindow *window,
                                         gboolean   urgent);
  void         (* set_geometry_hints)   (GdkWindow         *window,
                                         const GdkGeometry *geometry,
                                         GdkWindowHints     geom_mask);
  void         (* set_title)            (GdkWindow   *window,
                                         const gchar *title);
  void         (* set_role)             (GdkWindow   *window,
                                         const gchar *role);
  void         (* set_startup_id)       (GdkWindow   *window,
                                         const gchar *startup_id);
  void         (* set_transient_for)    (GdkWindow *window,
                                         GdkWindow *parent);
  void         (* get_frame_extents)    (GdkWindow    *window,
                                         GdkRectangle *rect);
  void         (* set_override_redirect) (GdkWindow *window,
                                          gboolean override_redirect);
  void         (* set_accept_focus)     (GdkWindow *window,
                                         gboolean accept_focus);
  void         (* set_focus_on_map)     (GdkWindow *window,
                                         gboolean focus_on_map);
  void         (* set_icon_list)        (GdkWindow *window,
                                         GList     *pixbufs);
  void         (* set_icon_name)        (GdkWindow   *window,
                                         const gchar *name);
  void         (* iconify)              (GdkWindow *window);
  void         (* deiconify)            (GdkWindow *window);
  void         (* stick)                (GdkWindow *window);
  void         (* unstick)              (GdkWindow *window);
  void         (* maximize)             (GdkWindow *window);
  void         (* unmaximize)           (GdkWindow *window);
  void         (* fullscreen)           (GdkWindow *window);
  void         (* fullscreen_on_monitor) (GdkWindow *window, gint monitor);
  void         (* apply_fullscreen_mode) (GdkWindow *window);
  void         (* unfullscreen)         (GdkWindow *window);
  void         (* set_keep_above)       (GdkWindow *window,
                                         gboolean   setting);
  void         (* set_keep_below)       (GdkWindow *window,
                                         gboolean   setting);
  GdkWindow *  (* get_group)            (GdkWindow *window);
  void         (* set_group)            (GdkWindow *window,
                                         GdkWindow *leader);
  void         (* set_decorations)      (GdkWindow      *window,
                                         GdkWMDecoration decorations);
  gboolean     (* get_decorations)      (GdkWindow       *window,
                                         GdkWMDecoration *decorations);
  void         (* set_functions)        (GdkWindow    *window,
                                         GdkWMFunction functions);
  void         (* begin_resize_drag)    (GdkWindow     *window,
                                         GdkWindowEdge  edge,
                                         GdkDevice     *device,
                                         gint           button,
                                         gint           root_x,
                                         gint           root_y,
                                         guint32        timestamp);
  void         (* begin_move_drag)      (GdkWindow *window,
                                         GdkDevice     *device,
                                         gint       button,
                                         gint       root_x,
                                         gint       root_y,
                                         guint32    timestamp);
  void         (* enable_synchronized_configure) (GdkWindow *window);
  void         (* configure_finished)   (GdkWindow *window);
  void         (* set_opacity)          (GdkWindow *window,
                                         gdouble    opacity);
  void         (* set_composited)       (GdkWindow *window,
                                         gboolean   composited);
  void         (* destroy_notify)       (GdkWindow *window);
  GdkDragProtocol (* get_drag_protocol) (GdkWindow *window,
                                         GdkWindow **target);
  void         (* register_dnd)         (GdkWindow *window);
  GdkDragContext * (*drag_begin)        (GdkWindow *window,
                                         GdkDevice *device,
                                         GList     *targets,
                                         gint       x_root,
                                         gint       y_root);
  void         (*process_updates_recurse) (GdkWindow      *window,
                                           cairo_region_t *region);

  void         (*sync_rendering)          (GdkWindow      *window);
  gboolean     (*simulate_key)            (GdkWindow      *window,
                                           gint            x,
                                           gint            y,
                                           guint           keyval,
                                           GdkModifierType modifiers,
                                           GdkEventType    event_type);
  gboolean     (*simulate_button)         (GdkWindow      *window,
                                           gint            x,
                                           gint            y,
                                           guint           button,
                                           GdkModifierType modifiers,
                                           GdkEventType    event_type);

  gboolean     (*get_property)            (GdkWindow      *window,
                                           GdkAtom         property,
                                           GdkAtom         type,
                                           gulong          offset,
                                           gulong          length,
                                           gint            pdelete,
                                           GdkAtom        *actual_type,
                                           gint           *actual_format,
                                           gint           *actual_length,
                                           guchar        **data);
  void         (*change_property)         (GdkWindow      *window,
                                           GdkAtom         property,
                                           GdkAtom         type,
                                           gint            format,
                                           GdkPropMode     mode,
                                           const guchar   *data,
                                           gint            n_elements);
  void         (*delete_property)         (GdkWindow      *window,
                                           GdkAtom         property);

  gint         (* get_scale_factor)       (GdkWindow      *window);
  void         (* get_unscaled_size)      (GdkWindow      *window,
                                           int            *unscaled_width,
                                           int            *unscaled_height);

  void         (* set_opaque_region)      (GdkWindow      *window,
                                           cairo_region_t *region);
  void         (* set_shadow_width)       (GdkWindow      *window,
                                           gint            left,
                                           gint            right,
                                           gint            top,
                                           gint            bottom);
  gboolean     (* show_window_menu)       (GdkWindow      *window,
                                           GdkEvent       *event);
  GdkGLContext *(*create_gl_context)      (GdkWindow      *window,
                                           gboolean        attached,
                                           GdkGLContext   *share,
                                           GError        **error);
  gboolean     (* realize_gl_context)     (GdkWindow      *window,
                                           GdkGLContext   *context,
                                           GError        **error);

  void         (*invalidate_for_new_frame)(GdkWindow      *window,
                                           cairo_region_t *update_area);

  GdkDrawingContext *(* create_draw_context)  (GdkWindow            *window,
                                               const cairo_region_t *region);
  void               (* destroy_draw_context) (GdkWindow            *window,
                                               GdkDrawingContext    *context);

  gboolean (* titlebar_gesture) (GdkWindow          *window,
                                 GdkTitlebarGesture  gesture);
};

//typedef struct _GdkDisplay GdkDisplay;
//typedef struct _GdkDisplay2 _GdkDisplay;

#if 0
struct _GdkWindow2
{
  GObject parent_instance;

  GdkWindowImpl *impl; /* window-system-specific delegate object */

  GdkWindow *parent;
  GdkWindow *transient_for;
  GdkVisual *visual;

  gpointer user_data;

  gint x;
  gint y;

  GdkEventMask event_mask;
  guint8 window_type;

  guint8 depth;
  guint8 resize_count;

  gint8 toplevel_window_type;

  GList *filters;
  GList *children;
  GList children_list_node;
  GList *native_children;

  cairo_pattern_t *background;

  struct {
    /* The temporary surface that we're painting to. This will be composited
     * back into the window when we call end_paint. This is our poor-man's
     * way of doing double buffering. */
    cairo_surface_t *surface;

    cairo_region_t *region;
    cairo_region_t *flushed_region;
    cairo_region_t *need_blend_region;

    gboolean surface_needs_composite;
    gboolean use_gl;
  } current_paint;
  GdkGLContext *gl_paint_context;

  cairo_region_t *update_area;
  guint update_freeze_count;
  /* This is the update_area that was in effect when the current expose
     started. It may be smaller than the expose area if we'e painting
     more than we have to, but it represents the "true" damage. */
  cairo_region_t *active_update_area;
  /* We store the old expose areas to support buffer-age optimizations */
  cairo_region_t *old_updated_area[2];

  GdkWindowState old_state;
  GdkWindowState state;

  guint synthesized_crossing_event_id;

  guint8 alpha;
  guint8 fullscreen_mode;

  guint input_only : 1;
  guint pass_through : 1;
  guint modal_hint : 1;
  guint composited : 1;
  guint has_alpha_background : 1;

  guint destroyed : 2;

  guint accept_focus : 1;
  guint focus_on_map : 1;
  guint shaped : 1;
  guint support_multidevice : 1;
  guint effective_visibility : 2;
  guint visibility : 2; /* The visibility wrt the toplevel (i.e. based on clip_region) */
  guint native_visibility : 2; /* the native visibility of a impl windows */
  guint viewable : 1; /* mapped and all parents mapped */
  guint applied_shape : 1;
  guint in_update : 1;
  guint geometry_dirty : 1;
  guint event_compression : 1;
  guint frame_clock_events_paused : 1;

  /* The GdkWindow that has the impl, ref:ed if another window.
   * This ref is required to keep the wrapper of the impl window alive
   * for as long as any GdkWindow references the impl. */
  GdkWindow *impl_window;

  guint update_and_descendants_freeze_count;

  gint abs_x, abs_y; /* Absolute offset in impl */
  gint width, height;
  gint shadow_top;
  gint shadow_left;
  gint shadow_right;
  gint shadow_bottom;

  guint num_offscreen_children;

  /* The clip region is the part of the window, in window coordinates
     that is fully or partially (i.e. semi transparently) visible in
     the window hierarchy from the toplevel and down */
  cairo_region_t *clip_region;

  GdkCursor *cursor;
  GHashTable *device_cursor;

  cairo_region_t *shape;
  cairo_region_t *input_shape;

  GList *devices_inside;
  GHashTable *device_events;

  GHashTable *source_event_masks;
  gulong device_added_handler_id;
  gulong device_changed_handler_id;

  GdkFrameClock *frame_clock; /* NULL to use from parent or default */
  GdkWindowInvalidateHandlerFunc invalidate_handler;

  GdkDrawingContext *drawing_context;

  cairo_region_t *opaque_region;
};
#endif

#define GDK_WINDOW_TYPE(d) ((((GdkWindow *)(d)))->window_type)
#define GDK_WINDOW_DESTROYED(d) (((GdkWindow *)(d))->destroyed)

typedef struct _GdkColorInfo           GdkColorInfo;

typedef enum {
  GDK_COLOR_WRITEABLE = 1 << 0
} GdkColorInfoFlags;

struct _GdkColorInfo
{
  GdkColorInfoFlags flags;
  guint ref_count;
};


#endif /* _GDK_STRUCTS_H */
