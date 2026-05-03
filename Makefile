CC = gcc
CFLAGS = -Wall -Wextra -O2

all: srv_hub srv_clock srv_guestbook srv_weather srv_spotify srv_lastfm srv_calendar pan_guestbook pan_music pan_calendar pan_clock pan_weather srv_news pan_news pan_hub front_tui iwakura

srv_hub: core/srv_hub.c
	$(CC) $(CFLAGS) -o srv_hub core/srv_hub.c

srv_clock: server/srv_clock.c
	$(CC) $(CFLAGS) -o srv_clock server/srv_clock.c

srv_guestbook: server/srv_guestbook.c
	$(CC) $(CFLAGS) -o srv_guestbook server/srv_guestbook.c

srv_weather: server/srv_weather.c
	$(CC) $(CFLAGS) -o srv_weather server/srv_weather.c -lcurl -ljson-c

srv_spotify: server/srv_spotify.c
	$(CC) $(CFLAGS) -o srv_spotify server/srv_spotify.c -lcurl -ljson-c

srv_lastfm: server/srv_lastfm.c
	$(CC) $(CFLAGS) -o srv_lastfm server/srv_lastfm.c -lcurl -ljson-c

srv_calendar: server/srv_calendar.c
	$(CC) $(CFLAGS) -o srv_calendar server/srv_calendar.c -lcurl

pan_guestbook: panels/pan_guestbook.c
	$(CC) $(CFLAGS) -o pan_guestbook panels/pan_guestbook.c

pan_music: panels/pan_music.c
	$(CC) $(CFLAGS) -o pan_music panels/pan_music.c

pan_calendar: panels/pan_calendar.c
	$(CC) $(CFLAGS) -o pan_calendar panels/pan_calendar.c

pan_clock: panels/pan_clock.c
	$(CC) $(CFLAGS) -o pan_clock panels/pan_clock.c

pan_weather: panels/pan_weather.c
	$(CC) $(CFLAGS) -o pan_weather panels/pan_weather.c

srv_news: server/srv_news.c
	$(CC) $(CFLAGS) -o srv_news server/srv_news.c -lcurl

pan_news: panels/pan_news.c
	$(CC) $(CFLAGS) -o pan_news panels/pan_news.c

pan_hub: core/pan_hub.c
	$(CC) $(CFLAGS) -o pan_hub core/pan_hub.c

front_tui: frontends/front_tui.c
	$(CC) $(CFLAGS) -o front_tui frontends/front_tui.c

iwakura: main.c
	$(CC) $(CFLAGS) -o iwakura main.c

clean:
	rm -f srv_hub srv_clock srv_guestbook srv_weather srv_spotify srv_lastfm srv_calendar pan_guestbook pan_music pan_calendar pan_clock pan_weather srv_news pan_news pan_hub front_tui iwakura