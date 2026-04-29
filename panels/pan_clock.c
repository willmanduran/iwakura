#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "../include/iwakura_net.h"

int main() {
    int port = get_iwakura_port();
    const char *host = get_iwakura_host();

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr = { .sin_family = AF_INET, .sin_port = htons(port) };
    inet_pton(AF_INET, host, &serv_addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Panel failed to connect to Hub");
        return 1;
    }

    iwakura_msg_t msg;
    memset(&msg, 0, sizeof(iwakura_msg_t));
    msg.type = REQ_CLOCK;

    send(sock, &msg, sizeof(iwakura_msg_t), 0);

    int bytes = recv(sock, &msg, sizeof(iwakura_msg_t), 0);
    if (bytes > 0) {
        printf("\033[1;32m[%s]\033[0m\n", msg.payload);
    } else {
        perror("Panel failed to receive data from Hub");
    }

    close(sock);
    return 0;
}