
#if 0
#pragma makedep unix
#endif

//#include "config.h"

#include <stdarg.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "wingdi.h"
#include "wine/debug.h"

/* graphics.c */
#include "wine/debug.h"
#include "wine/gdi_driver.h"

#include "rds.h"

/* Import declarations for RDS service functions */
extern BOOL rds_move_to(PHYSDEV dev, INT x, INT y);
extern BOOL rds_line_to(PHYSDEV dev, INT x, INT y);
extern BOOL rds_rectangle(PHYSDEV dev, INT left, INT top, INT right, INT bottom, BOOL filled);
extern BOOL rds_text_out(PHYSDEV dev, INT x, INT y, LPCWSTR str, INT count);


WINE_DEFAULT_DEBUG_CHANNEL(rds);


/***********************************************************************
 *           rds_MoveTo
 */
BOOL rds_MoveTo(PHYSDEV dev, INT x, INT y)
{
    WINE_TRACE("dev=%p, x=%d, y=%d\n", dev, x, y);
    //return rds_MoveTo(dev, x, y);
    return;
}

/***********************************************************************
 *           rds_LineTo
 */
BOOL rds_LineTo(PHYSDEV dev, INT x, INT y)
{
    WINE_TRACE("dev=%p, x=%d, y=%d\n", dev, x, y);
//    COLORREF color = GetTextColor(dev->hdc);
    //return rds_line_to(dev, x, y, color);
    return;
}

/***********************************************************************
 *           rds_Rectangle
 */
BOOL rds_Rectangle(PHYSDEV dev, INT left, INT top, INT right, INT bottom)
{
    WINE_TRACE("dev=%p, left=%d, top=%d, right=%d, bottom=%d\n",
               dev, left, top, right, bottom);
    
  //  HBRUSH brush = GetCurrentObject(dev->hdc, OBJ_BRUSH);
   // BOOL filled = (brush && brush != GetStockObject(NULL_BRUSH));
  //  COLORREF color = GetTextColor(dev->hdc);
    
    //return rds_rectangle(dev, left, top, right, bottom, color, filled);
    return;
}

/***********************************************************************
 *           rds_TextOut
 */
BOOL rds_TextOut(PHYSDEV dev, INT x, INT y, LPCWSTR str, INT count)
{
    WINE_TRACE("dev=%p, x=%d, y=%d, str=%p, count=%d\n", 
               dev, x, y, str, count);
    
    //COLORREF color = GetTextColor(dev->hdc);
    //return rds_text_out(dev, x, y, str, count, color);
    return;
}

/////////////////////////////////////////////////////////////////////////
// In your driver initialization code
static const struct gdi_dc_funcs rds_funcs = {
    // ... other function pointers ...
    .pMoveTo = rds_MoveTo,
    .pLineTo = rds_LineTo,
    .pRectangle = rds_Rectangle,
    //.pTextOut = rds_TextOut,
    // ... other function pointers ...
};

// In your driver's initialization function
//void register_rds_driver(void)
//{
//    register_gdi_driver(&rds_funcs);
//}



