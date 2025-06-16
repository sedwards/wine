// terminal_service_ws.c - WebSocket bridge with toggleable feedback via CLI
// Compile with: gcc terminal_service_ws.c mongoose.c -o terminal_service_ws.exe -lws2_32 -ladvapi32 -mwindows

#include "mongoose.h"
#include <windows.h>
#include <stdio.h>
#include <string.h>

#define WINDOW_TITLE "RemoteDesktop"
#define PORT "8765"

static int feedback_enabled = 1;

static HWND find_wine_window() {
    return FindWindowA(NULL, WINDOW_TITLE);
}

static void inject_mouse_click(HWND hwnd, int x, int y) {
    LPARAM lParam = (y << 16) | (x & 0xFFFF);
    PostMessage(hwnd, WM_MOUSEMOVE, 0, lParam);
    PostMessage(hwnd, WM_LBUTTONDOWN, MK_LBUTTON, lParam);
    PostMessage(hwnd, WM_LBUTTONUP, 0, lParam);
}

static void inject_key(HWND hwnd, char c) {
    WPARAM vk = VkKeyScan(c);
    PostMessage(hwnd, WM_KEYDOWN, vk, 0);
    PostMessage(hwnd, WM_CHAR, c, 0);
    PostMessage(hwnd, WM_KEYUP, vk, 0);
}

static void send_ack(struct mg_connection *c, const char *type, int x, int y, char key) {
    if (!feedback_enabled) return;
    char msg[128];
    if (type && strcmp(type, "mouse") == 0) {
        snprintf(msg, sizeof(msg), "{\"ack\":\"mouse\",\"x\":%d,\"y\":%d}", x, y);
    } else if (type && strcmp(type, "key") == 0) {
        snprintf(msg, sizeof(msg), "{\"ack\":\"key\",\"char\":\"%c\"}", key);
    } else {
        snprintf(msg, sizeof(msg), "{\"ack\":\"unknown\"}");
    }
    mg_ws_send(c, msg, strlen(msg), WEBSOCKET_OP_TEXT);
}

static void handle_event(struct mg_connection *c, int ev, void *ev_data, void *fn_data) {
    if (ev == MG_EV_WS_MSG) {
        struct mg_ws_message *wm = (struct mg_ws_message *) ev_data;
        HWND hwnd = find_wine_window();
        if (!hwnd) return;

        char buf[256];
        snprintf(buf, sizeof(buf) - 1, "%.*s", (int)wm->data.len, wm->data.ptr);
        buf[sizeof(buf)-1] = '\0';

        if (strstr(buf, "\"type\":\"mouse\"")) {
            int x, y;
            if (sscanf(buf, "%*[^x]:%d,%*[^y]:%d", &x, &y) == 2) {
                inject_mouse_click(hwnd, x, y);
                send_ack(c, "mouse", x, y, 0);
            }
        } else if (strstr(buf, "\"type\":\"key\"")) {
            char ckey = 0;
            char *p = strstr(buf, "\"char\":\"");
            if (p && sscanf(p + 9, "%c", &ckey) == 1) {
                inject_key(hwnd, ckey);
                send_ack(c, "key", 0, 0, ckey);
            }
        }
    }
}

int main(int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--no-feedback") == 0) {
            feedback_enabled = 0;
            printf("🛈 Feedback disabled by --no-feedback flag\n");
        }
    }

    struct mg_mgr mgr;
    mg_mgr_init(&mgr);
    mg_http_listen(&mgr, "ws://0.0.0.0:" PORT, handle_event, NULL);
    printf("✅ WebSocket input bridge running on ws://localhost:%s\n", PORT);
    for (;;) mg_mgr_poll(&mgr, 100);
    mg_mgr_free(&mgr);
    return 0;
}


