#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <curl/curl.h>
#include <json-c/json.h>
#include <time.h>
#include "../include/iwakura_net.h"

typedef struct { char *data; size_t len; } buf_t;

size_t write_cb(void *ptr, size_t size, size_t nmemb, void *userdata) {
    size_t n = size * nmemb;
    buf_t *b = (buf_t*)userdata;
    char *new_data = realloc(b->data, b->len + n + 1);
    if (!new_data) return 0;
    b->data = new_data;
    memcpy(b->data + b->len, ptr, n);
    b->len += n;
    b->data[b->len] = '\0';
    return n;
}

char access_token[512] = "";
time_t token_expires = 0;

void refresh_token() {
    const char *client_id = getenv("SPOTIFY_CLIENT_ID");
    const char *client_secret = getenv("SPOTIFY_CLIENT_SECRET");
    const char *refresh = getenv("SPOTIFY_REFRESH_TOKEN");

    if (!client_id || !client_secret || !refresh) {
        printf("[SPOTIFY] Missing credentials in .env!\n");
        return;
    }

    printf("[SPOTIFY] Requesting new access token...\n");

    CURL *curl = curl_easy_init();
    if (!curl) return;

    char post_fields[1024];
    snprintf(post_fields, sizeof(post_fields), "grant_type=refresh_token&refresh_token=%s", refresh);

    char userpwd[512];
    snprintf(userpwd, sizeof(userpwd), "%s:%s", client_id, client_secret);

    buf_t b = {0};
    curl_easy_setopt(curl, CURLOPT_URL, "https://accounts.spotify.com/api/token");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_fields);
    curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
    curl_easy_setopt(curl, CURLOPT_USERPWD, userpwd);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &b);

    long http_code = 0;
    if (curl_easy_perform(curl) == CURLE_OK) {
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        if (http_code == 200 && b.data) {
            struct json_object *j = json_tokener_parse(b.data);
            if (j) {
                struct json_object *tok, *exp;
                if (json_object_object_get_ex(j, "access_token", &tok)) {
                    snprintf(access_token, sizeof(access_token), "%s", json_object_get_string(tok));
                    printf("[SPOTIFY] Token refreshed successfully.\n");
                    if (json_object_object_get_ex(j, "expires_in", &exp)) {
                        token_expires = time(NULL) + json_object_get_int(exp) - 60;
                    }
                }
                json_object_put(j);
            }
        } else {
            printf("[SPOTIFY] Auth Failed! HTTP %ld: %s\n", http_code, b.data ? b.data : "No response");
        }
    }
    curl_easy_cleanup(curl);
    if (b.data) free(b.data);
}

void fetch_currently_playing() {
    if (time(NULL) >= token_expires) {
        refresh_token();
        if (strlen(access_token) == 0) return;
    }

    CURL *curl = curl_easy_init();
    if (!curl) return;

    struct curl_slist *headers = NULL;
    char auth_header[1024];
    snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", access_token);
    headers = curl_slist_append(headers, auth_header);

    buf_t b = {0};
    curl_easy_setopt(curl, CURLOPT_URL, "https://api.spotify.com/v1/me/player/currently-playing");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &b);

    long http_code = 0;
    if (curl_easy_perform(curl) == CURLE_OK) {
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        char payload[MAX_PAYLOAD];
        snprintf(payload, MAX_PAYLOAD, "0|%s|%s|0|1", _t("L_MUSIC_NOTHING", "No track selected"), _t("L_MUSIC_SILENCE", "Stopped"));

        if (http_code == 200 && b.data && strlen(b.data) > 0) {
            struct json_object *j = json_tokener_parse(b.data);
            if (j) {
                struct json_object *item, *artists, *first_artist, *artist_name, *track_name, *is_playing, *prog, *dur;
                if (json_object_object_get_ex(j, "item", &item) && !json_object_is_type(item, json_type_null)) {
                    json_object_object_get_ex(j, "is_playing", &is_playing);
                    json_object_object_get_ex(j, "progress_ms", &prog);
                    json_object_object_get_ex(item, "name", &track_name);
                    json_object_object_get_ex(item, "duration_ms", &dur);
                    json_object_object_get_ex(item, "artists", &artists);
                    first_artist = json_object_array_get_idx(artists, 0);
                    json_object_object_get_ex(first_artist, "name", &artist_name);

                    snprintf(payload, MAX_PAYLOAD, "%d|%s|%s|%d|%d",
                             json_object_get_boolean(is_playing) ? 1 : 0,
                             json_object_get_string(artist_name),
                             json_object_get_string(track_name),
                             json_object_get_int(prog),
                             json_object_get_int(dur));

                    printf("[SPOTIFY] Playing: %s - %s\n", json_object_get_string(artist_name), json_object_get_string(track_name));
                }
                json_object_put(j);
            }
        } else if (http_code == 204) {
            printf("[SPOTIFY] HTTP 204: No active playback detected by Spotify.\n");
        } else {
            printf("[SPOTIFY] HTTP %ld Error.\n", http_code);
        }

        net_push_to_hub(REQ_SPOTIFY, payload);
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    if (b.data) free(b.data);
}

int main() {
    curl_global_init(CURL_GLOBAL_DEFAULT);
    printf("[SPOTIFY] Provider starting...\n");

    int refresh_rate = get_refresh_rate("REFRESH_MUSIC", 6);

    while (1) {
        fetch_currently_playing();
        sleep(refresh_rate);
    }

    curl_global_cleanup();
    return 0;
}