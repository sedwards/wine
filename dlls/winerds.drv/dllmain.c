/*
 * winerds.drv entry points - Win32 Only Version
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 */

#include "rdsdrv_dll.h"
#include "ntuser.h"
#include "winuser.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(rdsdrv);

/* This file is kept minimal since the main initialization
 * is now handled in winerdsdrv_main.c 
 * 
 * We removed all the Unix library calls since we're using
 * a simplified Win32-only approach.
 */

BOOL WINAPI DllMain2(HINSTANCE instance, DWORD reason, void *reserved)
{
    switch (reason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(instance);
            TRACE("winerds.drv loaded (Win32 mode)\n");
            break;
            
        case DLL_PROCESS_DETACH:
            TRACE("winerds.drv unloaded\n");
            break;
    }

    return TRUE;
}

