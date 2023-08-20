/* GDK - The GIMP Drawing Kit
 * Copyright (C) 2009 Carlos Garnacho <carlosg@gnome.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

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
//#include <dlfcn.h>
//#include <fcntl.h>
//#include <sys/mman.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS

#include "basetsd.h"
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "wine/server.h"
//#include "wine/debug.h"
//#include "wine/list.h"

#include <glib.h>
#include <gio/gunixsocketaddress.h>

#include "eventsource.h"

static gboolean broadway_event_source_prepare  (GSource     *source,
                                           gint        *timeout);
static gboolean broadway_event_source_check    (GSource     *source);
static gboolean broadway_event_source_dispatch (GSource     *source,
                                           GSourceFunc  callback,
                                           gpointer     user_data);
static void     broadway_event_source_finalize (GSource     *source);

#define HAS_FOCUS(toplevel)                           \
  ((toplevel)->has_focus || (toplevel)->has_pointer_focus)

struct Display
{
  Display display;

  //GdkScreen *screen;

  HWND hwnd;
  HWND clipboard_hwnd;
}


struct _GdkEventSource
{
  GSource source;

  Display *display;
  GPollFD event_poll_fd;
};

static GSourceFuncs event_funcs = {
  broadway_event_source_prepare,
  broadway_event_source_check,
  broadway_event_source_dispatch,
  broadway_event_source_finalize
};

static GList *event_sources = NULL;

static gboolean
broadway_event_source_prepare (GSource *source,
                          gint    *timeout)
{
  Display *display = ((GdkEventSource*) source)->display;
  gboolean retval;

  g_threads_enter ();

  *timeout = -1;

  //retval = (_broadway_event_queue_find_first (display) != NULL);

  g_threads_leave ();

  return retval;
}

static gboolean
broadway_event_source_check (GSource *source)
{
  GdkEventSource *event_source = (GdkEventSource*) source;
  gboolean retval;

  gdk_threads_enter ();

  if (event_source->display->event_pause_count > 0 ||
      event_source->event_poll_fd.revents & G_IO_IN)
    //retval = (_broadway_event_queue_find_first (event_source->display) != NULL);
  else
    retval = FALSE;

  gdk_threads_leave ();

  return retval;
}

void
_broadway_events_got_input (BroadwayInputMsg *message)
{
  Display *display;
  BroadwayDisplay *display_broadway;
  BroadwayDeviceManager *device_manager;
  Screen *screen;
  Window *window;
  GdkEvent *event = NULL;
  GList *node;
  GSList *list, *d;

  display = NULL;

  list = gdk_display_manager_list_displays (gdk_display_manager_get ());
  // FIXME:  also this code is doing internally is the following:
  // g_slist_copy (manager->displays);
  // It seems like we can hack this
  for (d = list; d; d = d->next)
    {
      //if (GDK_IS_BROADWAY_DISPLAY (d->data))
      //  {
          display = d->data;
          break;
      //  }
    }
  g_slist_free (list);

  g_assert (display != NULL);

  /* This is going to setup the broadway side of the window */
  display_broadway = GDK_BROADWAY_DISPLAY (display);
  G_GNUC_BEGIN_IGNORE_DEPRECATIONS;

  /* This is going to setup the broadway side of keybd/mouse handling, I think */
  device_manager = GDK_BROADWAY_DEVICE_MANAGER (gdk_display_get_device_manager (display));
  G_GNUC_END_IGNORE_DEPRECATIONS;

  switch (message->base.type) {
  case BROADWAY_EVENT_ENTER:
    window = g_hash_table_lookup (display_broadway->id_ht, GINT_TO_POINTER (message->pointer.event_window_id));
    if (window)
      {
	event = broadway_event_new (GDK_ENTER_NOTIFY);
	event->crossing.window = g_object_ref (window);
	event->crossing.time = message->base.time;
	event->crossing.x = message->pointer.win_x;
	event->crossing.y = message->pointer.win_y;
	event->crossing.x_root = message->pointer.root_x;
	event->crossing.y_root = message->pointer.root_y;
	event->crossing.state = message->pointer.state;
	event->crossing.mode = message->crossing.mode;
	event->crossing.detail = GDK_NOTIFY_ANCESTOR;
	broadway_event_set_device (event, device_manager->core_pointer);
	broadway_event_set_seat (event, gdk_device_get_seat (device_manager->core_pointer));

	node = _broadway_event_queue_append (display, event);
	_gdk_windowing_got_event (display, node, event, message->base.serial);
      }
    break;
  case BROADWAY_EVENT_LEAVE:
    window = g_hash_table_lookup (display_broadway->id_ht, GINT_TO_POINTER (message->pointer.event_window_id));
    if (window)
      {
	event = broadway_event_new (GDK_LEAVE_NOTIFY);
	event->crossing.window = g_object_ref (window);
	event->crossing.time = message->base.time;
	event->crossing.x = message->pointer.win_x;
	event->crossing.y = message->pointer.win_y;
	event->crossing.x_root = message->pointer.root_x;
	event->crossing.y_root = message->pointer.root_y;
	event->crossing.state = message->pointer.state;
	event->crossing.mode = message->crossing.mode;
	event->crossing.detail = GDK_NOTIFY_ANCESTOR;
	broadway_event_set_device (event, device_manager->core_pointer);
	broadway_event_set_seat (event, gdk_device_get_seat (device_manager->core_pointer));

	node = _broadway_event_queue_append (display, event);
	_gdk_windowing_got_event (display, node, event, message->base.serial);
      }
    break;
  case BROADWAY_EVENT_POINTER_MOVE:
    if (_broadway_moveresize_handle_event (display, message))
      break;

    window = g_hash_table_lookup (display_broadway->id_ht, GINT_TO_POINTER (message->pointer.event_window_id));
    if (window)
      {
	event = broadway_event_new (GDK_MOTION_NOTIFY);
	event->motion.window = g_object_ref (window);
	event->motion.time = message->base.time;
	event->motion.x = message->pointer.win_x;
	event->motion.y = message->pointer.win_y;
	event->motion.x_root = message->pointer.root_x;
	event->motion.y_root = message->pointer.root_y;
	event->motion.state = message->pointer.state;
	broadway_event_set_device (event, device_manager->core_pointer);
	broadway_event_set_seat (event, gdk_device_get_seat (device_manager->core_pointer));

	node = _broadway_event_queue_append (display, event);
	_gdk_windowing_got_event (display, node, event, message->base.serial);
      }

    break;
  case BROADWAY_EVENT_BUTTON_PRESS:
  case BROADWAY_EVENT_BUTTON_RELEASE:
    if (message->base.type != 'b' &&
	_broadway_moveresize_handle_event (display, message))
      break;

    window = g_hash_table_lookup (display_broadway->id_ht, GINT_TO_POINTER (message->pointer.event_window_id));
    if (window)
      {
	event = broadway_event_new (message->base.type == 'b' ? GDK_BUTTON_PRESS : GDK_BUTTON_RELEASE);
	event->button.window = g_object_ref (window);
	event->button.time = message->base.time;
	event->button.x = message->pointer.win_x;
	event->button.y = message->pointer.win_y;
	event->button.x_root = message->pointer.root_x;
	event->button.y_root = message->pointer.root_y;
	event->button.button = message->button.button;
	event->button.state = message->pointer.state;
	broadway_event_set_device (event, device_manager->core_pointer);
	broadway_event_set_seat (event, gdk_device_get_seat (device_manager->core_pointer));

	node = _broadway_event_queue_append (display, event);
	_gdk_windowing_got_event (display, node, event, message->base.serial);
      }

    break;
  case BROADWAY_EVENT_SCROLL:
    window = g_hash_table_lookup (display_broadway->id_ht, GINT_TO_POINTER (message->pointer.event_window_id));
    if (window)
      {
	event = broadway_event_new (GDK_SCROLL);
	event->scroll.window = g_object_ref (window);
	event->scroll.time = message->base.time;
	event->scroll.x = message->pointer.win_x;
	event->scroll.y = message->pointer.win_y;
	event->scroll.x_root = message->pointer.root_x;
	event->scroll.y_root = message->pointer.root_y;
        event->scroll.state = message->pointer.state;
	event->scroll.direction = message->scroll.dir == 0 ? GDK_SCROLL_UP : GDK_SCROLL_DOWN;
	broadway_event_set_device (event, device_manager->core_pointer);
	broadway_event_set_seat (event, gdk_device_get_seat (device_manager->core_pointer));

	node = _broadway_event_queue_append (display, event);
	_gdk_windowing_got_event (display, node, event, message->base.serial);
      }

    break;
  case BROADWAY_EVENT_TOUCH:
    break;
  case BROADWAY_EVENT_KEY_PRESS:
  case BROADWAY_EVENT_KEY_RELEASE:
    window = g_hash_table_lookup (display_broadway->id_ht,
				  GINT_TO_POINTER (message->key.window_id));
    if (window)
      {
	event = broadway_event_new (message->base.type == 'k' ? GDK_KEY_PRESS : GDK_KEY_RELEASE);
	event->key.window = g_object_ref (window);
	event->key.time = message->base.time;
	event->key.keyval = message->key.key;
	event->key.state = message->key.state;
	event->key.hardware_keycode = message->key.key;
        broadway_event_set_scancode (event, message->key.key);
	event->key.length = 0;
	broadway_event_set_device (event, device_manager->core_keyboard);
	broadway_event_set_seat (event, gdk_device_get_seat (device_manager->core_keyboard));

	node = _broadway_event_queue_append (display, event);
	_gdk_windowing_got_event (display, node, event, message->base.serial);
      }

    break;
  case BROADWAY_EVENT_GRAB_NOTIFY:
  case BROADWAY_EVENT_UNGRAB_NOTIFY:
    _gdk_display_device_grab_update (display, device_manager->core_pointer, device_manager->core_pointer, message->base.serial);
    break;

  case BROADWAY_EVENT_CONFIGURE_NOTIFY:
    window = g_hash_table_lookup (display_broadway->id_ht, GINT_TO_POINTER (message->configure_notify.id));
    if (window)
      {
	window->x = message->configure_notify.x;
	window->y = message->configure_notify.y;

	event = broadway_event_new (GDK_CONFIGURE);
	event->configure.window = g_object_ref (window);
	event->configure.x = message->configure_notify.x;
	event->configure.y = message->configure_notify.y;
	event->configure.width = message->configure_notify.width;
	event->configure.height = message->configure_notify.height;

	node = _broadway_event_queue_append (display, event);
	_gdk_windowing_got_event (display, node, event, message->base.serial);

	if (window->resize_count >= 1)
	  {
	    window->resize_count -= 1;

	    if (window->resize_count == 0)
	      _broadway_moveresize_configure_done (display, window);
	  }
      }
    break;

  case BROADWAY_EVENT_DELETE_NOTIFY:
    window = g_hash_table_lookup (display_broadway->id_ht, GINT_TO_POINTER (message->delete_notify.id));
    if (window)
      {
	event = broadway_event_new (GDK_DELETE);
	event->any.window = g_object_ref (window);

	node = _broadway_event_queue_append (display, event);
	_gdk_windowing_got_event (display, node, event, message->base.serial);
      }
    break;

  case BROADWAY_EVENT_SCREEN_SIZE_CHANGED:
    FIXME("Got resize event, don't know what to do\n");
    //screen = gdk_display_get_default_screen (display);
    //window = gdk_screen_get_root_window (screen);
    window->width = message->screen_resize_notify.width;
    window->height = message->screen_resize_notify.height;

    _gdk_window_update_size (window);
    _broadway_screen_size_changed (screen, &message->screen_resize_notify);
    break;

  case BROADWAY_EVENT_FOCUS:
    window = g_hash_table_lookup (display_broadway->id_ht, GINT_TO_POINTER (message->focus.old_id));
    if (window)
      {
	event = broadway_event_new (GDK_FOCUS_CHANGE);
	event->focus_change.window = g_object_ref (window);
	event->focus_change.in = FALSE;
	broadway_event_set_device (event, device_manager->core_pointer);
	broadway_event_set_seat (event, gdk_device_get_seat (device_manager->core_pointer));
	node = _broadway_event_queue_append (display, event);
	_gdk_windowing_got_event (display, node, event, message->base.serial);
      }
    window = g_hash_table_lookup (display_broadway->id_ht, GINT_TO_POINTER (message->focus.new_id));
    if (window)
      {
	event = broadway_event_new (GDK_FOCUS_CHANGE);
	event->focus_change.window = g_object_ref (window);
	event->focus_change.in = TRUE;
	broadway_event_set_device (event, device_manager->core_pointer);
	broadway_event_set_seat (event, gdk_device_get_seat (device_manager->core_pointer));
	node = _broadway_event_queue_append (display, event);
	_gdk_windowing_got_event (display, node, event, message->base.serial);
      }
    break;

  default:
    g_printerr ("_broadway_events_got_input - Unknown input command %c\n", message->base.type);
    break;
  }
}

//void
//_broadway_display_queue_events (GdkDisplay *display)
//{
//}

static gboolean
broadway_event_source_dispatch (GSource     *source,
                           GSourceFunc  callback,
                           gpointer     user_data)
{
  GdkDisplay *display = ((GdkEventSource*) source)->display;
  GdkEvent *event;

  gdk_threads_enter ();

  event = gdk_display_get_event (display);

  if (event)
    {
      _broadway_event_emit (event);

      broadway_event_free (event);
    }

  gdk_threads_leave ();

  return TRUE;
}

static void
broadway_event_source_finalize (GSource *source)
{
  GdkEventSource *event_source = (GdkEventSource *)source;

  event_sources = g_list_remove (event_sources, event_source);
}

/// FIXME: We have to register an event source here that is win32 compatiable
//And figure out how to change GdkDisplay to be close enough to our
//broadwaydrv display to make it happy to map shit too
GSource *
_broadway_event_source_new (GdkDisplay *display)
{
  GSource *source;
  GdkEventSource *event_source;
  char *name;

  source = g_source_new (&event_funcs, sizeof (GdkEventSource));
  name = g_strdup_printf ("GDK Broadway Event source (%s)",
			  gdk_display_get_name (display));
  g_source_set_name (source, name);
  g_free (name);
  event_source = (GdkEventSource *) source;
  event_source->display = display;

  g_source_set_priority (source, GDK_PRIORITY_EVENTS);
  g_source_set_can_recurse (source, TRUE);
  g_source_attach (source, NULL);

  event_sources = g_list_prepend (event_sources, source);

  return source;
}

GList* _broadway_event_queue_find_first   (GdkDisplay *display)
{
  printf("stub - _broadway_event_queue_find_first\n");
}

