#ifndef IWAKURA_NET_H
#define IWAKURA_NET_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdio.h>

#define MAX_PAYLOAD 1024

static inline int get_iwakura_port() {
    const char* val = getenv("DASH_PORT");
    return (val && strlen(val) > 0) ? atoi(val) : 8888;
}

static inline const char* get_iwakura_host() {
    const char* val = getenv("DASH_HOST");
    return (val && strlen(val) > 0) ? val : "127.0.0.1";
}

static inline int get_guestbook_port() {
    const char* val = getenv("GUESTBOOK_PORT");
    return (val && strlen(val) > 0) ? atoi(val) : 8890;
}

static inline int get_orchestrator_port() {
    const char* val = getenv("ORCH_PORT");
    return (val && strlen(val) > 0) ? atoi(val) : 8889;
}

typedef enum {
    REQ_CLOCK = 0,
    REQ_WEATHER,
    REQ_GUESTBOOK,
    REQ_SPOTIFY,
    REQ_LASTFM,
    REQ_NEWS,
    REQ_CAL,
    UPDATE_DATA = 99
} iwakura_req_t;

typedef struct {
    uint8_t type;
    uint8_t target_type;
    uint32_t payload_len;
    char payload[MAX_PAYLOAD];
} iwakura_msg_t;

static inline const char* _t(const char* key, const char* fallback) {
    const char* val = getenv(key);
    return (val && strlen(val) > 0) ? val : fallback;
}

static inline void net_push_to_hub(iwakura_req_t target, const char* payload) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return;

    struct sockaddr_in hub_addr = { .sin_family = AF_INET, .sin_port = htons(get_iwakura_port()) };
    inet_pton(AF_INET, get_iwakura_host(), &hub_addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&hub_addr, sizeof(hub_addr)) == 0) {
        iwakura_msg_t msg;
        memset(&msg, 0, sizeof(msg));
        msg.type = UPDATE_DATA;
        msg.target_type = target;
        snprintf(msg.payload, MAX_PAYLOAD, "%s", payload);
        msg.payload_len = strlen(msg.payload);
        send(sock, &msg, sizeof(msg), 0);
    }
    close(sock);
}

static inline void net_fetch_from_hub(iwakura_req_t target, char* buffer, const char* fallback) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        strcpy(buffer, fallback);
        return;
    }

    struct sockaddr_in hub_addr = { .sin_family = AF_INET, .sin_port = htons(get_iwakura_port()) };
    inet_pton(AF_INET, get_iwakura_host(), &hub_addr.sin_addr);

    strcpy(buffer, fallback);

    if (connect(sock, (struct sockaddr *)&hub_addr, sizeof(hub_addr)) == 0) {
        iwakura_msg_t msg;
        memset(&msg, 0, sizeof(msg));
        msg.type = target;
        send(sock, &msg, sizeof(msg), 0);

        if (recv(sock, &msg, sizeof(msg), 0) > 0 && strlen(msg.payload) > 0) {
            snprintf(buffer, MAX_PAYLOAD, "%s", msg.payload);
        }
    }
    close(sock);
}

static inline void net_push_to_orchestrator(const char* payload) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) return;

    struct sockaddr_in orch_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(get_orchestrator_port())
    };

    const char* host = get_iwakura_host();
    if (!host || strlen(host) == 0) host = "127.0.0.1";

    inet_pton(AF_INET, host, &orch_addr.sin_addr);

    sendto(sock, payload, strlen(payload), 0, (struct sockaddr *)&orch_addr, sizeof(orch_addr));
    close(sock);
}

#endif