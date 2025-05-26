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
    RDS_MSG_BIT_BLT
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
            COLORREF color;
        } lineTo;

        struct
        {
            DWORD_PTR surfaceId;
            DWORD left;
            DWORD top;
            DWORD right;
            DWORD bottom;
            COLORREF color;
            BOOL filled;
        } rectangle;

        struct
        {
            DWORD_PTR surfaceId;
            DWORD x;
            DWORD y;
            COLORREF color;
            DWORD count;      // Number of WCHARs
            DWORD data_size;  // Size in bytes of the text data that follows the RDS_MESSAGE struct
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
    } params;
} RDS_MESSAGE;

