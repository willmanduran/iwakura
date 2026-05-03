#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <time.h>
#include <stdarg.h>
#include <fcntl.h>

#define MAX_PROCS 20

pid_t child_pids[MAX_PROCS];
int child_count = 0;
FILE *log_file = NULL;

void log_msg(const char *fmt, ...) {
    if (!log_file) return;

    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    fprintf(log_file, "[%04d-%02d-%02d %02d:%02d:%02d] ",
            t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
            t->tm_hour, t->tm_min, t->tm_sec);

    va_list args;
    va_start(args, fmt);
    vfprintf(log_file, fmt, args);
    va_end(args);

    fprintf(log_file, "\n");
    fflush(log_file);
}

void handle_shutdown(int sig) {
    printf("\n\033[?25h\033[0m");
    log_msg("Caught signal %d. Initiating graceful shutdown...", sig);

    for (int i = 0; i < child_count; i++) {
        log_msg("Killing PID %d", child_pids[i]);
        kill(child_pids[i], SIGTERM);
    }

    while (wait(NULL) > 0);

    log_msg("Iwakura Core stopped cleanly. Goodbye!");
    if (log_file) fclose(log_file);

    printf("Iwakura Core stopped cleanly. Check ~/.iwakura/iwakura.log for details.\n");
    exit(0);
}

void spawn_process(const char *command) {
    if (child_count >= MAX_PROCS) return;

    pid_t pid = fork();

    if (pid < 0) {
        log_msg("ERROR: Fork failed for %s", command);
    } else if (pid == 0) {
        int fd = open("/dev/null", O_WRONLY);
        if (fd >= 0) {
            dup2(fd, STDOUT_FILENO);
            dup2(fd, STDERR_FILENO);
            close(fd);
        }

        char *args[] = {(char *)command, NULL};
        execv(command, args);

        exit(1);
    } else {
        log_msg("Spawned %s (PID: %d)", command, pid);
        child_pids[child_count++] = pid;
    }
}

void load_env(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        log_msg("WARNING: Could not open %s", filename);
        return;
    }

    int loaded = 0;
    char line[512];
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '#' || line[0] == '\n') continue;

        char *key = strtok(line, "=");
        char *val = strtok(NULL, "\n");
        if (key && val) {
            if (val[0] == '"') val++;
            if (val[strlen(val)-1] == '"') val[strlen(val)-1] = '\0';
            setenv(key, val, 1);
            loaded++;
        }
    }
    fclose(f);
    log_msg("Loaded %d variables from %s", loaded, filename);
}

int main() {
    int ret = system("mkdir -p $HOME/.iwakura");
    (void)ret;

    char log_path[512];
    snprintf(log_path, sizeof(log_path), "%s/.iwakura/iwakura.log", getenv("HOME"));
    log_file = fopen(log_path, "a");

    log_msg("=== IWAKURA CORE STARTING ===");

    signal(SIGINT, handle_shutdown);
    signal(SIGTERM, handle_shutdown);

    load_env(".env");

    const char *lang = getenv("IWAKURA_LANG");
    if (lang) {
        char lang_path[256];
        snprintf(lang_path, sizeof(lang_path), "lang/%s.env", lang);
        load_env(lang_path);
    } else {
        load_env("lang/en.env");
    }

    log_msg("Booting Data Hub...");
    spawn_process("./srv_hub");
    sleep(1);

    log_msg("Booting Data Servers...");
    spawn_process("./srv_clock");
    spawn_process("./srv_weather");
    spawn_process("./srv_guestbook");
    spawn_process("./srv_spotify");
    spawn_process("./srv_lastfm");
    spawn_process("./srv_calendar");
    spawn_process("./srv_news");

    sleep(1);

    log_msg("Booting UI Panels...");
    spawn_process("./pan_clock");
    spawn_process("./pan_weather");
    spawn_process("./pan_guestbook");
    spawn_process("./pan_news");
    spawn_process("./pan_calendar");
    spawn_process("./pan_music");

    log_msg("Booting UI Broker (PAN_HUB)...");
    spawn_process("./pan_hub");

    log_msg("All services online. Core running in background.");
    printf("Iwakura Core is running. You can now launch a frontend (e.g., ./front_tui)\n");

    while (1) {
        pause();
    }

    return 0;
}