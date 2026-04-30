#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>
#include "../include/iwakura_net.h"

int server_sock = -1;

void handle_shutdown(int sig) {
    printf("\n\033[?25h\033[0m");
    printf("\n[ORCHESTRATOR] Caught signal %d. Releasing port and shutting down...\n", sig);
    if (server_sock >= 0) {
        close(server_sock);
    }
    exit(0);
}

void draw_clock(char *payload) {
    char *time_str = strtok(payload, "|");
    char *date_str = strtok(NULL, "|");

    if (time_str && date_str) {
        printf("\033[2;4H\033[1;32m%s\033[0m \033[0;90m│ %s\033[0m\033[K", time_str, date_str);
        fflush(stdout);
    }
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

    printf("\033[2J\033[?25l");
    fflush(stdout);

    char buffer[MAX_PAYLOAD];
    struct sockaddr_in client_addr;
    socklen_t len = sizeof(client_addr);

    while (1) {
        int n = recvfrom(server_sock, buffer, MAX_PAYLOAD - 1, 0, (struct sockaddr *)&client_addr, &len);
        if (n > 0) {
            buffer[n] = '\0';

            if (strncmp(buffer, "CLOCK|", 6) == 0) {
                draw_clock(buffer + 6);
            }
        }
    }

    return 0;
}