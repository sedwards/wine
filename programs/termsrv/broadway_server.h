#ifndef BROADWAY_SERVER_H
#define BROADWAY_SERVER_H

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "rds.h"

// Broadway server configuration
#define BROADWAY_DEFAULT_PORT 8080
#define BROADWAY_MAX_CLIENTS 10
#define BROADWAY_FRAMEBUFFER_SIZE (800 * 600 * 4) // RGBA32

// Broadway server state
typedef struct _BROADWAY_SERVER {
    BOOL enabled;
    BOOL running;
    DWORD port;
    SOCKET listen_sock;
    HANDLE server_thread;
    HANDLE client_threads[BROADWAY_MAX_CLIENTS];
    DWORD active_clients;
    
    // Shared framebuffer for web clients
    HANDLE shm_handle;
    void *framebuffer;
    BOOL framebuffer_dirty;
    CRITICAL_SECTION framebuffer_lock;
    
    // WebSocket support
    BOOL websocket_enabled;
    SOCKET websocket_sock;
    HANDLE websocket_thread;
} BROADWAY_SERVER;

// Function prototypes
BOOL broadway_server_init(BROADWAY_SERVER *server, DWORD port);
BOOL broadway_server_start(BROADWAY_SERVER *server);
void broadway_server_stop(BROADWAY_SERVER *server);
void broadway_server_cleanup(BROADWAY_SERVER *server);

// Framebuffer management
BOOL broadway_create_shared_framebuffer(BROADWAY_SERVER *server);
BOOL broadway_update_framebuffer(BROADWAY_SERVER *server, RDS_SURFACE *surface);
void broadway_cleanup_shared_framebuffer(BROADWAY_SERVER *server);

// Client handling
DWORD WINAPI broadway_server_thread(LPVOID param);
DWORD WINAPI broadway_client_thread(LPVOID param);
DWORD WINAPI broadway_websocket_thread(LPVOID param);

// HTTP/WebSocket protocol
void broadway_handle_http_request(SOCKET client_sock, const char *request, BROADWAY_SERVER *server);
void broadway_handle_websocket_frame(SOCKET client_sock, const char *frame, int frame_len, BROADWAY_SERVER *server);
void broadway_send_http_response(SOCKET client_sock, const char *content_type, const void *data, int data_len);

// Input injection
BOOL broadway_inject_mouse_event(int x, int y, int button, BOOL pressed);
BOOL broadway_inject_keyboard_event(int keycode, BOOL pressed);

// Global Broadway server instance
extern BROADWAY_SERVER g_broadway_server;

#endif // BROADWAY_SERVER_H