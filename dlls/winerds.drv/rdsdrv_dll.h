/*
 * Wine RDS Driver DLL Header - Simplified
 *
 * Copyright 2024 
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 */

#ifndef __WINE_RDSDRV_DLL_H
#define __WINE_RDSDRV_DLL_H

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"

#include "unixlib.h"
 
#define RDSDRV_UNIX_CALL(func, params) WINE_UNIX_CALL(rdsdrv_unix_func_ ## func, params)


/* Physical device structures for GDI driver */
struct gdi_physdev;
typedef struct gdi_physdev *PHYSDEV;

/* Constants */
#define WINE_DM_UNSUPPORTED 0x80000000
#define WINE_GDI_DRIVER_VERSION 104

/* Graphics function declarations */
BOOL rds_MoveTo(PHYSDEV dev, INT x, INT y);
BOOL rds_LineTo(PHYSDEV dev, INT x, INT y);
BOOL rds_Rectangle(PHYSDEV dev, INT left, INT top, INT right, INT bottom);
BOOL rds_TextOut(PHYSDEV dev, INT x, INT y, LPCWSTR str, INT count);

#endif /* __WINE_RDSDRV_DLL_H */

