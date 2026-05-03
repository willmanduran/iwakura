#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <curl/curl.h>
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

void clean_title(char *title) {
    char *cdata = strstr(title, "<![CDATA[");
    if (cdata) {
        memmove(title, cdata + 9, strlen(cdata + 9) + 1);
        char *end = strstr(title, "]]>");
        if (end) *end = '\0';
    }
}

void fetch_and_push_news() {
    const char *list_path = getenv("RSS_LIST_PATH");
    if (!list_path) return;

    FILE *f = fopen(list_path, "r");
    if (!f) {
        return;
    }

    CURL *curl = curl_easy_init();
    if (!curl) {
        fclose(f);
        return;
    }

    char url[256];
    char final_payload[MAX_PAYLOAD] = "";
    int total_items = 0;

    while (fgets(url, sizeof(url), f) && total_items < 10) {
        url[strcspn(url, "\r\n")] = 0;
        if (strlen(url) < 10 || url[0] == '#') continue;

        buf_t b = {0};
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &b);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

        if (curl_easy_perform(curl) == CURLE_OK && b.data) {
            char *ptr = b.data;

            while (1) {
                char *item_pos = strstr(ptr, "<item>");
                char *entry_pos = strstr(ptr, "<entry>");

                if (!item_pos && !entry_pos) break;

                if (item_pos && entry_pos) {
                    ptr = (item_pos < entry_pos) ? item_pos : entry_pos;
                } else {
                    ptr = item_pos ? item_pos : entry_pos;
                }

                if (total_items >= 10) break;

                char *title_start = strstr(ptr, "<title>");
                if (title_start) {
                    title_start += 7;
                    char *title_end = strstr(title_start, "</title>");
                    if (title_end) {
                        char entry[256];
                        int len = title_end - title_start;
                        if (len > 250) len = 250;
                        strncpy(entry, title_start, len);
                        entry[len] = '\0';

                        clean_title(entry);

                        if (strlen(final_payload) + strlen(entry) + 5 < MAX_PAYLOAD) {
                            strcat(final_payload, entry);
                            strcat(final_payload, "|");
                            total_items++;
                        }
                    }
                }
                ptr += 6;
            }
        }
        if (b.data) free(b.data);
    }

    fclose(f);
    curl_easy_cleanup(curl);

    if (strlen(final_payload) > 0) {
        final_payload[strlen(final_payload) - 1] = '\0';
    } else {
        strcpy(final_payload, _t("L_NEWS_EMPTY", "No news stories found."));
    }

    net_push_to_hub(REQ_NEWS, final_payload);
}

int main() {
    curl_global_init(CURL_GLOBAL_DEFAULT);
    int refresh_rate = get_refresh_rate("REFRESH_NEWS", 300);
    while (1) {
        fetch_and_push_news();
        sleep(refresh_rate);
    }
    curl_global_cleanup();
    return 0;
}