#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "../include/iwakura_net.h"

int main() {
    char payload[MAX_PAYLOAD];
    printf("\033[2J\033[?25l");

    while (1) {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in hub_addr = { .sin_family = AF_INET, .sin_port = htons(get_iwakura_port()) };
        inet_pton(AF_INET, get_iwakura_host(), &hub_addr.sin_addr);

        strcpy(payload, "Sincronizando Agenda...");

        if (connect(sock, (struct sockaddr *)&hub_addr, sizeof(hub_addr)) == 0) {
            iwakura_msg_t msg;
            memset(&msg, 0, sizeof(msg));
            msg.type = REQ_CAL;
            send(sock, &msg, sizeof(msg), 0);
            if (recv(sock, &msg, sizeof(msg), 0) > 0 && strlen(msg.payload) > 0) {
                strcpy(payload, msg.payload);
            }
        }
        close(sock);

        printf("\033[H\n");
        printf("\033[1;35m  PROTOCOLO DE AGENDA \033[0m\033[0;90m────────────────────────┐\033[0m\n\n");

        char *token = strtok(payload, "|");
        while (token) {
            while(*token == ' ') token++;
            printf("  \033[1;35m•\033[0m %-44s\033[K\n", token);
            token = strtok(NULL, "|");
        }

        printf("\n\033[0;90m  └──────────────────────────────────────────────┘\033[0m\n");
        fflush(stdout);

        sleep(60);
    }
    return 0;
}