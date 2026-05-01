#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../include/iwakura_net.h"

#define BAR_WIDTH 30
#define VIS_WIDTH 24
#define PANE_WIDTH 84

const char *vis_colors[] = {"\033[31m", "\033[32m", "\033[33m", "\033[34m", "\033[35m", "\033[36m"};

void format_time(long ms, char *buf, size_t buf_size) {
    long s = ms / 1000;
    snprintf(buf, buf_size, "%02ld:%02ld", s / 60, s % 60);
}

void get_padding(int visible_len, char *out_buf) {
    int padding = (PANE_WIDTH - visible_len) / 2;
    if (padding < 0) padding = 0;
    memset(out_buf, ' ', padding);
    out_buf[padding] = '\0';
}

int main() {
    int tick = 0;
    char pad[128], pt[32], dt[32], frame[4096];
    char spoti_data[MAX_PAYLOAD], lfm_data[MAX_PAYLOAD];

    while (1) {
        net_fetch_from_hub(REQ_SPOTIFY, spoti_data, "0|None|Silence|0|1");
        net_fetch_from_hub(REQ_LASTFM, lfm_data, _t("L_LFM_EMPTY", "User|Top: N/A|Scrobbles: 0"));

        char *is_play_str = strtok(spoti_data, "|");
        char *artist = strtok(NULL, "|");
        char *track = strtok(NULL, "|");
        char *prog_str = strtok(NULL, "|");
        char *dur_str = strtok(NULL, "|");

        char *l_user = strtok(lfm_data, "|");
        char *l_top = strtok(NULL, "|");
        char *l_scrob = strtok(NULL, "|");

        strcpy(frame, "MUSIC|");

        if (is_play_str && artist && track && prog_str && dur_str && l_user && l_top && l_scrob) {
            int is_playing = atoi(is_play_str);
            long prog = atol(prog_str);
            long dur = atol(dur_str);

            get_padding(76, pad);
            sprintf(frame + strlen(frame), "%s\033[0;90m┌── %-13.13s ─────────────────────────────────────────────────────────┐\033[0m%s\n",
                    pad, is_playing ? _t("L_MUSIC_PLAYING", "NOW PLAYING") : _t("L_MUSIC_PAUSED", "PAUSED"), pad);

            int info_len = strlen(artist) + strlen(track) + 3;
            get_padding(info_len, pad);
            sprintf(frame + strlen(frame), "%s\033[1;32m%s\033[0m - \033[1m%s\033[0m%s\n", pad, artist, track, pad);

            format_time(prog, pt, sizeof(pt));
            format_time(dur, dt, sizeof(dt));
            get_padding(52, pad);
            sprintf(frame + strlen(frame), "%s[\033[32m%s \033[0m", pad, is_playing ? "▶" : "⏸");

            float ratio = dur > 0 ? (float)prog / (float)dur : 0;
            int filled = (int)(ratio * BAR_WIDTH);
            for (int i = 0; i < BAR_WIDTH; ++i) {
                strcat(frame, (i < filled) ? "\033[32m⣿\033[0m" : "\033[2m⣀\033[0m");
            }
            sprintf(frame + strlen(frame), "] \033[1;32m%s\033[0m\033[2m / %s\033[0m%s\n", pt, dt, pad);

            get_padding(VIS_WIDTH, pad);
            strcat(frame, pad);
            const char *levels[] = {" ", "▂", "▃", "▄", "▅", "▆", "▇", "█"};
            const char *current_color = vis_colors[(tick / 4) % 6];
            for (int i = 0; i < VIS_WIDTH; i++) {
                int lvl = is_playing ? (rand() % 8) : 0;
                sprintf(frame + strlen(frame), "%s%s\033[0m", current_color, levels[lvl]);
            }
            sprintf(frame + strlen(frame), "%s\n", pad);

            char footer_text[256];
            snprintf(footer_text, sizeof(footer_text), " %s  │  %s  │  %s ", l_user, l_top, l_scrob);
            int footer_len = strlen(l_user) + strlen(l_top) + strlen(l_scrob) + 10;
            get_padding(footer_len, pad);
            sprintf(frame + strlen(frame), "%s\033[7m%s\033[0m%s\n", pad, footer_text, pad);

            get_padding(76, pad);
            sprintf(frame + strlen(frame), "%s\033[0;90m└────────────────────────────────────────────────────────────────────────┘\033[0m%s", pad, pad);
        }

        net_push_to_orchestrator(frame);
        tick++;
        usleep(100000);
    }
    return 0;
}