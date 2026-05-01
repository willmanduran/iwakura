#include <string.h>
#include "../include/iwakura_orch.h"

void route_packet(char *buffer) {
    if (strncmp(buffer, "CLOCK|", 6) == 0) {
        tui_draw_widget(4, 2, buffer + 6);
    }
    else if (strncmp(buffer, "WEATHER|", 8) == 0) {
        tui_draw_widget(4, 4, buffer + 8);
    }
    else if (strncmp(buffer, "GUESTBOOK|", 10) == 0) {
        tui_draw_widget(4, 10, buffer + 10);
    }
    else if (strncmp(buffer, "NEWS|", 5) == 0) {
        tui_draw_widget(48, 2, buffer + 5);
    }
    else if (strncmp(buffer, "CALENDAR|", 9) == 0) {
        tui_draw_widget(48, 16, buffer + 9);
    }
    else if (strncmp(buffer, "MUSIC|", 6) == 0) {
        tui_draw_widget(4, 24, buffer + 6);
    }
}