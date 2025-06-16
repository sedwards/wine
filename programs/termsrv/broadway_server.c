#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <process.h>

#include "broadway_server.h"
#include "rds.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(broadway);

// Global Broadway server instance
BROADWAY_SERVER g_broadway_server = {0};

// HTML client page embedded as string
static const char *broadway_html_client = 
"<!DOCTYPE html>\n"
"<html>\n"
"<head>\n"
"  <meta charset=\"UTF-8\">\n"
"  <title>Wine RDS Broadway Client</title>\n"
"  <style>\n"
"    body { margin: 0; padding: 10px; background: #f0f0f0; font-family: Arial, sans-serif; }\n"
"    canvas { border: 2px solid #333; background: white; cursor: crosshair; }\n"
"    #cursor { position: absolute; width: 8px; height: 8px; background: red; border-radius: 4px; pointer-events: none; z-index: 100; }\n"
"    .info { margin: 10px 0; }\n"
"    .status { padding: 5px; border-radius: 3px; }\n"
"    .connected { background: #d4edda; color: #155724; }\n"
"    .disconnected { background: #f8d7da; color: #721c24; }\n"
"  </style>\n"
"</head>\n"
"<body>\n"
"  <div class=\"info\">\n"
"    <h2>Wine RDS Broadway Client</h2>\n"
"    <div id=\"status\" class=\"status disconnected\">Connecting...</div>\n"
"  </div>\n"
"  <canvas id=\"screen\" width=\"800\" height=\"600\"></canvas>\n"
"  <div id=\"cursor\"></div>\n"
"\n"
"  <script>\n"
"    const canvas = document.getElementById('screen');\n"
"    const ctx = canvas.getContext('2d');\n"
"    const cursor = document.getElementById('cursor');\n"
"    const status = document.getElementById('status');\n"
"    let ws = null;\n"
"    let isConnected = false;\n"
"\n"
"    function updateStatus(connected) {\n"
"      isConnected = connected;\n"
"      status.textContent = connected ? 'Connected to Wine RDS' : 'Disconnected';\n"
"      status.className = connected ? 'status connected' : 'status disconnected';\n"
"    }\n"
"\n"
"    function connectWebSocket() {\n"
"      const wsUrl = `ws://${window.location.hostname}:8765`;\n"
"      ws = new WebSocket(wsUrl);\n"
"\n"
"      ws.onopen = () => {\n"
"        console.log('WebSocket connected');\n"
"        updateStatus(true);\n"
"      };\n"
"\n"
"      ws.onclose = () => {\n"
"        console.log('WebSocket disconnected');\n"
"        updateStatus(false);\n"
"        setTimeout(connectWebSocket, 2000); // Reconnect after 2 seconds\n"
"      };\n"
"\n"
"      ws.onerror = (error) => {\n"
"        console.error('WebSocket error:', error);\n"
"        updateStatus(false);\n"
"      };\n"
"\n"
"      ws.onmessage = (event) => {\n"
"        try {\n"
"          const msg = JSON.parse(event.data);\n"
"          if (msg.ack === 'mouse') {\n"
"            cursor.style.left = (msg.x + canvas.offsetLeft - 4) + 'px';\n"
"            cursor.style.top = (msg.y + canvas.offsetTop - 4) + 'px';\n"
"          }\n"
"        } catch (e) {\n"
"          console.log('Non-JSON message:', event.data);\n"
"        }\n"
"      };\n"
"    }\n"
"\n"
"    // Mouse event handling\n"
"    canvas.addEventListener('mousedown', (e) => {\n"
"      if (!isConnected) return;\n"
"      const rect = canvas.getBoundingClientRect();\n"
"      const x = Math.floor(e.clientX - rect.left);\n"
"      const y = Math.floor(e.clientY - rect.top);\n"
"      ws.send(JSON.stringify({ type: 'mouse', action: 'click', x, y, button: e.button }));\n"
"    });\n"
"\n"
"    canvas.addEventListener('mousemove', (e) => {\n"
"      const rect = canvas.getBoundingClientRect();\n"
"      const x = Math.floor(e.clientX - rect.left);\n"
"      const y = Math.floor(e.clientY - rect.top);\n"
"      cursor.style.left = (x + canvas.offsetLeft - 4) + 'px';\n"
"      cursor.style.top = (y + canvas.offsetTop - 4) + 'px';\n"
"    });\n"
"\n"
"    // Keyboard event handling\n"
"    window.addEventListener('keydown', (e) => {\n"
"      if (!isConnected) return;\n"
"      e.preventDefault();\n"
"      ws.send(JSON.stringify({ type: 'key', keyCode: e.keyCode, char: e.key, pressed: true }));\n"
"    });\n"
"\n"
"    window.addEventListener('keyup', (e) => {\n"
"      if (!isConnected) return;\n"
"      e.preventDefault();\n"
"      ws.send(JSON.stringify({ type: 'key', keyCode: e.keyCode, char: e.key, pressed: false }));\n"
"    });\n"
"\n"
"    // Framebuffer refresh\n"
"    function refreshFramebuffer() {\n"
"      fetch('/framebuffer')\n"
"        .then(response => response.arrayBuffer())\n"
"        .then(buffer => {\n"
"          const imageData = new ImageData(new Uint8ClampedArray(buffer), 800, 600);\n"
"          ctx.putImageData(imageData, 0, 0);\n"
"        })\n"
"        .catch(error => console.error('Error fetching framebuffer:', error));\n"
"    }\n"
"\n"
"    // Initialize\n"
"    connectWebSocket();\n"
"    refreshFramebuffer();\n"
"    setInterval(refreshFramebuffer, 100); // Refresh 10 times per second\n"
"\n"
"    // Focus handling\n"
"    canvas.tabIndex = 0;\n"
"    canvas.focus();\n"
"  </script>\n"
"</body>\n"
"</html>";

// Forward declarations
static DWORD WINAPI broadway_client_thread(LPVOID param);
static void broadway_handle_http_request(SOCKET client_sock, const char *request, BROADWAY_SERVER *server);

// Client thread parameter structure
typedef struct {
    SOCKET client_sock;
    BROADWAY_SERVER *server;
} BROADWAY_CLIENT_PARAM;

BOOL broadway_server_init(BROADWAY_SERVER *server, DWORD port)
{
    WSADATA wsaData;
    
    TRACE("Initializing Broadway server on port %d\n", port);
    
    memset(server, 0, sizeof(BROADWAY_SERVER));
    server->port = port;
    server->listen_sock = INVALID_SOCKET;
    server->websocket_sock = INVALID_SOCKET;
    
    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        ERR("WSAStartup failed\n");
        return FALSE;
    }
    
    // Initialize critical section for framebuffer access
    InitializeCriticalSection(&server->framebuffer_lock);
    
    // Create shared framebuffer
    if (!broadway_create_shared_framebuffer(server)) {
        ERR("Failed to create shared framebuffer\n");
        return FALSE;
    }
    
    server->enabled = TRUE;
    TRACE("Broadway server initialized successfully\n");
    return TRUE;
}

BOOL broadway_create_shared_framebuffer(BROADWAY_SERVER *server)
{
    TRACE("Creating shared framebuffer\n");
    
    // Create file mapping for shared framebuffer
    server->shm_handle = CreateFileMappingA(
        INVALID_HANDLE_VALUE,
        NULL,
        PAGE_READWRITE,
        0,
        BROADWAY_FRAMEBUFFER_SIZE,
        "Global\\winerds_framebuffer"
    );
    
    if (!server->shm_handle) {
        ERR("CreateFileMapping failed: %d\n", GetLastError());
        return FALSE;
    }
    
    // Map the framebuffer into memory
    server->framebuffer = MapViewOfFile(
        server->shm_handle,
        FILE_MAP_ALL_ACCESS,
        0,
        0,
        BROADWAY_FRAMEBUFFER_SIZE
    );
    
    if (!server->framebuffer) {
        ERR("MapViewOfFile failed: %d\n", GetLastError());
        CloseHandle(server->shm_handle);
        server->shm_handle = NULL;
        return FALSE;
    }
    
    // Initialize framebuffer to black
    memset(server->framebuffer, 0, BROADWAY_FRAMEBUFFER_SIZE);
    
    TRACE("Shared framebuffer created successfully\n");
    return TRUE;
}

BOOL broadway_update_framebuffer(BROADWAY_SERVER *server, RDS_SURFACE *surface)
{
    if (!server || !server->framebuffer || !surface || !surface->pBitmapBits) {
        return FALSE;
    }
    
    EnterCriticalSection(&server->framebuffer_lock);
    
    // Copy surface data to shared framebuffer
    // Note: RDS surface is likely BGR, need to convert to RGBA for web display
    BYTE *src = (BYTE*)surface->pBitmapBits;
    BYTE *dst = (BYTE*)server->framebuffer;
    
    int width = min(surface->width, 800);
    int height = min(surface->height, 600);
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int src_offset = (y * surface->width + x) * 4; // Assuming 32bpp
            int dst_offset = (y * 800 + x) * 4;
            
            // Convert BGR to RGBA
            dst[dst_offset + 0] = src[src_offset + 2]; // R
            dst[dst_offset + 1] = src[src_offset + 1]; // G
            dst[dst_offset + 2] = src[src_offset + 0]; // B  
            dst[dst_offset + 3] = 255; // A (opaque)
        }
    }
    
    server->framebuffer_dirty = TRUE;
    
    LeaveCriticalSection(&server->framebuffer_lock);
    return TRUE;
}

BOOL broadway_server_start(BROADWAY_SERVER *server)
{
    struct sockaddr_in addr;
    
    if (!server || !server->enabled) {
        return FALSE;
    }
    
    TRACE("Starting Broadway server on port %d\n", server->port);
    
    // Create HTTP server socket
    server->listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server->listen_sock == INVALID_SOCKET) {
        ERR("socket() failed: %d\n", WSAGetLastError());
        return FALSE;
    }
    
    // Set socket options
    int opt = 1;
    setsockopt(server->listen_sock, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
    
    // Bind socket
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons((unsigned short)server->port);
    
    if (bind(server->listen_sock, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        ERR("bind() failed: %d\n", WSAGetLastError());
        closesocket(server->listen_sock);
        server->listen_sock = INVALID_SOCKET;
        return FALSE;
    }
    
    // Listen for connections
    if (listen(server->listen_sock, SOMAXCONN) == SOCKET_ERROR) {
        ERR("listen() failed: %d\n", WSAGetLastError());
        closesocket(server->listen_sock);
        server->listen_sock = INVALID_SOCKET;
        return FALSE;
    }
    
    // Create WebSocket server socket (different port)
    server->websocket_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server->websocket_sock != INVALID_SOCKET) {
        addr.sin_port = htons(8765); // WebSocket port
        setsockopt(server->websocket_sock, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
        
        if (bind(server->websocket_sock, (struct sockaddr*)&addr, sizeof(addr)) == 0 &&
            listen(server->websocket_sock, SOMAXCONN) == 0) {
            server->websocket_enabled = TRUE;
            TRACE("WebSocket server listening on port 8765\n");
        } else {
            closesocket(server->websocket_sock);
            server->websocket_sock = INVALID_SOCKET;
            WARN("Failed to create WebSocket server\n");
        }
    }
    
    // Start server thread
    server->running = TRUE;
    server->server_thread = CreateThread(NULL, 0, broadway_server_thread, server, 0, NULL);
    
    if (server->websocket_enabled) {
        server->websocket_thread = CreateThread(NULL, 0, broadway_websocket_thread, server, 0, NULL);
    }
    
    TRACE("Broadway server started successfully\n");
    return TRUE;
}

DWORD WINAPI broadway_server_thread(LPVOID param)
{
    BROADWAY_SERVER *server = (BROADWAY_SERVER*)param;
    SOCKET client_sock;
    struct sockaddr_in client_addr;
    int addr_len = sizeof(client_addr);
    
    TRACE("Broadway server thread started\n");
    
    while (server->running) {
        client_sock = accept(server->listen_sock, (struct sockaddr*)&client_addr, &addr_len);
        
        if (client_sock == INVALID_SOCKET) {
            if (server->running) {
                ERR("accept() failed: %d\n", WSAGetLastError());
            }
            break;
        }
        
        TRACE("New HTTP client connected from %s\n", inet_ntoa(client_addr.sin_addr));
        
        // Create client thread
        BROADWAY_CLIENT_PARAM *param = malloc(sizeof(BROADWAY_CLIENT_PARAM));
        if (param) {
            param->client_sock = client_sock;
            param->server = server;
            
            HANDLE client_thread = CreateThread(NULL, 0, broadway_client_thread, param, 0, NULL);
            if (client_thread) {
                CloseHandle(client_thread); // We don't need to track individual client threads
            } else {
                free(param);
                closesocket(client_sock);
            }
        } else {
            closesocket(client_sock);
        }
    }
    
    TRACE("Broadway server thread exiting\n");
    return 0;
}

DWORD WINAPI broadway_client_thread(LPVOID param)
{
    BROADWAY_CLIENT_PARAM *client_param = (BROADWAY_CLIENT_PARAM*)param;
    SOCKET client_sock = client_param->client_sock;
    BROADWAY_SERVER *server = client_param->server;
    char request[4096];
    int bytes_received;
    
    free(client_param);
    
    // Read HTTP request
    bytes_received = recv(client_sock, request, sizeof(request) - 1, 0);
    if (bytes_received > 0) {
        request[bytes_received] = '\0';
        broadway_handle_http_request(client_sock, request, server);
    }
    
    closesocket(client_sock);
    return 0;
}

void broadway_handle_http_request(SOCKET client_sock, const char *request, BROADWAY_SERVER *server)
{
    TRACE("Handling HTTP request: %.50s...\n", request);
    
    if (strstr(request, "GET /framebuffer")) {
        // Send framebuffer data
        const char *header = "HTTP/1.1 200 OK\r\n"
                             "Content-Type: application/octet-stream\r\n"
                             "Content-Length: " "1920000" "\r\n" // 800*600*4
                             "Access-Control-Allow-Origin: *\r\n"
                             "\r\n";
        
        send(client_sock, header, strlen(header), 0);
        
        EnterCriticalSection(&server->framebuffer_lock);
        send(client_sock, server->framebuffer, BROADWAY_FRAMEBUFFER_SIZE, 0);
        LeaveCriticalSection(&server->framebuffer_lock);
        
    } else if (strstr(request, "GET /")) {
        // Send HTML client
        broadway_send_http_response(client_sock, "text/html", broadway_html_client, strlen(broadway_html_client));
    } else {
        // Send 404
        const char *response = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
        send(client_sock, response, strlen(response), 0);
    }
}

void broadway_send_http_response(SOCKET client_sock, const char *content_type, const void *data, int data_len)
{
    char header[512];
    snprintf(header, sizeof(header),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %d\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "\r\n",
        content_type, data_len);
    
    send(client_sock, header, strlen(header), 0);
    send(client_sock, data, data_len, 0);
}

DWORD WINAPI broadway_websocket_thread(LPVOID param)
{
    BROADWAY_SERVER *server = (BROADWAY_SERVER*)param;
    // Basic WebSocket implementation would go here
    // For now, this is a placeholder
    TRACE("WebSocket thread started (placeholder)\n");
    
    while (server->running && server->websocket_enabled) {
        Sleep(1000);
        // TODO: Implement WebSocket accept and message handling
    }
    
    return 0;
}

void broadway_server_stop(BROADWAY_SERVER *server)
{
    if (!server || !server->running) {
        return;
    }
    
    TRACE("Stopping Broadway server\n");
    
    server->running = FALSE;
    
    // Close sockets to break accept() calls
    if (server->listen_sock != INVALID_SOCKET) {
        closesocket(server->listen_sock);
        server->listen_sock = INVALID_SOCKET;
    }
    
    if (server->websocket_sock != INVALID_SOCKET) {
        closesocket(server->websocket_sock);
        server->websocket_sock = INVALID_SOCKET;
    }
    
    // Wait for server threads to exit
    if (server->server_thread) {
        WaitForSingleObject(server->server_thread, 5000);
        CloseHandle(server->server_thread);
        server->server_thread = NULL;
    }
    
    if (server->websocket_thread) {
        WaitForSingleObject(server->websocket_thread, 5000);
        CloseHandle(server->websocket_thread);
        server->websocket_thread = NULL;
    }
    
    TRACE("Broadway server stopped\n");
}

void broadway_server_cleanup(BROADWAY_SERVER *server)
{
    if (!server) {
        return;
    }
    
    broadway_server_stop(server);
    broadway_cleanup_shared_framebuffer(server);
    
    DeleteCriticalSection(&server->framebuffer_lock);
    WSACleanup();
    
    memset(server, 0, sizeof(BROADWAY_SERVER));
}

void broadway_cleanup_shared_framebuffer(BROADWAY_SERVER *server)
{
    if (server->framebuffer) {
        UnmapViewOfFile(server->framebuffer);
        server->framebuffer = NULL;
    }
    
    if (server->shm_handle) {
        CloseHandle(server->shm_handle);
        server->shm_handle = NULL;
    }
}

// Placeholder input injection functions
BOOL broadway_inject_mouse_event(int x, int y, int button, BOOL pressed)
{
    // TODO: Implement mouse event injection to Wine
    TRACE("Mouse event: x=%d, y=%d, button=%d, pressed=%d\n", x, y, button, pressed);
    return TRUE;
}

BOOL broadway_inject_keyboard_event(int keycode, BOOL pressed)
{
    // TODO: Implement keyboard event injection to Wine
    TRACE("Keyboard event: keycode=%d, pressed=%d\n", keycode, pressed);
    return TRUE;
}