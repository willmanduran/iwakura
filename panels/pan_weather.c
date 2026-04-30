#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../include/iwakura_net.h"

int main() {
    char raw_data[MAX_PAYLOAD];
    char display_frame[MAX_PAYLOAD];

    while (1) {
        net_fetch_from_hub(REQ_WEATHER, raw_data, "0|0|0|0|0");

        char *t = strtok(raw_data, "|");
        char *f = strtok(NULL, "|");
        char *w = strtok(NULL, "|");

        if (t && f && w) {
            snprintf(display_frame, MAX_PAYLOAD,
                "WEATHER|"
                "\033[0;36m%s\033[0m\n"
                "Temp:  %s°C\n"
                "Feels: %s°C\n"
                "Wind:  %s km/h",
                _t("L_WX_WEATHER", "WEATHER"), t, f, w);
        } else {
            snprintf(display_frame, MAX_PAYLOAD, "WEATHER|Updating...");
        }

        net_push_to_orchestrator(display_frame);

        sleep(15);
    }
    return 0;
}