#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "../include/iwakura_net.h"

int main() {
    char raw_data[MAX_PAYLOAD];
    char display_frame[MAX_PAYLOAD + 16];

    while (1) {
        net_fetch_from_hub(REQ_WEATHER, raw_data, "0|0|0|0|0");

        snprintf(display_frame, sizeof(display_frame), "WEATHER|%s", raw_data);
        net_push_to_orchestrator(display_frame);

        sleep(2);
    }
    return 0;
}