#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "../include/iwakura_net.h"

int main() {
    char spoti_data[MAX_PAYLOAD];
    char lfm_data[MAX_PAYLOAD];
    char display_frame[4096];

    while (1) {
        net_fetch_from_hub(REQ_SPOTIFY, spoti_data, "0|None|Silence|0|1");
        net_fetch_from_hub(REQ_LASTFM, lfm_data, _t("L_LFM_EMPTY", "User|Top: N/A|Scrobbles: 0"));

        snprintf(display_frame, sizeof(display_frame), "MUSIC|%s|%s", spoti_data, lfm_data);
        net_push_to_orchestrator(display_frame);

        sleep(1);
    }
    return 0;
}