#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>
#include <time.h>
#include "../include/iwakura_net.h"

#define BAR_WIDTH 30
#define VIS_WIDTH 24
#define PANE_WIDTH 84

int sock = -1;
int tick = 0;
const char *vis_colors[] = {"\033[31m", "\033[32m", "\033[33m", "\033[34m", "\033[35m", "\033[36m"};

void tui_init() {
    printf("\033[2J\033[?25l\033[?7l");
    fflush(stdout);
}

void tui_cleanup() {
    printf("\n\033[?25h\033[?7h\033[0m");
    fflush(stdout);
}

void tui_draw_widget(int start_x, int start_y, const char *payload) {
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

void handle_shutdown(int sig) {
    (void)sig;
    tui_cleanup();
    if (sock >= 0) close(sock);
    exit(0);
}

void load_env(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) return;
    char line[512];
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '#' || line[0] == '\n') continue;
        char *key = strtok(line, "=");
        char *val = strtok(NULL, "\n");
        if (key && val) {
            if (val[0] == '"') val++;
            if (val[strlen(val)-1] == '"') val[strlen(val)-1] = '\0';
            setenv(key, val, 1);
        }
    }
    fclose(f);
}

void render_clock(char *data) {
    char out[MAX_PAYLOAD];
    char *time_str = strtok(data, "|");
    char *date_str = strtok(NULL, "|");
    if (time_str && date_str) {
        snprintf(out, sizeof(out), "\033[1;32m%s\033[0m \033[1;30m|\033[0m %s                                        ", time_str, date_str);
    }
    tui_draw_widget(4, 2, out);
}

void render_weather(char *data) {
    char out[MAX_PAYLOAD];
    char *t = strtok(data, "|");
    char *f = strtok(NULL, "|");
    char *w = strtok(NULL, "|");
    if (t && f && w) {
        snprintf(out, sizeof(out),
            "\033[0;36m%s\033[0m                                        \n"
            "Temp:  %s°C                                        \n"
            "Feels: %s°C                                        \n"
            "Wind:  %s km/h                                     ",
            _t("L_WX_WEATHER", "WEATHER"), t, f, w);
        tui_draw_widget(4, 4, out);
    }
}

void render_news(char *data) {
    char out[4096];
    snprintf(out, sizeof(out), "\033[0;90m┌── %-56.56s ┐\033[0m\n", _t("L_NEWS_HEADER", "NEWS"));
    char *token = strtok(data, "|");
    int count = 0;
    while (token && count < 10) {
        char line[512];
        snprintf(line, sizeof(line), " \033[0;90m»\033[0m %-60.60s \n", token);
        strcat(out, line);
        token = strtok(NULL, "|");
        count++;
    }
    strcat(out, "\033[0;90m└──────────────────────────────────────────────────────────┘\033[0m");
    tui_draw_widget(48, 2, out);
}

void render_calendar(char *data) {
    char out[4096];
    snprintf(out, sizeof(out), "\033[1;35m  %-30.30s \033[0m\n\n", _t("L_CAL_HEADER", "CALENDAR"));
    char *token = strtok(data, "|");
    while (token) {
        while(*token == ' ') token++;
        char line[512];
        snprintf(line, sizeof(line), "  \033[1;35m•\033[0m %-40.40s \n", token);
        strcat(out, line);
        token = strtok(NULL, "|");
    }
    tui_draw_widget(48, 16, out);
}

void render_guestbook(char *data) {
    char out[MAX_PAYLOAD];
    snprintf(out, sizeof(out), "\033[1;33m“ %s ”\033[0m                                                            ", data);
    tui_draw_widget(4, 10, out);
}

void render_music(char *data) {
    char out[4096] = "";
    char *is_play_str = strtok(data, "|");
    char *artist = strtok(NULL, "|");
    char *track = strtok(NULL, "|");
    char *prog_str = strtok(NULL, "|");
    char *dur_str = strtok(NULL, "|");
    char *l_user = strtok(NULL, "|");
    char *l_top = strtok(NULL, "|");
    char *l_scrob = strtok(NULL, "|");

    if (is_play_str && artist && track && prog_str && dur_str && l_user && l_top && l_scrob) {
        int is_playing = atoi(is_play_str);
        long prog = atol(prog_str);
        long dur = atol(dur_str);

        sprintf(out + strlen(out), "\033[0;90m┌── %-13.13s ─────────────────────────────────────────────────────────┐\033[0m\n",
                is_playing ? _t("L_MUSIC_PLAYING", "NOW PLAYING") : _t("L_MUSIC_PAUSED", "PAUSED"));
        sprintf(out + strlen(out), "   \033[1;32m%s\033[0m - \033[1m%s\033[0m\n", artist, track);

        float ratio = dur > 0 ? (float)prog / (float)dur : 0;
        int filled = (int)(ratio * BAR_WIDTH);
        strcat(out, "   [");
        for (int i = 0; i < BAR_WIDTH; ++i) strcat(out, (i < filled) ? "\033[32m⣿\033[0m" : "\033[2m⣀\033[0m");
        strcat(out, "]   ");

        const char *levels[] = {" ", "▂", "▃", "▄", "▅", "▆", "▇", "█"};
        const char *current_color = vis_colors[(tick / 4) % 6];
        for (int i = 0; i < VIS_WIDTH; i++) {
            int lvl = is_playing ? (rand() % 8) : 0;
            sprintf(out + strlen(out), "%s%s\033[0m", current_color, levels[lvl]);
        }
        sprintf(out + strlen(out), "\n   %s  │  %s  │  %s \n", l_user, l_top, l_scrob);
        strcat(out, "\033[0;90m└────────────────────────────────────────────────────────────────────────┘\033[0m");
        tui_draw_widget(4, 24, out);
    }
}

void process_stream(char *buffer) {
    tick++;
    if (strncmp(buffer, "CLOCK|", 6) == 0) render_clock(buffer + 6);
    else if (strncmp(buffer, "WEATHER|", 8) == 0) render_weather(buffer + 8);
    else if (strncmp(buffer, "NEWS|", 5) == 0) render_news(buffer + 5);
    else if (strncmp(buffer, "CALENDAR|", 9) == 0) render_calendar(buffer + 9);
    else if (strncmp(buffer, "GUESTBOOK|", 10) == 0) render_guestbook(buffer + 10);
    else if (strncmp(buffer, "MUSIC|", 6) == 0) render_music(buffer + 6);
}

int main() {
    signal(SIGINT, handle_shutdown);
    signal(SIGTERM, handle_shutdown);
    srand(time(NULL));

    load_env(".env");
    const char *lang = getenv("IWAKURA_LANG");
    if (lang) {
        char lang_path[256];
        snprintf(lang_path, sizeof(lang_path), "lang/%s.env", lang);
        load_env(lang_path);
    } else {
        load_env("lang/en.env");
    }

    sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr = { .sin_family = AF_INET, .sin_port = htons(get_frontend_port()) };
    inet_pton(AF_INET, get_iwakura_host(), &serv_addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("Connection to PAN_HUB failed.\n");
        return 1;
    }

    tui_init();
    char buffer[8192];
    int bytes_read;
    char line[8192];
    int line_len = 0;

    while ((bytes_read = recv(sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
        for (int i = 0; i < bytes_read; i++) {
            if (buffer[i] == '\n' || buffer[i] == '\0') {
                line[line_len] = '\0';
                if (line_len > 0) process_stream(line);
                line_len = 0;
            } else {
                if (line_len < (int)sizeof(line) - 1) line[line_len++] = buffer[i];
            }
        }
    }
    handle_shutdown(0);
    return 0;
}