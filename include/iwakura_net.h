#ifndef IWAKURA_NET_H
#define IWAKURA_NET_H

#include <stdint.h>
#include <stdlib.h>

#define MAX_PAYLOAD 1024

static inline int get_iwakura_port() {
    return atoi(getenv("IWAKURA_PORT"));
}

static inline const char* get_iwakura_host() {
    return getenv("IWAKURA_HOST");
}

static inline int get_guestbook_port() {
    return atoi(getenv("GUESTBOOK_PORT"));
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