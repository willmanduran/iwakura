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
    if (!new_data) return 0;
    b->data = new_data;
    memcpy(b->data + b->len, ptr, n);
    b->len += n;
    b->data[b->len] = '\0';
    return n;
}

void fetch_and_push_weather() {
    const char *lat = getenv("WX_LAT");
    const char *lon = getenv("WX_LON");

    char url[512];
    snprintf(url, sizeof(url),
             "https://api.open-meteo.com/v1/forecast?latitude=%s&longitude=%s"
             "&current=temperature_2m,apparent_temperature,wind_speed_10m,weather_code,is_day&timezone=auto",
             lat, lon);

    CURL *curl = curl_easy_init();
    if (!curl) return;

    buf_t b = {0};
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &b);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

    printf("[WEATHER] Fetching Open-Meteo data for %s, %s...\n", lat, lon);

    if (curl_easy_perform(curl) == CURLE_OK && b.data) {
        struct json_object *j = json_tokener_parse(b.data);
        if (j) {
            struct json_object *cur, *o;
            double t = 0, f = 0, w = 0; int c = 0, d = 0;

            if (json_object_object_get_ex(j, "current", &cur)) {
                if(json_object_object_get_ex(cur, "temperature_2m", &o)) t = json_object_get_double(o);
                if(json_object_object_get_ex(cur, "apparent_temperature", &o)) f = json_object_get_double(o);
                if(json_object_object_get_ex(cur, "wind_speed_10m", &o)) w = json_object_get_double(o);
                if(json_object_object_get_ex(cur, "weather_code", &o)) c = json_object_get_int(o);
                if(json_object_object_get_ex(cur, "is_day", &o)) d = json_object_get_int(o);

                char payload[MAX_PAYLOAD];
                snprintf(payload, MAX_PAYLOAD, "%.1f|%.1f|%.1f|%d|%d", t, f, w, c, d);

                net_push_to_hub(REQ_WEATHER, payload);
                printf("[WEATHER] Pushed to Hub: %s\n", payload);
            }
            json_object_put(j);
        }
    } else {
        printf("[WEATHER] Failed to reach Open-Meteo.\n");
    }

    curl_easy_cleanup(curl);
    if (b.data) free(b.data);
}

int main() {
    curl_global_init(CURL_GLOBAL_DEFAULT);
    printf("Weather Provider starting...\n");

    while (1) {
        fetch_and_push_weather();
        sleep(900);
    }

    curl_global_cleanup();
    return 0;
}