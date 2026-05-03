#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <poll.h>
#include <signal.h>
#include "../include/iwakura_net.h"

#define MAX_CLIENTS 10

int udp_sock = -1;
int tcp_server = -1;
int clients[MAX_CLIENTS];

void handle_shutdown(int sig) {
    printf("\n[PAN_HUB] Caught signal %d. Shutting down broadcaster...\n", sig);
    if (udp_sock >= 0) close(udp_sock);
    if (tcp_server >= 0) close(tcp_server);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] >= 0) close(clients[i]);
    }
    exit(0);
}

int main() {
    signal(SIGINT, handle_shutdown);
    signal(SIGTERM, handle_shutdown);

    int udp_port = get_orchestrator_port();
    int tcp_port = get_frontend_port();

    udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
    int opt = 1;
    setsockopt(udp_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in udp_addr = {
        .sin_family = AF_INET,
        .sin_addr.s_addr = htonl(INADDR_LOOPBACK),
        .sin_port = htons(udp_port)
    };
    if (bind(udp_sock, (struct sockaddr *)&udp_addr, sizeof(udp_addr)) < 0) {
        perror("[PAN_HUB] UDP Bind failed");
        return 1;
    }

    tcp_server = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(tcp_server, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in tcp_addr = { .sin_family = AF_INET, .sin_addr.s_addr = INADDR_ANY, .sin_port = htons(tcp_port) };
    if (bind(tcp_server, (struct sockaddr *)&tcp_addr, sizeof(tcp_addr)) < 0) {
        perror("[PAN_HUB] TCP Bind failed");
        return 1;
    }
    listen(tcp_server, 5);

    for (int i = 0; i < MAX_CLIENTS; i++) clients[i] = -1;

    printf("[PAN_HUB] Active! Receiving UDP on %d, Broadcasting TCP on %d...\n", udp_port, tcp_port);

    struct pollfd fds[MAX_CLIENTS + 2];

    while (1) {
        fds[0].fd = udp_sock;     fds[0].events = POLLIN;
        fds[1].fd = tcp_server;   fds[1].events = POLLIN;

        for (int i = 0; i < MAX_CLIENTS; i++) {
            fds[i + 2].fd = clients[i];
            fds[i + 2].events = POLLIN;
        }

        int poll_count = poll(fds, MAX_CLIENTS + 2, -1);
        if (poll_count < 0) continue;

        if (fds[1].revents & POLLIN) {
            struct sockaddr_in client_addr;
            socklen_t addrlen = sizeof(client_addr);
            int new_sock = accept(tcp_server, (struct sockaddr *)&client_addr, &addrlen);

            if (new_sock >= 0) {
                printf("[PAN_HUB] New frontend connected.\n");
                for (int i = 0; i < MAX_CLIENTS; i++) {
                    if (clients[i] == -1) {
                        clients[i] = new_sock;
                        break;
                    }
                }
            }
        }

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i] != -1 && (fds[i + 2].revents & POLLIN)) {
                char dev_null[16];
                if (recv(clients[i], dev_null, sizeof(dev_null), 0) <= 0) {
                    printf("[PAN_HUB] Frontend disconnected.\n");
                    close(clients[i]);
                    clients[i] = -1;
                }
            }
        }

        if (fds[0].revents & POLLIN) {
            char buffer[8192];
            struct sockaddr_in sender_addr;
            socklen_t len = sizeof(sender_addr);
            int n = recvfrom(udp_sock, buffer, sizeof(buffer) - 1, 0, (struct sockaddr *)&sender_addr, &len);

            if (n > 0) {
                buffer[n] = '\0';

                if (buffer[n-1] != '\n') {
                    buffer[n] = '\n';
                    buffer[n+1] = '\0';
                    n++;
                }

                char secret[65];
                populate_auth_token(secret);
                char signed_buffer[8192];
                int signed_len = snprintf(signed_buffer, sizeof(signed_buffer), "%s|%s", secret, buffer);

                for (int i = 0; i < MAX_CLIENTS; i++) {
                    if (clients[i] != -1) {
                        send(clients[i], signed_buffer, signed_len, 0);
                    }
                }
            }
        }
    }
    return 0;
}