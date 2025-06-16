/*
 * RDS Constants Header - Missing definitions
 *
 * Copyright 2024
 */

#ifndef __RDS_CONSTANTS_H
#define __RDS_CONSTANTS_H

/* Font face name size - typically 32 characters */
#ifndef LF_FACESIZE
#define LF_FACESIZE 32
#endif

/* GDI Driver priority levels */
#ifndef GDI_PRIORITY_GRAPHICS_DRV
#define GDI_PRIORITY_GRAPHICS_DRV 0
#endif

/* Wine Display Device Manager return values */
#ifndef WINE_DM_UNSUPPORTED  
#define WINE_DM_UNSUPPORTED ((UINT)-1)
#endif

/* Wine GDI Driver version */
#ifndef WINE_GDI_DRIVER_VERSION
#define WINE_GDI_DRIVER_VERSION 0x0001
#endif

/* External function prototype */
extern void __wine_set_user_driver(const struct user_driver_funcs *funcs, UINT version);

#endif /* __RDS_CONSTANTS_H */
