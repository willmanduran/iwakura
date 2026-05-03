#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <poll.h>
#include <signal.h>
#include "../include/iwakura_net.h"

char state_table[10][MAX_PAYLOAD];
int server_fd;

void handle_shutdown(int sig) {
    printf("\n[HUB] Caught signal %d. Releasing port and shutting down...\n", sig);
    if (server_fd >= 0) {
        close(server_fd);
    }
    exit(0);
}

void handle_client(int fd) {
    iwakura_msg_t msg;
    int bytes = recv(fd, &msg, sizeof(iwakura_msg_t), 0);

    if (bytes <= 0) {
        perror("[HUB] recv failed or client disconnected");
        return;
    }

    msg.payload[MAX_PAYLOAD - 1] = '\0';

    printf("[HUB] Received %d bytes. Type requested: %d\n", bytes, msg.type);

    if (msg.type == UPDATE_DATA) {
        int sub_type = msg.target_type;
        snprintf(state_table[sub_type], MAX_PAYLOAD, "%s", msg.payload);
        printf("[HUB] Updated state for slot %d with payload: %s\n", sub_type, msg.payload);
    } else {
        iwakura_msg_t response;
        memset(&response, 0, sizeof(iwakura_msg_t));
        response.type = msg.type;
        snprintf(response.payload, MAX_PAYLOAD, "%s", state_table[msg.type]);
        response.payload_len = strlen(response.payload);

        send(fd, &response, sizeof(iwakura_msg_t), 0);
        printf("[HUB] Sent response to panel for slot %d\n", msg.type);
    }
}

int main() {
    signal(SIGINT, handle_shutdown);
    signal(SIGTERM, handle_shutdown);

    int port = get_iwakura_port();
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr = { .sin_family = AF_INET, .sin_addr.s_addr = INADDR_ANY, .sin_port = htons(port) };

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("[HUB] Bind failed (is the port already in use?)");
        return 1;
    }

    listen(server_fd, 10);
    printf("Iwakura Hub active on port %d...\n", port);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &len);
        if (client_fd >= 0) {
            handle_client(client_fd);
            close(client_fd);
        }
    }
    return 0;
}