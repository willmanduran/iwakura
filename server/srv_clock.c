#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include "../include/iwakura_net.h"

int main() {
    int port = get_iwakura_port();
    const char *host = get_iwakura_host();
    struct sockaddr_in serv_addr = { .sin_family = AF_INET, .sin_port = htons(port) };
    inet_pton(AF_INET, host, &serv_addr.sin_addr);

    printf("[CLOCK] Provider starting... targeting %s:%d\n", host, port);

    while (1) {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) >= 0) {
            time_t now = time(NULL);
            struct tm *t = localtime(&now);

            iwakura_msg_t msg;
            memset(&msg, 0, sizeof(msg));
            msg.type = UPDATE_DATA;
            msg.target_type = REQ_CLOCK;

            strftime(msg.payload, MAX_PAYLOAD, "%H:%M|%A, %d %B", t);
            msg.payload_len = strlen(msg.payload);

            send(sock, &msg, sizeof(msg), 0);
            close(sock);
        }
        sleep(1);
    }
    return 0;
}