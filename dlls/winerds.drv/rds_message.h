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
    RDS_MSG_PONG
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
        
        // More message parameter structs
        // ...
    } params;
} RDS_MESSAGE;

