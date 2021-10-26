#ifndef ECHO_H
#define ECHO_H

/*extern int raftId = 0;
extern int term = 0;
extern int LeaderId = 0;
extern char *logmsg = "hello";
extern char log[100];
extern int LeaderCommit = 0;
extern int index = 0;
*/


void echo_server_init (int port);
int echo_client_send (char *send_buffer);
void echo_client_init (int *ipaddr, int port);

#endif	/* ECHO_H */
