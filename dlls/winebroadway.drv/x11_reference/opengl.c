/*
 * BROADWAYDRV OpenGL functions
 *
 * Copyright 2000 Lionel Ulmer
 * Copyright 2005 Alex Woods
 * Copyright 2005 Raphael Junqueira
 * Copyright 2006-2009 Roderick Colenbrander
 * Copyright 2006 Tomas Carnecky
 * Copyright 2012 Alexandre Julliard
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

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <sys/socket.h>
#ifdef HAVE_SYS_UN_H
#include <sys/un.h>
#endif

#include "broadwaydrv.h"
#include "xcomposite.h"
#include "winternl.h"
#include "wine/debug.h"

struct opengl_funcs *get_glx_driver( UINT version )
{
    return NULL;
}

void sync_gl_drawable( HWND hwnd, BOOL known_child )
{
}

void set_gl_drawable_parent( HWND hwnd, HWND parent )
{
}

void destroy_gl_drawable( HWND hwnd )
{
}

