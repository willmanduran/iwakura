#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include "../include/iwakura_net.h"

int main() {
    printf("[CLOCK] Provider starting...\n");

    while (1) {
        time_t now = time(NULL);
        struct tm *t = localtime(&now);

        char payload[MAX_PAYLOAD];
        strftime(payload, MAX_PAYLOAD, "%H:%M|%A, %d %B", t);

        net_push_to_hub(REQ_CLOCK, payload);

        sleep(1);
    }
    return 0;
}