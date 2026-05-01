#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <arpa/inet.h>
#include "include/iwakura_net.h"
#include "include/iwakura_orch.h"

int server_sock = -1;

void handle_shutdown(int sig) {
    tui_cleanup();
    printf("\n[ORCHESTRATOR] Caught signal %d. Releasing port and shutting down...\n", sig);
    net_close_udp_server(server_sock);
    exit(0);
}

int main() {
    signal(SIGINT, handle_shutdown);
    signal(SIGTERM, handle_shutdown);

    int port = get_orchestrator_port();
    server_sock = net_init_udp_server(port);

    if (server_sock < 0) {
        perror("Socket creation failed");
        return 1;
    }

    tui_init();

    char buffer[8192];
    struct sockaddr_in client_addr;
    socklen_t len = sizeof(client_addr);

    while (1) {
        int n = recvfrom(server_sock, buffer, sizeof(buffer) - 1, 0, (struct sockaddr *)&client_addr, &len);
        if (n > 0) {
            buffer[n] = '\0';
            route_packet(buffer);
        }
    }

    return 0;
}