#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "../include/iwakura_net.h"

int main() {
    char raw_data[MAX_PAYLOAD];
    char display_frame[4096];

    while (1) {
        net_fetch_from_hub(REQ_CAL, raw_data, _t("L_CAL_SYNC", "Updating calendar..."));

        strcpy(display_frame, "CALENDAR|");
        char header[256];
        snprintf(header, sizeof(header), "\033[1;35m  %-30.30s \033[0m\n\n", _t("L_CAL_HEADER", "CALENDAR"));
        strcat(display_frame, header);

        char *token = strtok(raw_data, "|");
        while (token) {
            while(*token == ' ') token++;
            char line[512];
            snprintf(line, sizeof(line), "  \033[1;35m•\033[0m %-40.40s \n", token);

            if (strlen(display_frame) + strlen(line) < sizeof(display_frame) - 200) {
                strcat(display_frame, line);
            }
            token = strtok(NULL, "|");
        }

        net_push_to_orchestrator(display_frame);
        sleep(2);
    }
    return 0;
}