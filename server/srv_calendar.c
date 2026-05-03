#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <curl/curl.h>
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

void get_iso_date(char *buf, int offset_days) {
    time_t now = time(NULL) + (offset_days * 86400);
    struct tm *t = gmtime(&now);
    strftime(buf, 20, "%Y%m%dT%H%M%SZ", t);
}

void parse_event_time(char *block, char *end_block, char *time_str) {
    strcpy(time_str, "[-] ");

    char *dtstart = strstr(block, "DTSTART");
    if (dtstart && dtstart < end_block) {
        char *val_start = strchr(dtstart, ':');
        if (val_start && val_start < end_block) {
            val_start++;

            char *t_marker = strchr(val_start, 'T');
            if (t_marker && (t_marker - val_start) == 8 && t_marker < end_block) {
                struct tm ev_time = {0};
                char format_str[32];
                strncpy(format_str, val_start, 15);
                format_str[15] = '\0';

                if (strptime(format_str, "%Y%m%dT%H%M%S", &ev_time)) {
                    time_t raw_time = timegm(&ev_time);
                    struct tm *local_time = localtime(&raw_time);

                    strftime(time_str, 16, "[%H:%M] ", local_time);
                }
            }
        }
    }
}

void extract_summaries(char *raw_data, char *final_payload, int *count) {
    char *search_ptr = raw_data;

    while (*count < 8) {
        char *block = strstr(search_ptr, "BEGIN:V");
        if (!block) break;

        char *end_block = strstr(block, "END:V");
        if (!end_block) break;

        char *completed = strstr(block, "STATUS:COMPLETED");
        if (completed && completed < end_block) {
            search_ptr = end_block + 5;
            continue;
        }

        char *summary = strstr(block, "SUMMARY:");
        if (summary && summary < end_block) {
            summary += 8;
            char *line_end = strstr(summary, "\r\n");
            if (line_end) {
                char time_prefix[16];
                parse_event_time(block, end_block, time_prefix);

                char entry[128];
                int len = line_end - summary;
                if (len > 80) len = 80;

                snprintf(entry, sizeof(entry), "%s%.*s", time_prefix, len, summary);

                if (strlen(final_payload) + strlen(entry) + 5 < MAX_PAYLOAD) {
                    strcat(final_payload, entry);
                    strcat(final_payload, " | ");
                    (*count)++;
                }
            }
        }
        search_ptr = end_block + 5;
    }
}

void fetch_and_push_calendar() {
    const char *url = getenv("CAL_URL");
    const char *user = getenv("CAL_USER");
    const char *pass = getenv("CAL_PASS");

    if (!url || !user || !pass || strlen(url) < 5) return;

    char start_date[20], end_date[20];
    get_iso_date(start_date, 0);
    get_iso_date(end_date, 1);

    char xml_events[1024];
    snprintf(xml_events, sizeof(xml_events),
        "<c:calendar-query xmlns:d='DAV:' xmlns:c='urn:ietf:params:xml:ns:caldav'>"
        "<d:prop><c:calendar-data/></d:prop>"
        "<c:filter><c:comp-filter name='VCALENDAR'>"
        "<c:comp-filter name='VEVENT'><c:time-range start='%s' end='%s'/></c:comp-filter>"
        "</c:comp-filter></c:filter></c:calendar-query>", start_date, end_date);

    char xml_todos[1024] =
        "<c:calendar-query xmlns:d='DAV:' xmlns:c='urn:ietf:params:xml:ns:caldav'>"
        "<d:prop><c:calendar-data/></d:prop>"
        "<c:filter><c:comp-filter name='VCALENDAR'>"
        "<c:comp-filter name='VTODO' />"
        "</c:comp-filter></c:filter></c:calendar-query>";

    CURL *curl = curl_easy_init();
    if (!curl) return;

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/xml; charset=utf-8");
    headers = curl_slist_append(headers, "Depth: 1");

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "REPORT");
    curl_easy_setopt(curl, CURLOPT_USERNAME, user);
    curl_easy_setopt(curl, CURLOPT_PASSWORD, pass);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    char final_payload[MAX_PAYLOAD] = "";
    int item_count = 0;

    buf_t b_events = {0};
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, xml_events);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &b_events);
    if (curl_easy_perform(curl) == CURLE_OK && b_events.data) {
        extract_summaries(b_events.data, final_payload, &item_count);
    }
    if (b_events.data) free(b_events.data);

    buf_t b_todos = {0};
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, xml_todos);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &b_todos);
    if (curl_easy_perform(curl) == CURLE_OK && b_todos.data) {
        extract_summaries(b_todos.data, final_payload, &item_count);
    }
    if (b_todos.data) free(b_todos.data);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (strlen(final_payload) > 3) {
        final_payload[strlen(final_payload) - 3] = '\0';
    } else {
        strcpy(final_payload, _t("L_CAL_EMPTY", "No events scheduled for today."));
    }

    net_push_to_hub(REQ_CAL, final_payload);
}

int main() {
    curl_global_init(CURL_GLOBAL_DEFAULT);
    int refresh_rate = get_refresh_rate("REFRESH_CAL", 30);
    while (1) {
        fetch_and_push_calendar();
        sleep(refresh_rate);
    }
    curl_global_cleanup();
    return 0;
}