#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "../include/iwakura_net.h"

int main() {
    char raw_data[MAX_PAYLOAD];
    char display_frame[4096];

    while (1) {
        net_fetch_from_hub(REQ_NEWS, raw_data, _t("L_NEWS_SEARCH", "Loading news feed..."));

        strcpy(display_frame, "NEWS|");
        char header[256];
        snprintf(header, sizeof(header), "\033[0;90m┌── %-56.56s ┐\033[0m\n", _t("L_NEWS_HEADER", "NEWS"));
        strcat(display_frame, header);

        char *token = strtok(raw_data, "|");
        int count = 0;

        while (token && count < 10) {
            char line[512];
            snprintf(line, sizeof(line), " \033[0;90m»\033[0m %-60.60s \n", token);

            if (strlen(display_frame) + strlen(line) < sizeof(display_frame) - 200) {
                strcat(display_frame, line);
            }
            token = strtok(NULL, "|");
            count++;
        }

        strcat(display_frame, "\033[0;90m└──────────────────────────────────────────────────────────┘\033[0m");

        net_push_to_orchestrator(display_frame);
        sleep(2);
    }
    return 0;
}