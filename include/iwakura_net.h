#ifndef IWAKURA_NET_H
#define IWAKURA_NET_H

#include <stdint.h>

#define IWAKURA_PORT 9000
#define MAX_PAYLOAD 1024

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