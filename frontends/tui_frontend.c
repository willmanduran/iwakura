#include <stdio.h>
#include "../include/iwakura_orch.h"

void tui_init() {
    printf("\033[2J\033[?25l\033[?7l");
    fflush(stdout);
}

void tui_cleanup() {
    printf("\n\033[?25h\033[?7h\033[0m");
    fflush(stdout);
}

void tui_draw_widget(int start_x, int start_y, char *payload) {
    int cx = start_x;
    int cy = start_y;

    printf("\033[%d;%dH", cy, cx);

    for (int i = 0; payload[i] != '\0'; i++) {
        if (payload[i] == '\n') {
            cy++;
            printf("\033[%d;%dH", cy, cx);
        } else {
            putchar(payload[i]);
        }
    }
    fflush(stdout);
}