/*
 * Dummy Unix file for Wine build system
 * 
 * This file exists solely to satisfy the Wine build system's
 * expectation of having Unix sources when certain patterns are detected.
 */

#include "config.h"
#include "wine/port.h"

/* Minimal function to prevent empty object file warnings */
void rds_unix_dummy(void)
{
    /* This function is never called - it just prevents 
     * the compiler from complaining about an empty translation unit */
}

