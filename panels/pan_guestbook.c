#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "../include/iwakura_net.h"

int main() {
    char raw_data[MAX_PAYLOAD];
    char display_frame[MAX_PAYLOAD];

    while (1) {
        net_fetch_from_hub(REQ_GUESTBOOK, raw_data, "");

        if (strlen(raw_data) > 0) {
            snprintf(display_frame, MAX_PAYLOAD, "GUESTBOOK|\033[1;33m“ %s ”\033[0m                                                            ", raw_data);
        } else {
            snprintf(display_frame, MAX_PAYLOAD, "GUESTBOOK|\033[1;33m[ %s ]\033[0m                                                            ", _t("L_GB_EMPTY", "No messages yet"));
        }

        net_push_to_orchestrator(display_frame);
        sleep(2);
    }
    return 0;
}