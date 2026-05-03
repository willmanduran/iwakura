#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <curl/curl.h>
#include <json-c/json.h>
#include "../include/iwakura_net.h"

typedef struct { char *data; size_t len; } buf_t;

size_t write_cb(void *ptr, size_t size, size_t nmemb, void *userdata) {
    size_t n = size * nmemb;
    buf_t *b = (buf_t*)userdata;
    char *new_data = realloc(b->data, b->len + n + 1);
    if (!new_data) {
        free(b->data);
        b->data = NULL;
        return 0;
    }
    b->data = new_data;
    memcpy(b->data + b->len, ptr, n);
    b->len += n;
    b->data[b->len] = '\0';
    return n;
}

void fetch_and_push_lastfm() {
    const char *api_key = getenv("LASTFM_API_KEY");
    const char *user = getenv("LASTFM_USER");
    const char *display_name = getenv("LASTFM_DISPLAY_NAME");

    if (!api_key || !user || !display_name) return;

    char url_top[512], url_info[512];
    snprintf(url_top, sizeof(url_top), "http://ws.audioscrobbler.com/2.0/?method=user.gettopartists&user=%s&api_key=%s&period=7day&limit=1&format=json", user, api_key);
    snprintf(url_info, sizeof(url_info), "http://ws.audioscrobbler.com/2.0/?method=user.getinfo&user=%s&api_key=%s&format=json", user, api_key);

    CURL *curl = curl_easy_init();
    if (!curl) return;

    char top_artist[128] = "N/A";
    char scrobbles[32] = "0";

    buf_t b_top = {0};
    curl_easy_setopt(curl, CURLOPT_URL, url_top);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &b_top);
    if (curl_easy_perform(curl) == CURLE_OK && b_top.data) {
        struct json_object *j = json_tokener_parse(b_top.data);
        if (j) {
            struct json_object *t_arts, *art_arr, *first_art, *name;
            if (json_object_object_get_ex(j, "topartists", &t_arts) &&
                json_object_object_get_ex(t_arts, "artist", &art_arr)) {
                if (json_object_array_length(art_arr) > 0) {
                    first_art = json_object_array_get_idx(art_arr, 0);
                    if (json_object_object_get_ex(first_art, "name", &name)) {
                        snprintf(top_artist, sizeof(top_artist), "%s", json_object_get_string(name));
                    }
                }
            }
            json_object_put(j);
        }
    }
    if (b_top.data) free(b_top.data);

    buf_t b_info = {0};
    curl_easy_setopt(curl, CURLOPT_URL, url_info);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &b_info);
    if (curl_easy_perform(curl) == CURLE_OK && b_info.data) {
        struct json_object *j = json_tokener_parse(b_info.data);
        if (j) {
            struct json_object *u, *playcount;
            if (json_object_object_get_ex(j, "user", &u) &&
                json_object_object_get_ex(u, "playcount", &playcount)) {
                snprintf(scrobbles, sizeof(scrobbles), "%s", json_object_get_string(playcount));
            }
            json_object_put(j);
        }
    }
    if (b_info.data) free(b_info.data);
    curl_easy_cleanup(curl);

    char payload[MAX_PAYLOAD];
    snprintf(payload, MAX_PAYLOAD, "%s|%s: %s|%s: %s",
             display_name,
             _t("L_LFM_TOP", "Top"),
             top_artist,
             _t("L_LFM_SCROBBLES", "Scrobbles"),
             scrobbles);

    net_push_to_hub(REQ_LASTFM, payload);
    printf("[LASTFM] Pushed: %s\n", payload);
}

int main() {
    curl_global_init(CURL_GLOBAL_DEFAULT);
    printf("Last.fm Provider starting...\n");

    int refresh_rate = get_refresh_rate("REFRESH_LFM", 300);

    while (1) {
        fetch_and_push_lastfm();
        sleep(refresh_rate);
    }

    curl_global_cleanup();
    return 0;
}