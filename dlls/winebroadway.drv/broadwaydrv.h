#ifndef BROADWAYDRV_H
#define BROADWAYDRV_H

#include <glib.h>
#include <gio/gunixsocketaddress.h>

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

//unsigned int screen_width;
//unsigned int screen_height;

#endif /* BROADWAYDRV_H */
