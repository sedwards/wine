/*
 * Wine Remote Desktop Services - Main header
 *
 * Copyright 2025 Steven Edwards
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 */

#ifndef __WINE_RDS_H
#define __WINE_RDS_H

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "ntgdi.h"
//#include "wine/debug.h"
//#include "wine/gdi_driver.h"
#include "unixlib.h"

#ifdef RDS_MSG_INTERFACE

/* Message types for RDS service communication */
enum {
    RDS_MSG_CONNECT,            /* Driver connecting to service */
    RDS_MSG_DISCONNECT,         /* Driver disconnecting */
    RDS_MSG_CREATE_SURFACE,     /* Create a new rendering surface */
    RDS_MSG_DESTROY_SURFACE,    /* Destroy a rendering surface */
    RDS_MSG_MOVE_TO,            /* MoveTo operation */
    RDS_MSG_LINE_TO,            /* LineTo operation */
    RDS_MSG_RECTANGLE,          /* Rectangle operation */
    RDS_MSG_TEXT_OUT,           /* TextOut operation */
    RDS_MSG_FLUSH,              /* Flush rendering to output */
    /* Add more message types as needed */
};

/* Base message structure */
typedef struct {
    DWORD msg_type;             /* One of RDS_MSG_* values */
    DWORD size;                 /* Total message size */
    DWORD surface_id;           /* Target surface ID */
} RDS_MSG_HEADER;

/* Specific message structures */
typedef struct {
    RDS_MSG_HEADER header;      /* Common header */
    DWORD width;                /* Requested surface width */
    DWORD height;               /* Requested surface height */
    DWORD flags;                /* Format flags */
    WCHAR name[1];              /* Surface name (variable length) */
} RDS_MSG_CREATE_SURFACE;

typedef struct {
    RDS_MSG_HEADER header;      /* Common header */
    DWORD x;                    /* X coordinate */
    DWORD y;                    /* Y coordinate */
} RDS_MSG_MOVE_TO;

typedef struct {
    RDS_MSG_HEADER header;      /* Common header */
    DWORD x;                    /* X coordinate */
    DWORD y;                    /* Y coordinate */
    COLORREF color;             /* Line color */
} RDS_MSG_LINE_TO;

typedef struct {
    RDS_MSG_HEADER header;      /* Common header */
    DWORD left;                 /* Left coordinate */
    DWORD top;                  /* Top coordinate */
    DWORD right;                /* Right coordinate */
    DWORD bottom;               /* Bottom coordinate */
    COLORREF color;             /* Rectangle color */
    BOOL filled;                /* Filled rectangle flag */
} RDS_MSG_RECTANGLE;

typedef struct {
    RDS_MSG_HEADER header;      /* Common header */
    DWORD x;                    /* X coordinate */
    DWORD y;                    /* Y coordinate */
    COLORREF color;             /* Text color */
    DWORD count;                /* Character count */
    WCHAR text[1];              /* Text (variable length) */
} RDS_MSG_TEXT_OUT;

/* Response message */
typedef struct {
    DWORD status;               /* Status code */
    DWORD surface_id;           /* Surface ID (for create surface) */
    /* Add more response data as needed */
} RDS_MSG_RESPONSE;

#endif /* RDS_MSG_INTERFACE */

#ifdef WINESERVER_RDS_MSG_INTERFACE

/* In common header file - shared between driver and service */
#define RDS_REQ_BASE 0x1000

/* Request codes for RDS service */
enum rds_req
{
    RDS_REQ_CREATE_SURFACE = RDS_REQ_BASE,
    RDS_REQ_DESTROY_SURFACE,
    RDS_REQ_MOVE_TO,
    RDS_REQ_LINE_TO,
    RDS_REQ_RECTANGLE,
    RDS_REQ_TEXT_OUT,
    RDS_REQ_FLUSH,
    /* Add more as needed */
};

/* Surface creation params */
struct rds_create_surface_params
{
    int width;
    int height;
    int bpp;   /* bits per pixel */
    int flags;
};

/* Surface reply */
struct rds_surface_reply
{
    unsigned int surface_id;
    int status;
};

/* DrawLine params */
struct rds_line_params
{
    unsigned int surface_id;
    int x1, y1;   /* Start point (or current position for LineTo) */
    int x2, y2;   /* End point */
    int color;    /* RGB color */
};

/* Rectangle params */
struct rds_rectangle_params
{
    unsigned int surface_id;
    int left, top, right, bottom;
    int color;
    int filled;
};

/* TextOut params - fixed part */
struct rds_text_out_params
{
    unsigned int surface_id;
    int x, y;
    int color;
    int count;
    /* Text follows in variable part */
};

#endif /* WINESERVER_RDS_MSG_INTERFACE */

/* Surface management */
typedef struct _RDS_SURFACE {
    DWORD id;         /* Unique surface ID */
    int width;
    int height;
    int bpp;          /* Bits per pixel */
    void *data;       /* Pixel data */
    int stride;       /* Bytes per row */
    BOOL dirty;       /* Whether surface needs updating */
    RECT dirty_rect;  /* Region that needs updating */
    DWORD current_x;  /* Current pen X position */
    DWORD current_y;  /* Current pen Y position */
} RDS_SURFACE;

typedef struct _RDS_SERVICE {
    BOOL should_exit;
    RDS_SURFACE *default_surface;
    /* Add more service state as needed */
} RDS_SERVICE;

/* Define maximum number of devices to track per session */
#define MAX_RDS_DEVICES_PER_SESSION 100

#if 0
/* Main device tracking structure */
typedef struct _RDSDRV_PDEVICE
{
    HDC         hdc;           /* The underlying Wine DC */
    DWORD       session_id;    /* Session this device belongs to */
    BOOL        is_screen;     /* Is this a screen DC or memory DC? */
    RECT        bounds;        /* Bounds of the device (for screen DCs) */
    DWORD       bpp;           /* Color depth */
    void*       surface_data;  /* Optional pointer to surface bitmap data */
    DWORD       flags;         /* Additional device flags */
    HANDLE      mutex;         /* Synchronization for device access */
    struct _RDSDRV_PDEVICE* next; /* For linked list of session devices */
} RDSDRV_PDEVICE;

/* Session-specific device tracking */
typedef struct _RDS_SESSION_DEVICES
{
    DWORD           session_id;
    DWORD           device_count;
    RDSDRV_PDEVICE* device_list;
    HANDLE          session_mutex;
} RDS_SESSION_DEVICES;
#endif

/* Surface management functions */
BOOL surface_initialize(void);
void surface_shutdown(void);
RDS_SURFACE *create_surface(DWORD width, DWORD height);
RDS_SURFACE *create_surface_ex(DWORD width, DWORD height, DWORD bpp);
RDS_SURFACE *find_surface(DWORD surface_id);
BOOL destroy_surface(DWORD surface_id);
void surface_mark_dirty(RDS_SURFACE *surface, const RECT *rect);
void *surface_get_data(RDS_SURFACE *surface);

/* GDI primitive functions */
BOOL gdi_initialize(void);
void gdi_shutdown(void);
void gdi_draw_test_pattern(RDS_SURFACE *surface);
BOOL gdi_line_to(RDS_SURFACE *surface, int x, int y, COLORREF color);
BOOL gdi_move_to(RDS_SURFACE *surface, int x, int y);
BOOL gdi_rectangle(RDS_SURFACE *surface, int left, int top, int right, int bottom, COLORREF color, BOOL filled);
BOOL gdi_text_out(RDS_SURFACE *surface, int x, int y, const WCHAR *text, int count, COLORREF color);

/* Session management functions */
BOOL session_initialize(int port);
void session_shutdown(void);
void session_process_events(void);

/* Broadway diagnostic interface */
BOOL broadway_initialize(int port);
void broadway_shutdown(void);
void broadway_update(RDS_SURFACE *surface);

#endif /* __WINE_RDS_H */

