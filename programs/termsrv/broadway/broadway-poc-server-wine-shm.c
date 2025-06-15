// minimal_broadway_poc.c
// Extended version: integrates with shared memory to receive RGBA framebuffer
// This assumes a Wine-based GDI driver is writing raw RGBA32 into shared memory

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#define PORT 8080
#define WIDTH 800
#define HEIGHT 600
#define SHM_NAME "/winerds_framebuffer"
#define FRAMEBUFFER_SIZE (WIDTH * HEIGHT * 4)

unsigned char *framebuffer = NULL;

void *client_thread(void *arg) {
    int client_sock = *(int *)arg;
    free(arg);

    char request[2048];
    read(client_sock, request, sizeof(request));

    if (strstr(request, "GET /framebuffer")) {
        // Simulate sending the RGBA framebuffer
        char header[256];
        snprintf(header, sizeof(header),
                 "HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\nContent-Length: %d\r\n\r\n",
                 FRAMEBUFFER_SIZE);
        send(client_sock, header, strlen(header), 0);
        send(client_sock, framebuffer, FRAMEBUFFER_SIZE, 0);
    } else {
        const char *html = "<!DOCTYPE html><html><body><canvas id=\"c\"></canvas>"
            "<script>"
            "let c=document.getElementById('c');c.width=800;c.height=600;let x=c.getContext('2d');"
            "fetch('/framebuffer').then(r=>r.arrayBuffer()).then(b=>{"
            "let i=new ImageData(new Uint8ClampedArray(b),800,600);x.putImageData(i,0,0);});"
            "setInterval(()=>fetch('/framebuffer').then(r=>r.arrayBuffer()).then(b=>{"
            "let i=new ImageData(new Uint8ClampedArray(b),800,600);x.putImageData(i,0,0);}), 500);"
            "</script></body></html>";

        char header[256];
        snprintf(header, sizeof(header),
                 "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: %ld\r\n\r\n",
                 strlen(html));
        send(client_sock, header, strlen(header), 0);
        send(client_sock, html, strlen(html), 0);
    }

    close(client_sock);
    return NULL;
}

int main() {
    int shm_fd = shm_open(SHM_NAME, O_RDONLY, 0666);
    if (shm_fd < 0) {
        perror("shm_open");
        exit(1);
    }

    framebuffer = mmap(0, FRAMEBUFFER_SIZE, PROT_READ, MAP_SHARED, shm_fd, 0);
    if (framebuffer == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(server_sock, (struct sockaddr *)&addr, sizeof(addr));
    listen(server_sock, 5);

    printf("Broadway POC server running on http://localhost:%d\n", PORT);

    while (1) {
        struct sockaddr_in client;
        socklen_t len = sizeof(client);
        int *client_sock = malloc(sizeof(int));
        *client_sock = accept(server_sock, (struct sockaddr *)&client, &len);

        pthread_t tid;
        pthread_create(&tid, NULL, client_thread, client_sock);
        pthread_detach(tid);
    }

    return 0;
}


