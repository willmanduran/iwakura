#ifndef IWAKURA_ORCH_H
#define IWAKURA_ORCH_H

void tui_init();
void tui_cleanup();
void tui_draw_widget(int start_x, int start_y, char *payload);

int net_init_udp_server(int port);
void net_close_udp_server(int sock);

void route_packet(char *buffer);

#endif