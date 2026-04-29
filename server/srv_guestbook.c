#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include <poll.h>
#include "../include/iwakura_net.h"

const char* get_env_str(const char* key, const char* fallback) {
    const char *val = getenv(key);
    return val ? val : fallback;
}

void send_webpage(int socket, int is_success) {
    char html[4096];

    const char* title = get_env_str("GB_TITLE", "Iwakura Guestbook");
    const char* heading = get_env_str("GB_HEADING", "Transmit data to HUD");
    const char* bg = get_env_str("GB_BG_COLOR", "#000000");
    const char* fg = get_env_str("GB_FG_COLOR", "#00ff00");
    const char* btn = get_env_str("GB_BTN_COLOR", "#222222");

    const char* content = is_success ?
        "<div class='thanks'><h2>Transmisión recibida.</h2><p>El mensaje ha sido inyectado en el HUD.</p></div>" :
        "<form method='POST'><input name='n' placeholder='Identificador' required autocomplete='off'><textarea name='m' rows='5' placeholder='Mensaje...' required></textarea><button type='submit'>Transmitir</button></form>";

    snprintf(html, sizeof(html),
        "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=UTF-8\r\n\r\n"
        "<!DOCTYPE html><html><head><title>%s</title>"
        "<meta name='viewport' content='width=device-width, initial-scale=1'>"
        "<style>"
        "body{background:%s;color:%s;font-family:monospace;padding:40px;max-width:500px;margin:auto;line-height:1.6;}"
        "h2{color:%s;border-bottom:1px solid %s;padding-bottom:10px;}"
        "input,textarea{display:block;width:100%%;margin:15px 0;background:#111;color:%s;border:1px solid #333;padding:12px;box-sizing:border-box;}"
        "button{background:%s;color:%s;border:none;padding:12px 20px;cursor:pointer;width:100%%;font-weight:bold;}"
        "button:hover{background:%s;color:%s;}"
        ".thanks{text-align:center;padding:50px;}"
        "</style></head><body><h2>%s</h2>%s</body></html>",
        title, bg, fg, fg, btn, fg, btn, fg, fg, bg, heading, content
    );

    send(socket, html, strlen(html), 0);
}

void decode_url(char *src, char *dest) {
    while (*src) {
        if (*src == '+') *dest = ' ';
        else if (*src == '%' && src[1] && src[2]) {
            int val; sscanf(src + 1, "%2x", &val);
            *dest = (char)val; src += 2;
        } else *dest = *src;
        src++; dest++;
    }
    *dest = '\0';
}

void get_log_path(char *path) {
    char *home = getenv("HOME");
    snprintf(path, 256, "%s/visitas.txt", home ? home : ".");
}

void log_message(char *body) {
    char raw_n[128] = {0}, raw_m[512] = {0}, clean_n[128] = {0}, clean_m[512] = {0};
    char path[256]; get_log_path(path);
    if (sscanf(body, "n=%[^&]&m=%s", raw_n, raw_m) >= 1) {
        decode_url(raw_n, clean_n); decode_url(raw_m, clean_m);
        FILE *f = fopen(path, "a");
        if (f) {
            time_t now = time(NULL);
            struct tm *t = localtime(&now);
            fprintf(f, "%02d/%02d/%02d %02d:%02d | %s: %s\n",
                    t->tm_mday, t->tm_mon + 1, t->tm_year % 100,
                    t->tm_hour, t->tm_min, clean_n, clean_m);
            fclose(f);
            printf("[GUESTBOOK] New entry saved: %s\n", clean_n);
        }
    }
}

void push_random_message_to_hub() {
    char path[256]; get_log_path(path);
    FILE *f = fopen(path, "r");
    if (!f) return;

    char lines[100][256];
    int count = 0;
    while (fgets(lines[count], 256, f) && count < 100) {
        lines[count][strcspn(lines[count], "\n")] = 0;
        count++;
    }
    fclose(f);

    if (count == 0) return;
    int r = rand() % count;

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in hub_addr = { .sin_family = AF_INET, .sin_port = htons(get_iwakura_port()) };
    inet_pton(AF_INET, get_iwakura_host(), &hub_addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&hub_addr, sizeof(hub_addr)) == 0) {
        iwakura_msg_t msg;
        memset(&msg, 0, sizeof(msg));
        msg.type = UPDATE_DATA;
        msg.target_type = REQ_GUESTBOOK;
        snprintf(msg.payload, MAX_PAYLOAD, "%s", lines[r]);
        msg.payload_len = strlen(msg.payload);
        send(sock, &msg, sizeof(msg), 0);
        printf("[GUESTBOOK] Pushed to Hub: %s\n", msg.payload);
    }
    close(sock);
}

int main() {
    srand(time(NULL));
    const char *p = getenv("GUESTBOOK_PORT");
    int http_port = p ? atoi(p) : 9090;

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in address = { .sin_family = AF_INET, .sin_addr.s_addr = INADDR_ANY, .sin_port = htons(http_port) };
    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 5);

    printf("Guestbook HTTP Server active on port %d...\n", http_port);

    struct pollfd fds[1];
    fds[0].fd = server_fd;
    fds[0].events = POLLIN;

    time_t last_push = 0;

    while (1) {
        int ret = poll(fds, 1, 1000);

        if (ret > 0 && (fds[0].revents & POLLIN)) {
            struct sockaddr_in client_addr;
            socklen_t addrlen = sizeof(client_addr);
            int new_socket = accept(server_fd, (struct sockaddr *)&client_addr, &addrlen);

            char buffer[2048] = {0};
            if (read(new_socket, buffer, 2048) > 0) {
                if (strncmp(buffer, "POST", 4) == 0) {
                    char *body = strstr(buffer, "\r\n\r\n");
                    if (body) log_message(body + 4);

                    send_webpage(new_socket, 1);

                    push_random_message_to_hub();
                    last_push = time(NULL);
                } else {
                    send_webpage(new_socket, 0);
                }
            }
            close(new_socket);
        }

        time_t now = time(NULL);
        if (now - last_push >= 60) {
            push_random_message_to_hub();
            last_push = now;
        }
    }
    return 0;
}