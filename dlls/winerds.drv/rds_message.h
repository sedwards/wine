#ifndef RDS_MESSAGE_H
#define RDS_MESSAGE_H

// Shared between termsrv.exe.so and winerds.drv
typedef enum _RDS_MESSAGE_TYPE
{
    // Driver messages
    RDS_MSG_CREATE_SURFACE,
    RDS_MSG_SURFACE_CREATED,
    RDS_MSG_DESTROY_SURFACE,
    RDS_MSG_PAINT_SURFACE,
    RDS_MSG_PAINT_COMPLETE,
    RDS_MSG_GET_SURFACE_DATA,
    RDS_MSG_SURFACE_UPDATED,

    // Service messages
    RDS_MSG_WINDOW_CREATED,
    RDS_MSG_WINDOW_DESTROYED,
    RDS_MSG_INPUT_EVENT,

    // System messages
    RDS_MSG_SHUTDOWN,
    RDS_MSG_PING,
    RDS_MSG_PONG,

    // GDI drawing operations
    RDS_MSG_MOVE_TO,
    RDS_MSG_LINE_TO,
    RDS_MSG_RECTANGLE,
    RDS_MSG_TEXT_OUT,
    RDS_MSG_BIT_BLT,

    // New messages
    RDS_MSG_ENABLE_PRIMARY_SURFACE, // Sent by winerds.drv when DrvEnableSurface is called for the primary display
    RDS_MSG_GDIOBJ_CREATED          // Response from termsrv after creating a GDI object (e.g., pen) requested by winerds.drv
} RDS_MESSAGE_TYPE;

typedef struct _RDS_MESSAGE
{
    RDS_MESSAGE_TYPE msgType;

    union
    {
        struct
        {
            DWORD width;
            DWORD height;
            DWORD bpp;
        } createSurface;

        struct
        {
            DWORD_PTR surfaceId;
        } surfaceCreated;

        struct
        {
            DWORD_PTR surfaceId;
            RECT updateRect;
        } paintSurface;

        struct
        {
            HWND windowId;
            DWORD_PTR surfaceId;
        } surfaceUpdated;

        struct
        {
            DWORD_PTR surfaceId;
        } getSurfaceData;

        struct
        {
            DWORD_PTR surfaceId;
            DWORD x;
            DWORD y;
        } moveTo;

        struct
        {
            DWORD_PTR surfaceId;
            DWORD x;
            DWORD y;
            COLORREF color;      // Pen color, effectively logpen.lopnColor
            UINT  lopnStyle;     // Pen style (e.g., PS_SOLID)
            INT   lopnWidth_x;   // Pen width in logical units for x 
        } lineTo;

        struct
        {
            DWORD_PTR surfaceId;
            DWORD left;
            DWORD top;
            DWORD right;
            DWORD bottom;
            COLORREF color;      // Pen color for border, effectively pen_lopnColor
            BOOL filled;         // Whether the rectangle is filled

            // Pen properties for border
            UINT     pen_lopnStyle;
            INT      pen_lopnWidth_x;
            // COLORREF pen_lopnColor is 'color' above

            // Brush properties for fill (if filled is TRUE)
            UINT     brush_lbStyle;    // Brush style (e.g., BS_SOLID, BS_HATCHED)
            COLORREF brush_lbColor;    // Brush color
            ULONG_PTR brush_lbHatch;   // Hatch style if BS_HATCHED (e.g., HS_DIAGCROSS)
        } rectangle;

        struct
        {
            DWORD_PTR surfaceId;
            DWORD x;
            DWORD y;
            COLORREF color;      // Text foreground color, effectively text_fg_color
            DWORD count;         // Number of WCHARs
            DWORD data_size;     // Size in bytes of the text data that follows

            // Text state
            // COLORREF text_fg_color is 'color' above
            COLORREF text_bk_color;
            INT      bk_mode;    // OPAQUE or TRANSPARENT from SetBkMode

            // Font properties (subset of LOGFONTW)
            LONG     font_lfHeight;
            LONG     font_lfWidth;
            LONG     font_lfWeight;
            BYTE     font_lfItalic;
            BYTE     font_lfUnderline;
            BYTE     font_lfStrikeOut;
            BYTE     font_lfCharSet;
            WCHAR    font_lfFaceName[LF_FACESIZE]; // LF_FACESIZE is usually 32
            // WCHAR text[...] will follow RDS_MESSAGE in the stream
        } textOut;

        struct
        {
            DWORD_PTR surfaceId_dest;
            DWORD x_dest;
            DWORD y_dest;
            DWORD width;
            DWORD height;
            DWORD_PTR surfaceId_src; // 0 if source is a bitmap block
            DWORD x_src;
            DWORD y_src;
            DWORD rop;
            DWORD data_size; // Size in bytes of the bitmap data that follows, if surfaceId_src is 0
            // BYTE data[...] will follow RDS_MESSAGE in the stream if surfaceId_src is 0
        } bitBlt;

        struct
        {
            DWORD_PTR surfaceId; // The ID the driver will use for this primary surface (e.g., 1)
                                 // Could also include desired width, height, bpp if winerds.drv dictates these to termsrv
        } enablePrimarySurface;

        struct 
        {
            DWORD_PTR temp_driver_hobject_id;   // ID used by winerds.drv when requesting object creation
            DWORD_PTR actual_termsrv_hobject_id; // Handle or ID assigned by termsrv (opaque to winerds.drv)
                                                // This is for a potential future model where termsrv manages GDI object handles.
                                                // For the "send effective state" model, this response might not be immediately used.
        } gdiObjCreated;
    } params;
} RDS_MESSAGE;

#endif
