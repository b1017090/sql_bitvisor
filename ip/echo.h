#ifndef ECHO_H
#define ECHO_H

void echo_server_init (int port);
int echo_client_send (char *send_buffer);
void echo_client_init (int *ipaddr, int port);

#endif	/* ECHO_H */
