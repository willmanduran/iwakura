#ifndef IWAKURA_NET_H
#define IWAKURA_NET_H

#include <stdint.h>
#include <stdlib.h>

#define MAX_PAYLOAD 1024

static inline int get_iwakura_port() {
    const char *p = getenv("IWAKURA_PORT");
    return p ? atoi(p) : 9000;
}

static inline const char* get_iwakura_host() {
    const char *h = getenv("IWAKURA_HOST");
    return h ? h : "127.0.0.1";
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

#endif