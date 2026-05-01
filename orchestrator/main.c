#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>
#include "../include/iwakura_net.h"

int server_sock = -1;

void handle_shutdown(int sig) {
    printf("\n\033[?25h\033[?7h\033[0m");
    printf("\n[ORCHESTRATOR] Caught signal %d. Releasing port and shutting down...\n", sig);
    if (server_sock >= 0) {
        close(server_sock);
    }
    exit(0);
}

void draw_widget(int start_x, int start_y, char *payload) {
    int cx = start_x;
    int cy = start_y;

    printf("\033[%d;%dH", cy, cx);

    for (int i = 0; payload[i] != '\0'; i++) {
        if (payload[i] == '\n') {
            cy++;
            printf("\033[%d;%dH", cy, cx);
        } else {
            putchar(payload[i]);
        }
    }
    fflush(stdout);
}

int main() {
    signal(SIGINT, handle_shutdown);
    signal(SIGTERM, handle_shutdown);

    int port = get_orchestrator_port();
    server_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_sock < 0) {
        perror("Socket creation failed");
        return 1;
    }

    int opt = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_addr.s_addr = INADDR_ANY,
        .sin_port = htons(port)
    };

    if (bind(server_sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Bind failed");
        return 1;
    }

    printf("\033[2J\033[?25l\033[?7l");
    fflush(stdout);

    char buffer[8192];
    struct sockaddr_in client_addr;
    socklen_t len = sizeof(client_addr);

    while (1) {
        int n = recvfrom(server_sock, buffer, sizeof(buffer) - 1, 0, (struct sockaddr *)&client_addr, &len);
        if (n > 0) {
            buffer[n] = '\0';

            if (strncmp(buffer, "CLOCK|", 6) == 0) {
                draw_widget(4, 2, buffer + 6);
            }
            else if (strncmp(buffer, "WEATHER|", 8) == 0) {
                draw_widget(4, 4, buffer + 8);
            }
            else if (strncmp(buffer, "GUESTBOOK|", 10) == 0) {
                draw_widget(4, 10, buffer + 10);
            }
            else if (strncmp(buffer, "NEWS|", 5) == 0) {
                draw_widget(48, 2, buffer + 5);
            }
            else if (strncmp(buffer, "CALENDAR|", 9) == 0) {
                draw_widget(48, 16, buffer + 9);
            }
            else if (strncmp(buffer, "MUSIC|", 6) == 0) {
                draw_widget(4, 24, buffer + 6);
            }
        }
    }

    return 0;
}