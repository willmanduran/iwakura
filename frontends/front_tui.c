#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>
#include <time.h>
#include <sys/ioctl.h>
#include "../include/iwakura_net.h"

#define BAR_WIDTH 30
#define VIS_WIDTH 24

int sock = -1;
int tick = 0;
int last_cols = 0, last_rows = 0;

char raw_clock[256] = "00:00|Syncing...";
char raw_weather[256] = "0|0|0|0|0";
char raw_news[4096] = "";
char raw_calendar[4096] = "";
char raw_guestbook[512] = "No messages yet.";
char raw_music[1024] = "0|None|Silence|0|1|User|N/A|0";

const char *digit_font[11][5] = {
    {"###", "# #", "# #", "# #", "###"},
    {" # ", "## ", " # ", " # ", "###"},
    {"###", "  #", "###", "#  ", "###"},
    {"###", "  #", "###", "  #", "###"},
    {"# #", "# #", "###", "  #", "  #"},
    {"###", "#  ", "###", "  #", "###"},
    {"###", "#  ", "###", "# #", "###"},
    {"###", "  #", "  #", "  #", "  #"},
    {"###", "# #", "###", "# #", "###"},
    {"###", "# #", "###", "  #", "###"},
    {"   ", " * ", "   ", " * ", "   "}
};

void tui_init() {
    printf("\033[2J\033[?25l\033[?7l");
    fflush(stdout);
}

void tui_cleanup() {
    printf("\n\033[?25h\033[?7h\033[0m\033[2J");
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

void append_centered(char *out, int width, int vis_len, const char *content) {
    int pad = (width - vis_len) / 2;
    if (pad < 0) pad = 0;
    for (int i = 0; i < pad; i++) strcat(out, " ");
    strcat(out, content);
    strcat(out, "\033[K\n");
}

void render_clock(int y, int cols) {
    char buf[256]; strcpy(buf, raw_clock);
    char *time_str = strtok(buf, "|");
    char *date_str = strtok(NULL, "|");
    if (!time_str || !date_str) return;

    int date_len = strlen(date_str);
    int date_x = (cols - date_len) / 2;
    char out_date[256];
    snprintf(out_date, sizeof(out_date), "\033[1;37m%s\033[0m\033[K", date_str);
    tui_draw_widget(date_x, y, out_date);

    int time_len = strlen(time_str);
    int total_width = (time_len * 3) + (time_len - 1);
    int start_x = (cols - total_width) / 2;

    for (int r = 0; r < 5; r++) {
        printf("\033[%d;%dH\033[1;36m", y + 2 + r, start_x);
        for (int i = 0; i < time_len; i++) {
            int idx = (time_str[i] == ':') ? 10 : (time_str[i] - '0');
            printf("%s ", digit_font[idx][r]);
        }
        printf("\033[0m\033[K");
    }
}

void render_weather(int y, int cols) {
    char buf[256]; strcpy(buf, raw_weather);
    char *t = strtok(buf, "|");
    char *f = strtok(NULL, "|");
    char *w = strtok(NULL, "|");
    if (!t || !f || !w) return;

    char str[128];
    snprintf(str, sizeof(str), "Temp %s°C  |  Feels %s°C  |  Wind %s km/h", t, f, w);
    int x = (cols - strlen(str)) / 2;

    char out[256];
    snprintf(out, sizeof(out), "\033[0;90m%s\033[0m\033[K", str);
    tui_draw_widget(x, y, out);
}

void render_guestbook(int y, int cols) {
    char str[1024];
    snprintf(str, sizeof(str), "“ %s ”", raw_guestbook);

    int len = strlen(str);
    if (len > 120) len = 120;
    int x = (cols - len) / 2;

    char out[2048];
    snprintf(out, sizeof(out), "\033[38;5;216m%s\033[0m\033[K", str);
    tui_draw_widget(x, y, out);
}

void render_news(int x, int y, int width) {
    char buf[4096]; strcpy(buf, raw_news);
    char out[4096] = "";
    int hr_len = width - 15;
    if (hr_len < 5) hr_len = 5;

    char header[256];
    snprintf(header, sizeof(header), "\033[0;90m┌── \033[1;37m%-10.10s\033[0;90m ", _t("L_NEWS_HEADER", "NEWS"));
    strcpy(out, header);
    for(int i = 0; i < hr_len; i++) strcat(out, "─");
    strcat(out, "\033[0m\033[K\n");

    char *token = strtok(buf, "|");
    int count = 0;
    while (token && count < 8) {
        char line[512];
        snprintf(line, sizeof(line), " \033[0;90m»\033[0m \033[38;5;250m%.*s\033[0m\033[K\n", width - 6, token);
        strcat(out, line);
        token = strtok(NULL, "|");
        count++;
    }
    tui_draw_widget(x, y, out);
}

void render_calendar(int x, int y, int width) {
    char buf[4096]; strcpy(buf, raw_calendar);
    char out[4096] = "";
    int hr_len = width - 15;
    if (hr_len < 5) hr_len = 5;

    char header[256];
    snprintf(header, sizeof(header), "\033[1;35m┌── \033[1;35m%-10.10s\033[1;35m ", _t("L_CAL_HEADER", "CALENDAR"));
    strcpy(out, header);
    for(int i = 0; i < hr_len; i++) strcat(out, "─");
    strcat(out, "\033[0m\033[K\n");

    char *token = strtok(buf, "|");
    int count = 0;
    while (token && count < 8) {
        while(*token == ' ') token++;
        char line[512];
        snprintf(line, sizeof(line), " \033[1;35m•\033[0m \033[38;5;250m%.*s\033[0m\033[K\n", width - 6, token);
        strcat(out, line);
        token = strtok(NULL, "|");
        count++;
    }
    tui_draw_widget(x, y, out);
}

void render_music(int x, int y, int width) {
    char buf[1024]; strcpy(buf, raw_music);
    char out[4096] = "";
    char line_buf[1024];

    char *is_play_str = strtok(buf, "|");
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

        const char* title = is_playing ? _t("L_MUSIC_PLAYING", "REPRODUCIENDO") : _t("L_MUSIC_PAUSED", "PAUSED");
        sprintf(line_buf, "\033[1;32m%s \033[0;90m── \033[1;37m%s\033[0;90m ──\033[0m", is_playing ? "►" : "■", title);
        append_centered(out, width, 8 + strlen(title), line_buf);
        strcat(out, "\n");

        sprintf(line_buf, "\033[1;32m%s\033[0m \033[0;90m-\033[0m \033[1;37m%s\033[0m", artist, track);
        append_centered(out, width, strlen(artist) + 3 + strlen(track), line_buf);
        strcat(out, "\n");

        char p_time[32], d_time[32];
        long p_s = prog / 1000;
        long d_s = dur / 1000;
        snprintf(p_time, sizeof(p_time), "%02ld:%02ld", p_s / 60, p_s % 60);
        snprintf(d_time, sizeof(d_time), "%02ld:%02ld", d_s / 60, d_s % 60);

        strcpy(line_buf, "\033[0;90m[\033[0m");
        float ratio = dur > 0 ? (float)prog / (float)dur : 0;
        int filled = (int)(ratio * BAR_WIDTH);
        for (int i = 0; i < BAR_WIDTH; ++i) {
            strcat(line_buf, (i < filled) ? "\033[1;32m⣿\033[0m" : "\033[0;90m⣀\033[0m");
        }
        sprintf(line_buf + strlen(line_buf), "\033[0;90m] \033[1;32m%s \033[0;90m/ \033[1;37m%s\033[0m", p_time, d_time);
        append_centered(out, width, 1 + BAR_WIDTH + 2 + 5 + 3 + 5, line_buf);
        strcat(out, "\n");

        const char *levels[] = {" ", "▂", "▃", "▄", "▅", "▆", "▇", "█"};
        line_buf[0] = '\0';
        for (int i = 0; i < VIS_WIDTH; i++) {
            int lvl = is_playing ? (rand() % 8) : 0;
            sprintf(line_buf + strlen(line_buf), "\033[38;5;137m%s\033[0m", levels[lvl]);
        }
        append_centered(out, width, VIS_WIDTH, line_buf);
        strcat(out, "\n");

        char f1[128], f2[128], f3[128];
        snprintf(f1, sizeof(f1), " %s ", l_user);
        snprintf(f3, sizeof(f3), " %s ", l_scrob);

        int used_width = strlen(f1) + strlen(f3) + 2;
        int max_top_len = width - used_width - 2;

        if (max_top_len < 3) {
            snprintf(f2, sizeof(f2), " ... ");
        } else if ((int)strlen(l_top) > max_top_len) {
            snprintf(f2, sizeof(f2), " %.*s... ", max_top_len - 3, l_top);
        } else {
            snprintf(f2, sizeof(f2), " %.*s ", 120, l_top);
        }

        int ft_vis_len = strlen(f1) + 1 + strlen(f2) + 1 + strlen(f3);
        sprintf(line_buf, "\033[30;47m%s\033[0m \033[30;47m%s\033[0m \033[30;47m%s\033[0m", f1, f2, f3);
        append_centered(out, width, ft_vis_len, line_buf);

        tui_draw_widget(x, y, out);
    }
}

void draw_screen() {
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    int cols = w.ws_col;
    int rows = w.ws_row;

    if (cols != last_cols || rows != last_rows) {
        printf("\033[2J");
        last_cols = cols;
        last_rows = rows;
    }

    render_clock(3, cols);
    render_weather(13, cols);
    render_guestbook(17, cols);

    int bottom_y = rows - 15;
    if (bottom_y < 20) bottom_y = 20;

    int col_width = cols / 3;
    int left_x = 2;
    int center_x = col_width + 2;
    int right_x = (col_width * 2) + 2;

    render_news(left_x, bottom_y, col_width - 4);
    render_music(center_x, bottom_y, col_width - 4);
    render_calendar(right_x, bottom_y, col_width - 4);

    fflush(stdout);
}

void process_stream(char *buffer) {
    char expected_secret[65];
    populate_auth_token(expected_secret);
    int sec_len = strlen(expected_secret);

    if (sec_len > 0 && (strncmp(buffer, expected_secret, sec_len) != 0 || buffer[sec_len] != '|')) {
        return;
    }

    char *actual_payload = (sec_len > 0) ? (buffer + sec_len + 1) : buffer;

    tick++;
    if (strncmp(actual_payload, "CLOCK|", 6) == 0)
        snprintf(raw_clock, sizeof(raw_clock), "%s", actual_payload + 6);
    else if (strncmp(actual_payload, "WEATHER|", 8) == 0)
        snprintf(raw_weather, sizeof(raw_weather), "%s", actual_payload + 8);
    else if (strncmp(actual_payload, "NEWS|", 5) == 0)
        snprintf(raw_news, sizeof(raw_news), "%s", actual_payload + 5);
    else if (strncmp(actual_payload, "CALENDAR|", 9) == 0)
        snprintf(raw_calendar, sizeof(raw_calendar), "%s", actual_payload + 9);
    else if (strncmp(actual_payload, "GUESTBOOK|", 10) == 0)
        snprintf(raw_guestbook, sizeof(raw_guestbook), "%s", actual_payload + 10);
    else if (strncmp(actual_payload, "MUSIC|", 6) == 0)
        snprintf(raw_music, sizeof(raw_music), "%s", actual_payload + 6);
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

    draw_screen();
    int needs_redraw = 0;

    while ((bytes_read = recv(sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
        for (int i = 0; i < bytes_read; i++) {
            if (buffer[i] == '\n' || buffer[i] == '\0') {
                line[line_len] = '\0';
                if (line_len > 0) {
                    process_stream(line);
                    needs_redraw = 1;
                }
                line_len = 0;
            } else {
                if (line_len < (int)sizeof(line) - 1) line[line_len++] = buffer[i];
            }
        }

        if (needs_redraw) {
            draw_screen();
            needs_redraw = 0;
        }
    }
    handle_shutdown(0);
    return 0;
}