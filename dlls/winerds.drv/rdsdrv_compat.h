/*
 * RDS Driver Compatibility Header
 * 
 * Provides missing definitions for Wine driver development
 */

#ifndef __RDSDRV_COMPAT_H
#define __RDSDRV_COMPAT_H

/* Include this in files that need missing Wine definitions */
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"

/* Missing constants */
#ifndef WINE_DM_UNSUPPORTED
#define WINE_DM_UNSUPPORTED 0x80000000
#endif

#ifndef WINE_GDI_DRIVER_VERSION
#define WINE_GDI_DRIVER_VERSION 104
#endif

#ifndef GDI_PRIORITY_GRAPHICS_DRV
#define GDI_PRIORITY_GRAPHICS_DRV 200
#endif

/* Display change return codes */
#ifndef DISP_CHANGE_SUCCESSFUL
#define DISP_CHANGE_SUCCESSFUL 0
#endif

#ifndef DISP_CHANGE_BADMODE
#define DISP_CHANGE_BADMODE -2
#endif

/* Font face size */
#ifndef LF_FACESIZE
#define LF_FACESIZE 32
#endif

/* Brush styles */
#ifndef BS_NULL
#define BS_NULL 1
#endif

#ifndef BS_HOLLOW
#define BS_HOLLOW BS_NULL
#endif

/* Function declaration for setting user driver */
extern void __wine_set_user_driver(const struct user_driver_funcs *funcs, UINT version);

#endif /* __RDSDRV_COMPAT_H */

