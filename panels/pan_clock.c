#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "../include/iwakura_net.h"

int main() {
    char raw_data[MAX_PAYLOAD];
    char display_frame[MAX_PAYLOAD];

    while (1) {
        net_fetch_from_hub(REQ_CLOCK, raw_data, "--:--|Syncing...");

        char *time_str = strtok(raw_data, "|");
        char *date_str = strtok(NULL, "|");

        if (time_str && date_str) {
            snprintf(display_frame, MAX_PAYLOAD, "CLOCK|\033[1;32m%s\033[0m \033[1;30m|\033[0m %s                                        ", time_str, date_str);
        } else {
            snprintf(display_frame, MAX_PAYLOAD, "CLOCK|Syncing...                                        ");
        }

        net_push_to_orchestrator(display_frame);
        sleep(1);
    }

    return 0;
}