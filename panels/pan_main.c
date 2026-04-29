#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "../include/iwakura_net.h"

void fetch_slot(iwakura_req_t type, char *buffer) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in hub_addr = { .sin_family = AF_INET, .sin_port = htons(get_iwakura_port()) };
    inet_pton(AF_INET, get_iwakura_host(), &hub_addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&hub_addr, sizeof(hub_addr)) == 0) {
        iwakura_msg_t msg;
        memset(&msg, 0, sizeof(msg));
        msg.type = type;
        send(sock, &msg, sizeof(msg), 0);

        if (recv(sock, &msg, sizeof(msg), 0) > 0) {
            snprintf(buffer, MAX_PAYLOAD, "%s", msg.payload);
        } else {
            strcpy(buffer, "AWAITING DATA...");
        }
    } else {
        strcpy(buffer, "HUB OFFLINE");
    }
    close(sock);
}

int main() {
    char clock_data[MAX_PAYLOAD], weather_data[MAX_PAYLOAD], guest_data[MAX_PAYLOAD];

    printf("\033[2J\033[?25l");

    while (1) {
        fetch_slot(REQ_CLOCK, clock_data);
        fetch_slot(REQ_WEATHER, weather_data);
        fetch_slot(REQ_GUESTBOOK, guest_data);

        printf("\033[H");

        printf("\033[0;90m┌── IWAKURA CORE ────────────────────────────────────────────────────────┐\033[0m\n\n");

        char *time_str = strtok(clock_data, "|");
        char *date_str = strtok(NULL, "|");
        if (time_str && date_str) {
            printf("    \033[1;32m%s\033[0m \033[0;90m│\033[0m %s\033[K\n\n", time_str, date_str);
        } else {
            printf("    \033[1;32m--:--\033[0m \033[0;90m│\033[0m Syncing...\033[K\n\n");
        }

        char *t = strtok(weather_data, "|");
        char *f = strtok(NULL, "|");
        char *w = strtok(NULL, "|");
        if (t && f && w) {
            printf("    \033[0;36mCLIMA:\033[0m %s°C (Sensación: %s°C) \033[0;90m│\033[0m Viento: %s km/h\033[K\n\n", t, f, w);
        } else {
            printf("    \033[0;36mCLIMA:\033[0m Analizando atmósfera...\033[K\n\n");
        }

        if (strlen(guest_data) > 0) {
            printf("    \033[1;33m“ %s ”\033[0m\033[K\n\n", guest_data);
        } else {
            printf("    \033[1;33m[ Archivo de visitas vacío ]\033[0m\033[K\n\n");
        }

        printf("\033[0;90m└────────────────────────────────────────────────────────────────────────┘\033[0m\n");
        fflush(stdout);

        sleep(1);
    }

    return 0;
}